///////////////////////////////////////////////////////////////////////////////
// 文件名称: http.cpp
// 功能描述: HTTP客户端
///////////////////////////////////////////////////////////////////////////////

#include "BaseHttp.h"
#include "LogManager.h"
#include "base64.h"
#include "sha1.h"

///////////////////////////////////////////////////////////////////////////////
// Misc Routines

std::string getHttpProtoVerStr(HTTP_PROTO_VER version)
{
    switch (version)
    {
    case HPV_1_0:  return "1.0";
    case HPV_1_1:  return "1.1";
    default:       return "";
    }
}

//-----------------------------------------------------------------------------

std::string getHttpMethodStr(HTTP_METHOD_TYPE httpMethod)
{
    switch (httpMethod)
    {
    case HMT_GET:   return "GET";
    case HMT_POST:  return "POST";
    default:        return "";
    }
}

//-----------------------------------------------------------------------------

std::string getHttpStatusMessage(int statusCode)
{
    switch (statusCode)
    {
    case 100: return "Continue";
    // 2XX: Success
    case 200: return "OK";
    case 201: return "Created";
    case 202: return "Accepted";
    case 203: return "Non-authoritative Information";
    case 204: return "No Content";
    case 205: return "Reset Content";
    case 206: return "Partial Content";
    // 3XX: Redirections
    case 301: return "Moved Permanently";
    case 302: return "Moved Temporarily";
    case 303: return "See Other";
    case 304: return "Not Modified";
    case 305: return "Use Proxy";
    // 4XX Client Errors
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method not allowed";
    case 406: return "Not Acceptable";
    case 407: return "Proxy Authentication Required";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 410: return "Gone";
    case 411: return "Length Required";
    case 412: return "Precondition Failed";
    case 413: return "Request Entity Too Long";
    case 414: return "Request-URI Too Long. 256 Chars max";
    case 415: return "Unsupported Media Type";
    case 417: return "Expectation Failed";
    // 5XX Server errors
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    case 504: return "Gateway timeout";
    case 505: return "HTTP version not supported";
    default:  return "Unknown Status Code";
    }
}

///////////////////////////////////////////////////////////////////////////////
// class HttpHeaderStrList

HttpHeaderStrList::HttpHeaderStrList()
{
    nameValueSep_ = ":";
}

//-----------------------------------------------------------------------------

int HttpHeaderStrList::add(const std::string& str)
{
    return items_.add(str.c_str());
}

//-----------------------------------------------------------------------------

void HttpHeaderStrList::del(int index)
{
    items_.del(index);
}

//-----------------------------------------------------------------------------

void HttpHeaderStrList::clear()
{
    items_.clear();
}

//-----------------------------------------------------------------------------

void HttpHeaderStrList::addStrings(const HttpHeaderStrList& strings)
{
    for (int i = 0; i < strings.getCount(); i++)
        add(strings.getString(i));
}

//-----------------------------------------------------------------------------

void HttpHeaderStrList::moveToTop(const StrList& nameList)
{
    for (int i = nameList.getCount() - 1; i >= 0; i--)
    {
        int index = indexOfName(nameList[i]);
        if (index >= 0)
            items_.move(index, 0);
    }
}

//-----------------------------------------------------------------------------

int HttpHeaderStrList::indexOfName(const std::string& name) const
{
    int result = -1;

    for (int i = 0; i < items_.getCount(); i++)
        if (sameText(getName(i), name))
        {
            result = i;
            break;
        }

    return result;
}

//-----------------------------------------------------------------------------

std::string HttpHeaderStrList::getText() const
{
    std::string result;
    for (int i = 0; i < items_.getCount(); i++)
    {
        std::string name = getName(i);
        std::string value = getValue(i);
        if (!name.empty() && !value.empty())
        {
            result += makeLine(name, value);
            result += "\r\n";
        }
    }
    return result;
}

//-----------------------------------------------------------------------------

std::string HttpHeaderStrList::getName(int index) const
{
    std::string result;
    std::string line = items_[index];
    std::string::size_type pos = line.find(nameValueSep_);
    if (pos != std::string::npos && pos > 0)
        result = trimString(line.substr(0, pos));
    return result;
}

//-----------------------------------------------------------------------------

std::string HttpHeaderStrList::getValue(int index) const
{
    std::string result;
    std::string line = items_[index];
    std::string::size_type pos = line.find(nameValueSep_);
    if (pos != std::string::npos && pos > 0)
        result = trimString(line.substr(pos + 1));
    return result;
}

//-----------------------------------------------------------------------------

std::string HttpHeaderStrList::getValue(const std::string& name) const
{
    std::string result;
    int index = indexOfName(name);
    if (index >= 0)
        result = getValue(index);
    return result;
}

//-----------------------------------------------------------------------------

void HttpHeaderStrList::setValue(const std::string& name, const std::string& value)
{
    std::string newName(trimString(name));
    std::string newValue(trimString(value));

    int index = indexOfName(newName);
    if (newValue.empty())
    {
        if (index >= 0) del(index);
    }
    else
    {
        if (index < 0)
            index = add("");
        items_.setString(index, makeLine(newName, newValue).c_str());
    }
}

//-----------------------------------------------------------------------------

std::string HttpHeaderStrList::makeLine(const std::string& name, const std::string& value) const
{
    return name + nameValueSep_ + " " + value;
}

///////////////////////////////////////////////////////////////////////////////
// class HttpEntityHeaderInfo

void HttpEntityHeaderInfo::init()
{
    rawHeaders_.clear();
    customHeaders_.clear();
    cacheControl_ = "no-cache";
    connection_ = "close";
    contentDisposition_.clear();
    contentEncoding_.clear();
    contentLanguage_.clear();
    contentLength_ = -1;
    contentRangeStart_ = 0;
    contentRangeEnd_ = 0;
    contentRangeInstanceLength_ = 0;
    contentType_.clear();
    contentVersion_.clear();
    date_.clear();
    expires_.clear();
    eTag_.clear();
    lastModified_.clear();
    pragma_ = "no-cache";
    transferEncoding_.clear();
}

//-----------------------------------------------------------------------------

void HttpEntityHeaderInfo::clear()
{
    init();
}

//-----------------------------------------------------------------------------

void HttpEntityHeaderInfo::parseHeaders()
{
    cacheControl_ = rawHeaders_.getValue("Cache-control");
    connection_ = rawHeaders_.getValue("Connection");
    contentVersion_ = rawHeaders_.getValue("Content-Version");
    contentDisposition_ = rawHeaders_.getValue("Content-Disposition");
    contentEncoding_ = rawHeaders_.getValue("Content-Encoding");
    contentLanguage_ = rawHeaders_.getValue("Content-Language");
    contentType_ = rawHeaders_.getValue("Content-Type");
    contentLength_ = strToInt64(rawHeaders_.getValue("Content-Length"), -1);

    contentRangeStart_ = 0;
    contentRangeEnd_ = 0;
    contentRangeInstanceLength_ = 0;

    /* Content-Range Examples: */
    // content-range: bytes 1-65536/102400
    // content-range: bytes */102400
    // content-range: bytes 1-65536/*

    std::string s = rawHeaders_.getValue("Content-Range");
    if (!s.empty())
    {
        fetchStr(s);
        std::string strRange = fetchStr(s, '/');
        std::string strLength = fetchStr(s);

        contentRangeStart_ = strToInt64(fetchStr(strRange, '-'), 0);
        contentRangeEnd_ = strToInt64(strRange, 0);
        contentRangeInstanceLength_ = strToInt64(strLength, 0);
    }

    date_ = rawHeaders_.getValue("Date");
    lastModified_ = rawHeaders_.getValue("Last-Modified");
    expires_ = rawHeaders_.getValue("Expires");
    eTag_ = rawHeaders_.getValue("ETag");
    pragma_ = rawHeaders_.getValue("Pragma");
    transferEncoding_ = rawHeaders_.getValue("Transfer-Encoding");
}

//-----------------------------------------------------------------------------

void HttpEntityHeaderInfo::buildHeaders()
{
    rawHeaders_.clear();

    if (!connection_.empty())
        rawHeaders_.setValue("Connection", connection_);
    if (!contentVersion_.empty())
        rawHeaders_.setValue("Content-Version", contentVersion_);
    if (!contentDisposition_.empty())
        rawHeaders_.setValue("Content-Disposition", contentDisposition_);
    if (!contentEncoding_.empty())
        rawHeaders_.setValue("Content-Encoding", contentEncoding_);
    if (!contentLanguage_.empty())
        rawHeaders_.setValue("Content-Language", contentLanguage_);
    if (!contentType_.empty())
        rawHeaders_.setValue("Content-Type", contentType_);
    if (contentLength_ >= 0)
        rawHeaders_.setValue("Content-Length", intToStr(contentLength_));
    if (!cacheControl_.empty())
        rawHeaders_.setValue("Cache-control", cacheControl_);
    if (!date_.empty())
        rawHeaders_.setValue("Date", date_);
    if (!eTag_.empty())
        rawHeaders_.setValue("ETag", eTag_);
    if (!expires_.empty())
        rawHeaders_.setValue("Expires", expires_);
    if (!pragma_.empty())
        rawHeaders_.setValue("Pragma", pragma_);
    if (!transferEncoding_.empty())
        rawHeaders_.setValue("Transfer-Encoding", transferEncoding_);

    if (customHeaders_.getCount() > 0)
        rawHeaders_.addStrings(customHeaders_);
}

///////////////////////////////////////////////////////////////////////////////
// class HttpRequestHeaderInfo

void HttpRequestHeaderInfo::init()
{
    accept_ = "text/html, */*";
    acceptCharSet_.clear();
    acceptEncoding_.clear();
    acceptLanguage_.clear();
    from_.clear();
    referer_.clear();
    userAgent_ = DEFAULT_USER_AGENT;
    host_.clear();
    range_.clear();
}

//-----------------------------------------------------------------------------

void HttpRequestHeaderInfo::clear()
{
    HttpEntityHeaderInfo::clear();
    init();
}

//-----------------------------------------------------------------------------

void HttpRequestHeaderInfo::parseHeaders()
{
    HttpEntityHeaderInfo::parseHeaders();

    accept_ = rawHeaders_.getValue("Accept");
    acceptCharSet_ = rawHeaders_.getValue("Accept-Charset");
    acceptEncoding_ = rawHeaders_.getValue("Accept-Encoding");
    acceptLanguage_ = rawHeaders_.getValue("Accept-Language");
    host_ = rawHeaders_.getValue("Host");
    from_ = rawHeaders_.getValue("From");
    referer_ = rawHeaders_.getValue("Referer");
    userAgent_ = rawHeaders_.getValue("User-Agent");

    // strip off the 'bytes=' portion of the header
    std::string s = rawHeaders_.getValue("Range");
    fetchStr(s, '=');
    range_ = s;
}

//-----------------------------------------------------------------------------

void HttpRequestHeaderInfo::buildHeaders()
{
    HttpEntityHeaderInfo::buildHeaders();

    if (!host_.empty())
        rawHeaders_.setValue("Host", host_);
    if (!accept_.empty())
        rawHeaders_.setValue("Accept", accept_);
    if (!acceptCharSet_.empty())
        rawHeaders_.setValue("Accept-Charset", acceptCharSet_);
    if (!acceptEncoding_.empty())
        rawHeaders_.setValue("Accept-Encoding", acceptEncoding_);
    if (!acceptLanguage_.empty())
        rawHeaders_.setValue("Accept-Language", acceptLanguage_);
    if (!from_.empty())
        rawHeaders_.setValue("From", from_);
    if (!referer_.empty())
        rawHeaders_.setValue("Referer", referer_);
    if (!userAgent_.empty())
        rawHeaders_.setValue("User-Agent", userAgent_);
    if (!range_.empty())
        rawHeaders_.setValue("Range", "bytes=" + range_);
    if (!lastModified_.empty())
        rawHeaders_.setValue("If-Modified-Since", lastModified_);

    // Sort the list
    StrList nameList;
    nameList.add("Host");
    nameList.add("Accept");
    nameList.add("Accept-Charset");
    nameList.add("Accept-Encoding");
    nameList.add("Accept-Language");
    nameList.add("From");
    nameList.add("Referer");
    nameList.add("User-Agent");
    nameList.add("Range");
    nameList.add("Connection");
    rawHeaders_.moveToTop(nameList);
}

//-----------------------------------------------------------------------------

void HttpRequestHeaderInfo::setRange(INT64 rangeStart, INT64 rangeEnd)
{
    std::string range = formatString("%I64d-", rangeStart);
    if (rangeEnd >= 0)
        range += formatString("%I64d", rangeEnd);
    setRange(range);
}

///////////////////////////////////////////////////////////////////////////////
// class HttpResponseHeaderInfo

void HttpResponseHeaderInfo::init()
{
    acceptRanges_.clear();
    location_.clear();
    server_.clear();

    contentType_ = "text/html";
}

//-----------------------------------------------------------------------------

void HttpResponseHeaderInfo::clear()
{
    HttpEntityHeaderInfo::clear();
    init();
}

//-----------------------------------------------------------------------------

void HttpResponseHeaderInfo::parseHeaders()
{
    HttpEntityHeaderInfo::parseHeaders();

    acceptRanges_ = rawHeaders_.getValue("Accept-Ranges");
    location_ = rawHeaders_.getValue("Location");
    server_ = rawHeaders_.getValue("Server");
}

//-----------------------------------------------------------------------------

void HttpResponseHeaderInfo::buildHeaders()
{
    HttpEntityHeaderInfo::buildHeaders();

    if (hasContentRange() || hasContentRangeInstance())
    {
        std::string cr = (hasContentRange() ?
            formatString("%I64d-%I64d", contentRangeStart_, contentRangeEnd_) : "*");
        std::string ci = (hasContentRangeInstance() ?
            intToStr(contentRangeInstanceLength_) : "*");

        rawHeaders_.setValue("Content-Range", "bytes " + cr + "/" + ci);
    }

    if (!acceptRanges_.empty())
        rawHeaders_.setValue("Accept-Ranges", acceptRanges_);
}

///////////////////////////////////////////////////////////////////////////////
// class HttpRequest

HttpRequest::HttpRequest()
{
    init();
}

//-----------------------------------------------------------------------------

void HttpRequest::init()
{
    protocolVersion_ = HPV_1_1;
    url_.clear();
    method_.clear();
    contentStream_ = NULL;
}

//-----------------------------------------------------------------------------

void HttpRequest::clear()
{
    HttpRequestHeaderInfo::clear();
    init();
}

//-----------------------------------------------------------------------------

bool HttpRequest::setRequestLine(const std::string& reqLine)
{
    return parseRequestLine(reqLine, method_, url_, protocolVersion_);
}

//-----------------------------------------------------------------------------

void HttpRequest::makeRequestHeaderBuffer(Buffer& buffer)
{
    buildHeaders();

    std::string text;
    text = method_ + " " + url_ + " HTTP/" + getHttpProtoVerStr(protocolVersion_) + "\r\n";

    for (int i = 0; i < getRawHeaders().getCount(); i++)
    {
        std::string s = getRawHeaders()[i];
        if (!s.empty())
            text = text + s + "\r\n";
    }

    text += "\r\n";

    buffer.assign(text.c_str(), (int)text.length());
}

//-----------------------------------------------------------------------------

// example: "GET /images/logo.gif HTTP/1.1"
bool HttpRequest::parseRequestLine(const std::string& reqLine, std::string& method,
    std::string& url, HTTP_PROTO_VER& protoVer)
{
    bool result = false;
    StrList strList;

    splitString(trimString(reqLine), ' ', strList, true);
    result = (strList.getCount() == 3);
    if (result)
    {
        std::string s = strList[0];
        result = (s == "HEAD" || s == "GET" || s == "POST");
    }

    if (result)
    {
        std::string s = strList[2];
        result = (s == "HTTP/1.0" || s == "HTTP/1.1");
    }

    if (result)
    {
        method = strList[0];
        url = strList[1];
        protoVer = (strList[2] == "HTTP/1.1" ? HPV_1_1 : HPV_1_0);
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class HttpResponse

HttpResponse::HttpResponse()
{
    init();
}

HttpResponse::~HttpResponse()
{
    setContentStream(NULL, false);
}

//-----------------------------------------------------------------------------

void HttpResponse::init()
{
    statusLine_.clear();
    contentStream_ = NULL;
    ownsContentStream_ = false;
}

//-----------------------------------------------------------------------------

void HttpResponse::clear()
{
    HttpResponseHeaderInfo::clear();
    init();
}

//-----------------------------------------------------------------------------

bool HttpResponse::getKeepAlive() const
{
    std::string str = getConnection();
    bool result = !str.empty() && !sameText(str, "close");
    return result;
}

//-----------------------------------------------------------------------------

int HttpResponse::getStatusCode() const
{
    std::string s = statusLine_;
    fetchStr(s);
    s = trimString(s);
    s = fetchStr(s);
    s = fetchStr(s, '.');

    return strToInt(s, -1);
}

//-----------------------------------------------------------------------------

HTTP_PROTO_VER HttpResponse::getResponseVersion() const
{
    // eg: HTTP/1.1 200 OK
    std::string s = (statusLine_.length() > 8) ? statusLine_.substr(5, 3) : "";

    if (sameText(s, getHttpProtoVerStr(HPV_1_0)))
        return HPV_1_0;
    else if (sameText(s, getHttpProtoVerStr(HPV_1_1)))
        return HPV_1_1;
    else
        return HPV_1_1;
}

//-----------------------------------------------------------------------------

void HttpResponse::setStatusCode(int statusCode)
{
    statusLine_ = formatString("HTTP/%s %d %s",
        getHttpProtoVerStr(getResponseVersion()).c_str(),
        statusCode, getHttpStatusMessage(statusCode).c_str());
}

//-----------------------------------------------------------------------------

void HttpResponse::setContentStream(Stream *stream, bool ownsObject)
{
    if (ownsContentStream_)
    {
        delete contentStream_;
        contentStream_ = NULL;
    }

    contentStream_ = stream;
    ownsContentStream_ = ownsObject;
}

//-----------------------------------------------------------------------------

void HttpResponse::makeResponseHeaderBuffer(Buffer& buffer)
{
    buildHeaders();

    std::string text;
    text = statusLine_ + "\r\n";

    for (int i = 0; i < getRawHeaders().getCount(); i++)
    {
        std::string s = getRawHeaders()[i];
        if (!s.empty())
            text = text + s + "\r\n";
    }

    text += "\r\n";

    buffer.assign(text.c_str(), (int)text.length());
}

///////////////////////////////////////////////////////////////////////////////
// class CustomHttpClient

CustomHttpClient::CustomHttpClient() :
    handleRedirects_(true),
    redirectCount_(0),
    lastKeepAlive_(false)
{
    ensureNetworkInited();
}

CustomHttpClient::~CustomHttpClient()
{
    // nothing
}

//-----------------------------------------------------------------------------

InetAddress CustomHttpClient::getInetAddrFromUrl(Url& url)
{
    InetAddress inetAddr(0, 0);

    if (!url.getHost().empty())
    {
        std::string strIp = lookupHostAddr(url.getHost());
        if (!strIp.empty())
        {
            inetAddr.ip = stringToIp(strIp);
            inetAddr.port = static_cast<WORD>(strToInt(url.getPort(), DEFAULT_HTTP_PORT));
        }
    }

    return inetAddr;
}

//-----------------------------------------------------------------------------

int CustomHttpClient::beforeRequest(HTTP_METHOD_TYPE httpMethod, const std::string& urlStr,
    Stream *requestContent, Stream *responseContent,
    INT64 reqStreamPos, INT64 resStreamPos)
{
    ASSERT_X(httpMethod == HMT_GET || httpMethod == HMT_POST);

    if (httpMethod == HMT_POST)
    {
        tcpClient_.disconnect();

        // Currently when issuing a POST, SFC HTTP will automatically set the protocol
        // to version 1.0 independently of the value it had initially. This is because
        // there are some servers that don't respect the RFC to the full extent. In
        // particular, they don't respect sending/not sending the Expect: 100-Continue
        // header. Until we find an optimum solution that does NOT break the RFC, we
        // will restrict POSTS to version 1.0.
        request_.setProtocolVersion(HPV_1_0);

        // Usual posting request have default ContentType is "application/x-www-form-urlencoded"
        if (request_.getContentType().empty() ||
            sameText(request_.getContentType(), "text/html"))
        {
            request_.setContentType("application/x-www-form-urlencoded");
        }
    }

    Url url(urlStr);
    if (redirectCount_ == 0)
    {
        if (url.getHost().empty())
            return EC_HTTP_URL_ERROR;
        url_ = url;
    }
    else
    {
        if (url.getHost().empty())
        {
            Url OldUrl = url_;
            url_ = url;
            url_.setProtocol(OldUrl.getProtocol());
            url_.setHost(OldUrl.getHost());
            url_.setPort(OldUrl.getPort());
        }
        else
            url_ = url;
    }

    if (requestContent)
        requestContent->setPosition(reqStreamPos);
    if (responseContent)
    {
        responseContent->setSize(resStreamPos);
        responseContent->setPosition(resStreamPos);
    }

    request_.setMethod(getHttpMethodStr(httpMethod));
    request_.setUrl(url_.getUrl(Url::URL_PATH | Url::URL_FILENAME | Url::URL_PARAMS));

    //if (request_.getReferer().empty())
    //	request_.setReferer(url_.getUrl(Url::URL_ALL & ~(Url::URL_FILENAME | Url::URL_BOOKMARK | Url::URL_PARAMS)));

    int port = strToInt(url.getPort(), DEFAULT_HTTP_PORT);
    request_.setHost(port == DEFAULT_HTTP_PORT ?
        url_.getHost() : url_.getHost() + ":" + intToStr(port));

    request_.setContentLength(requestContent?
        requestContent->getSize() - requestContent->getPosition() : -1);
    request_.setContentStream(requestContent);

    response_.clear();
    response_.setContentStream(responseContent);

    return EC_HTTP_SUCCESS;
}

//-----------------------------------------------------------------------------

void CustomHttpClient::checkResponseHeader(char *buffer, int size, bool& finished, bool& error)
{
    finished = error = false;

    if (size >= 4)
    {
        char *p = buffer + size - 4;
        if (p[0] == '\r' && p[1] == '\n' && p[2] == '\r' && p[3] == '\n')
            finished = true;

        if (!sameText(std::string(buffer, 4), "HTTP"))
            error = true;
    }
}

//-----------------------------------------------------------------------------

bool CustomHttpClient::parseResponseHeader(void *buffer, int size)
{
    bool result = true;
    MemoryStream stream;
    stream.write(buffer, size);

    StrList lines;
    lines.setLineBreak("\r\n");
    stream.setPosition(0);
    lines.loadFromStream(stream);

    if (lines.getCount() > 0)
    {
        std::string str = lines[0];
        if (sameText(str.substr(0, 7), "HTTP/1."))
            response_.setStatusLine(str);
        else
            result = false;
    }
    else
        result = false;

    if (result)
    {
        response_.getRawHeaders().clear();
        for (int i = 1; i < lines.getCount(); i++)
        {
            if (!lines[i].empty())
                response_.getRawHeaders().add(lines[i]);
        }
        response_.parseHeaders();
    }

    return result;
}

//-----------------------------------------------------------------------------

HTTP_NEXT_OP CustomHttpClient::processResponseHeader()
{
    HTTP_NEXT_OP result = HNO_EXIT;

    int statusCode = response_.getStatusCode();
    int responseDigit = statusCode / 100;

    lastKeepAlive_ = tcpClient_.isConnected() && response_.getKeepAlive();

    // Handle Redirects
    if ((responseDigit == 3 && statusCode != 304) ||
        (!response_.getLocation().empty() && statusCode != 201))
    {
        redirectCount_++;
        if (handleRedirects_)
            result = HNO_REDIRECT;
    }
    else if (responseDigit == 2)
    {
        result = HNO_RECV_CONTENT;
    }

    return result;
}

//-----------------------------------------------------------------------------

void CustomHttpClient::tcpDisconnect(bool force)
{
    if (!lastKeepAlive_ || force)
        tcpClient_.disconnect();
}

///////////////////////////////////////////////////////////////////////////////
// class HttpClient

HttpClient::HttpClient()
{
    // nothing
}

HttpClient::~HttpClient()
{
    // nothing
}

//-----------------------------------------------------------------------------

int HttpClient::get(const std::string& url, Stream *responseContent)
{
    return executeHttpAction(HMT_GET, url, NULL, responseContent);
}

//-----------------------------------------------------------------------------

int HttpClient::post(const std::string& url, Stream *requestContent, Stream *responseContent)
{
    ASSERT_X(requestContent != NULL);
    return executeHttpAction(HMT_POST, url, requestContent, responseContent);
}

//-----------------------------------------------------------------------------

int HttpClient::downloadFile(const std::string& url, const std::string& localFileName)
{
    forceDirectories(extractFilePath(localFileName));
    FileStream fs;
    if (!fs.open(localFileName, FM_CREATE | FM_SHARE_DENY_WRITE))
        return EC_HTTP_CANNOT_CREATE_FILE;

    int result = get(url, &fs);
    if (result != EC_HTTP_SUCCESS)
        deleteFile(localFileName);
    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::requestFile(const std::string& url)
{
    bool canRecvContent;
    int result = executeHttpRequest(HMT_GET, url, NULL, NULL, true, canRecvContent);
    if (result != EC_HTTP_SUCCESS || !canRecvContent)
        tcpDisconnect(true);
    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::receiveFile(void *buffer, int size, int timeoutMSecs)
{
    return tcpClient_.getConnection().recvBuffer(buffer, size, true, timeoutMSecs);
}

//-----------------------------------------------------------------------------

int HttpClient::executeHttpAction(HTTP_METHOD_TYPE httpMethod, const std::string& url,
    Stream *requestContent, Stream *responseContent)
{
    class AutoFinalizer
    {
    private:
        HttpClient& owner_;
    public:
        AutoFinalizer(HttpClient& owner) : owner_(owner) {}
        ~AutoFinalizer() { owner_.tcpDisconnect(); }
    } finalizer(*this);

    bool needRecvContent = (responseContent != NULL);
    bool canRecvContent = false;

    int result = executeHttpRequest(httpMethod, url, requestContent,
        responseContent, needRecvContent, canRecvContent);

    if (result == EC_HTTP_SUCCESS && needRecvContent)
        result = recvResponseContent();

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::executeHttpRequest(HTTP_METHOD_TYPE httpMethod, const std::string& url,
    Stream *requestContent, Stream *responseContent, bool needRecvContent,
    bool& canRecvContent)
{
    int result = EC_HTTP_SUCCESS;

    std::string newUrl(url);
    INT64 reqStreamPos = (requestContent? requestContent->getPosition() : 0);
    INT64 resStreamPos = (responseContent? responseContent->getPosition() : 0);

    canRecvContent = false;
    redirectCount_ = 0;

    while (true)
    {
        result = beforeRequest(httpMethod, newUrl, requestContent, responseContent,
            reqStreamPos, resStreamPos);

        if (result == EC_HTTP_SUCCESS) result = tcpConnect();
        if (result == EC_HTTP_SUCCESS) result = sendRequestHeader();
        if (result == EC_HTTP_SUCCESS) result = sendRequestContent();
        if (result == EC_HTTP_SUCCESS) result = recvResponseHeader();

        if (result == EC_HTTP_SUCCESS)
        {
            HTTP_NEXT_OP nextOp = processResponseHeader();

            if (nextOp == HNO_REDIRECT)
            {
                newUrl = response_.getLocation();
            }
            else if (nextOp == HNO_RECV_CONTENT)
            {
                canRecvContent = true;
                break;
            }
            else
                break;
        }
        else
            break;
    }

    if (result == EC_HTTP_SUCCESS && needRecvContent && !canRecvContent)
        result = EC_HTTP_CANNOT_RECV_CONTENT;

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::tcpConnect()
{
    if (!lastKeepAlive_)
        tcpClient_.disconnect();

    if (tcpClient_.isConnected())
    {
        InetAddress inetAddr = getInetAddrFromUrl(url_);
        if (tcpClient_.getConnection().getPeerAddr() != inetAddr)
            tcpClient_.disconnect();
    }

    if (!tcpClient_.isConnected())
    {
        InetAddress inetAddr = getInetAddrFromUrl(url_);
        int state = tcpClient_.asyncConnect(ipToString(inetAddr.ip),
            inetAddr.port, options_.tcpConnectTimeout);

        if (state != ACS_CONNECTED)
            return EC_HTTP_SOCKET_ERROR;
    }

    if (tcpClient_.isConnected())
    {
        SOCKET handle = tcpClient_.getConnection().getSocket().getHandle();
        int timeout = options_.socketOpTimeout;
        ::setsockopt(handle, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
        ::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    }

    return EC_HTTP_SUCCESS;
}

//-----------------------------------------------------------------------------

int HttpClient::sendRequestHeader()
{
    int result = EC_HTTP_SUCCESS;
    Buffer buffer;
    request_.makeRequestHeaderBuffer(buffer);

    int r = tcpClient_.getConnection().sendBuffer(buffer.data(), buffer.getSize(),
        true, options_.sendReqHeaderTimeout);

    if (r != buffer.getSize())
        result = EC_HTTP_SOCKET_ERROR;

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::sendRequestContent()
{
    int result = EC_HTTP_SUCCESS;

    Stream *stream = request_.getContentStream();
    if (!stream) return result;

    const int BLOCK_SIZE = 1024*64;

    Buffer buffer(BLOCK_SIZE);

    while (true)
    {
        int readSize = stream->read(buffer.data(), BLOCK_SIZE);
        if (readSize > 0)
        {
            int r = tcpClient_.getConnection().sendBuffer(buffer.data(), readSize,
                true, options_.sendReqContBlockTimeout);

            if (r < 0)
            {
                result = EC_HTTP_SOCKET_ERROR;
                break;
            }
            else if (r != readSize)
            {
                result = EC_HTTP_SEND_TIMEOUT;
                break;
            }
        }

        if (readSize < BLOCK_SIZE) break;
    }

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::recvResponseHeader()
{
    const int RECV_TIMEOUT = options_.recvResHeaderTimeout;

    int result = EC_HTTP_SUCCESS;
    UINT64 startTicks = getCurTicks();

    do
    {
        MemoryStream stream;
        char ch;

        while (true)
        {
            int remainTimeout = max(0, RECV_TIMEOUT - (int)getTickDiff(startTicks, getCurTicks()));

            int r = tcpClient_.getConnection().recvBuffer(&ch, sizeof(ch), true, remainTimeout);
            if (r < 0)
            {
                result = EC_HTTP_SOCKET_ERROR;
                break;
            }
            else if (r > 0)
            {
                stream.write(&ch, sizeof(ch));

                bool finished, error;
                checkResponseHeader(stream.getMemory(), (int)stream.getSize(), finished, error);
                if (error)
                {
                    result = EC_HTTP_RESPONSE_TEXT_ERROR;
                    break;
                }
                if (finished)
                    break;
            }

            if (getTickDiff(startTicks, getCurTicks()) > (UINT64)RECV_TIMEOUT)
            {
                result = EC_HTTP_RECV_TIMEOUT;
                break;
            }
        }

        if (result == EC_HTTP_SUCCESS)
        {
            if (!parseResponseHeader(stream.getMemory(), (int)stream.getSize()))
                result = EC_HTTP_RESPONSE_TEXT_ERROR;
        }
    }
    while (response_.getStatusCode() == 100 && result == EC_HTTP_SUCCESS);

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::recvResponseContent()
{
    int result = EC_HTTP_SUCCESS;
    if (!response_.getContentStream()) return result;

    if (lowerCase(response_.getTransferEncoding()).find("chunked") != std::string::npos)
    {
        while (true)
        {
            int timeout = options_.recvResContBlockTimeout;
            UINT chunkSize = 0;
            result = readChunkSize(chunkSize, timeout);
            if (result != EC_HTTP_SUCCESS)
                break;

            if (chunkSize != 0)
            {
                result = readStream(*response_.getContentStream(), chunkSize, timeout);
                if (result != EC_HTTP_SUCCESS)
                    break;

                std::string crlf;
                result = readLine(crlf, timeout);
                if (result != EC_HTTP_SUCCESS)
                    break;
            }
            else
                break;
        }
    }
    else
    {
        INT64 contentLength = response_.getContentLength();
        if (contentLength <= 0)
            result = EC_HTTP_CONTENT_LENGTH_ERROR;

        if (result == EC_HTTP_SUCCESS)
        {
            const int BLOCK_SIZE = 1024*64;

            INT64 remainSize = contentLength;
            Buffer buffer(BLOCK_SIZE);

            while (remainSize > 0)
            {
                int blockSize = (int)min(remainSize, (INT64)BLOCK_SIZE);
                int recvSize = tcpClient_.getConnection().recvBuffer(buffer.data(),
                    blockSize, true, options_.recvResContBlockTimeout);

                if (recvSize < 0)
                {
                    result = EC_HTTP_SOCKET_ERROR;
                    break;
                }

                remainSize -= recvSize;
                response_.getContentStream()->writeBuffer(buffer.data(), recvSize);
            }
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::readLine(std::string& line, int timeout)
{
    int result = EC_HTTP_SUCCESS;
    UINT64 startTicks = getCurTicks();
    MemoryStream stream;
    char ch;

    while (true)
    {
        int remainTimeout = max(0, timeout - (int)getTickDiff(startTicks, getCurTicks()));

        int r = tcpClient_.getConnection().recvBuffer(&ch, sizeof(ch), true, remainTimeout);
        if (r < 0)
        {
            result = EC_HTTP_SOCKET_ERROR;
            break;
        }
        else if (r > 0)
        {
            stream.write(&ch, sizeof(ch));

            char *buffer = stream.getMemory();
            int size = (int)stream.getSize();
            bool finished = false;

            if (size >= 2)
            {
                char *p = buffer + size - 2;
                if (p[0] == '\r' && p[1] == '\n')
                    finished = true;
            }

            if (finished)
                break;
        }

        if (getTickDiff(startTicks, getCurTicks()) > (UINT64)timeout)
        {
            result = EC_HTTP_RECV_TIMEOUT;
            break;
        }
    }

    if (result == EC_HTTP_SUCCESS)
    {
        if (stream.getSize() > 0)
            line.assign(stream.getMemory(), (int)stream.getSize());
        else
            line.clear();
    }

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::readChunkSize(UINT& chunkSize, int timeout)
{
    std::string line;
    int result = readLine(line, timeout);
    if (result == EC_HTTP_SUCCESS)
    {
        std::string::size_type pos = line.find(';');
        if (pos != std::string::npos)
            line = line.substr(0, pos);
        chunkSize = (UINT)strtol(line.c_str(), NULL, 16);
    }

    return result;
}

//-----------------------------------------------------------------------------

int HttpClient::readStream(Stream& stream, int bytes, int timeout)
{
    const int BLOCK_SIZE = 1024*64;

    int result = EC_HTTP_SUCCESS;
    UINT64 startTicks = getCurTicks();
    INT64 remainSize = bytes;
    Buffer buffer(BLOCK_SIZE);

    while (remainSize > 0)
    {
        int blockSize = (int)min(remainSize, (INT64)BLOCK_SIZE);
        int remainTimeout = max(0, timeout - (int)getTickDiff(startTicks, getCurTicks()));
        int recvSize = tcpClient_.getConnection().recvBuffer(
            buffer.data(), blockSize, true, remainTimeout);

        if (recvSize < 0)
        {
            result = EC_HTTP_SOCKET_ERROR;
            break;
        }

        remainSize -= recvSize;
        stream.writeBuffer(buffer.data(), recvSize);

        if (getTickDiff(startTicks, getCurTicks()) > (UINT64)timeout)
        {
            result = EC_HTTP_RECV_TIMEOUT;
            break;
        }
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class HttpServer

HttpServer::HttpServer(std::shared_ptr<IoService> service, WORD port) : m_TcpServer(service,this, port)
{
}

HttpServer::~HttpServer()
{
    // nothing
}

void HttpServer::open()
{
	m_TcpServer.open();
}

void HttpServer::close()
{
	m_TcpServer.close();
}

//-----------------------------------------------------------------------------

void HttpServer::onTcpConnected(const TcpConnectionPtr& connection)
{
    connCount_.increment();

    if (options_.maxConnectionCount >= 0 && getConnCount() > options_.maxConnectionCount)
    {
        connection->shutdown();
        return;
    }

    connection->setContext(ConnContextPtr(new ConnContext()));
    connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT, options_.recvLineTimeout);
}

//-----------------------------------------------------------------------------

void HttpServer::onTcpDisconnected(const TcpConnectionPtr& connection)
{
    connCount_.decrement();
}

//-----------------------------------------------------------------------------

void HttpServer::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
    int packetSize, const Context& context)
{
    ConnContextPtr connContext = connection->getContext().AnyCast<ConnContextPtr>();

    while (true)
    {
        switch (connContext->recvReqState)
        {
        case RRS_RECVING_REQ_LINE:
            {
                std::string line((const char*)packetBuffer, packetSize);
                if (connContext->httpRequest.setRequestLine(line))
                {
                    connContext->recvReqState = RRS_RECVING_REQ_HEADERS;
                    connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT, options_.recvLineTimeout);
                }
                else
                    connection->shutdown();
                break;
            }

        case RRS_RECVING_REQ_HEADERS:
            {
                std::string line((const char*)packetBuffer, packetSize);
                line = trimString(line);

                if (!line.empty())
                {
                    connContext->httpRequest.getRawHeaders().add(line);
                    connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT, options_.recvLineTimeout);
                }
                else
                {
                    // recved '\r\n\r\n'.
                    connContext->recvReqState = RRS_RECVING_CONTENT;
                    connContext->httpRequest.parseHeaders();

                    if (connContext->httpRequest.getContentLength() > 0)
                        connection->recv(ANY_PACKET_SPLITTER, EMPTY_CONTEXT, options_.recvContentTimeout);
                    else
                    {
                        connContext->recvReqState = RRS_COMPLETE;
                        continue;
                    }
                }

                break;
            }

        case RRS_RECVING_CONTENT:
            {
                INT64 contentLength = connContext->httpRequest.getContentLength();
                if (connContext->reqContentStream.getSize() >= contentLength)
                {
                    connContext->recvReqState = RRS_COMPLETE;
                    connContext->httpRequest.getContentStream()->setPosition(0);
                }
                else
                {
                    connContext->reqContentStream.write(packetBuffer, packetSize);
                    connection->recv(ANY_PACKET_SPLITTER, EMPTY_CONTEXT, options_.recvContentTimeout);
                }

                break;
            }

        case RRS_COMPLETE:
            {
                if (onHttpSession_)
                    onHttpSession_(connContext->httpRequest, connContext->httpResponse);

                connContext->sendResState = SRS_SENDING_RES_HEADERS;

                if (connContext->httpResponse.getStatusLine().empty())
                    connContext->httpResponse.setStatusCode(200);

                Stream *contentStream = connContext->httpResponse.getContentStream();
                if (contentStream != NULL)
                {
                    contentStream->setPosition(0);
                    connContext->httpResponse.setContentLength(contentStream->getSize());
                }

                Buffer buffer;
                connContext->httpResponse.makeResponseHeaderBuffer(buffer);
                connection->send(buffer.data(), buffer.getSize(), EMPTY_CONTEXT, options_.sendResponseHeaderTimeout);

                break;
            }

        default:
            break;
        }

        break;
    }
}

//-----------------------------------------------------------------------------

void HttpServer::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
    ConnContextPtr connContext = connection->getContext().AnyCast<ConnContextPtr>();

    switch (connContext->sendResState)
    {
    case SRS_SENDING_RES_HEADERS:
    case SRS_SENDING_CONTENT:
        {
            connContext->sendResState = SRS_SENDING_CONTENT;

            Stream *contentStream = connContext->httpResponse.getContentStream();

            const int SEND_BLOCK_SIZE = 1024*64;
            Buffer buffer(SEND_BLOCK_SIZE);

            int readSize = contentStream->read(buffer.data(), buffer.getSize());
            if (readSize > 0)
                connection->send(buffer.data(), buffer.getSize(), EMPTY_CONTEXT, options_.sendContentBlockTimeout);
            else
            {
                connContext->sendResState = SRS_COMPLETE;
                connection->disconnect();
            }

            break;
        }

    case SRS_COMPLETE:
        break;

    default:
        break;
    }
}


//////////////////////////////////////////////////////////////////////////
//WebSocket Session

/*
					0                   1                   2                   3
					0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
					+-+-+-+-+-------+-+-------------+-------------------------------+
					|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
					|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
					|N|V|V|V|       |S|             |   (if payload len==126/127)   |
					| |1|2|3|       |K|             |                               |
					+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
					|     Extended payload length continued, if payload len == 127  |
					+ - - - - - - - - - - - - - - - +-------------------------------+
					|                               |Masking-key, if MASK set to 1  |
					+-------------------------------+-------------------------------+
					| Masking-key (continued)       |          Payload Data         |
					+-------------------------------- - - - - - - - - - - - - - - - +
					:                     Payload Data continued ...                :
					+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
					|                     Payload Data continued ...                |
					+---------------------------------------------------------------+

					opcode:
					*  %x0 denotes a continuation frame
					*  %x1 denotes a text frame
					*  %x2 denotes a binary frame
					*  %x3-7 are reserved for further non-control frames
					*  %x8 denotes a connection close
					*  %x9 denotes a ping
					*  %xA denotes a pong
					*  %xB-F are reserved for further control frames

					Payload length:  7 bits, 7+16 bits, or 7+64 bits

					Masking-key:  0 or 4 bytes
*/

/// 将从client发过来的数据帧进行解码
/// inbuf      : 接收到的client发送的数据
/// insize     : 接收到的数据大小
/// outbuf     : 解码缓冲区
/// return     : WsFrameType
WsFrameType decodeFrame(const char* inbuf, int inlength, std::vector<char>* outbuf)
{
	if (inlength < 3) return WS_INCOMPLETE_FRAME;

	const unsigned char* inp = (const unsigned char*)(inbuf);

	unsigned char msg_opcode = inp[0] & 0x0F;
	unsigned char msg_fin = (inp[0] >> 7) & 0x01;
	unsigned char msg_masked = (inp[1] >> 7) & 0x01;

	int payload_length = 0;
	int pos = 2;
	int length_field = inp[1] & (~0x80);
	unsigned int mask = 0;

	if (length_field <= PAYLOAD_SIZE_BASIC)
	{
		payload_length = length_field;
	}
	else if (length_field == 126)  //msglen is 16bit!
	{
		payload_length = (inp[2] << 8) + inp[3];
		pos += 2;
	}
	else if (length_field == 127)  //msglen is 64bit!
	{
		payload_length = ((unsigned long long)inp[2] << 56) + ((unsigned long long)inp[3] << 48) +
			((unsigned long long)inp[4] << 40) + ((unsigned long long)inp[5] << 32) +
			((unsigned long long)inp[6] << 24) + ((unsigned long long)inp[7] << 16) +
			((unsigned long long)inp[8] << 8) + ((unsigned long long)inp[9] << 0);
		payload_length = payload_length / 2;
		pos += 8;
	}

	if (inlength < payload_length + pos)
	{
		return WS_INCOMPLETE_FRAME;
	}
	else if (payload_length > (int)outbuf->size())
	{
		outbuf->resize(payload_length + 1);
	}

	if (outbuf->size() > 0)
	{
		if (msg_masked)
		{
			mask = *((unsigned int*)(inp + pos));
			pos += 4;
			memcpy(&*outbuf->begin(), (void*)(inp + pos), payload_length);
			// unmask data:
			char* c = &*outbuf->begin();
			for (int i = 0; i < payload_length; i++)
			{
				c[i] = c[i] ^ ((unsigned char*)(&mask))[i % 4];
			}
		}
		(*outbuf)[payload_length] = 0;
	}

	if (msg_opcode == 0x0) return (msg_fin) ? WS_TEXT_FRAME : WS_INCOMPLETE_TEXT_FRAME; // continuation frame ?
	if (msg_opcode == 0x1) return (msg_fin) ? WS_TEXT_FRAME : WS_INCOMPLETE_TEXT_FRAME;
	if (msg_opcode == 0x2) return (msg_fin) ? WS_BINARY_FRAME : WS_INCOMPLETE_BINARY_FRAME;
	if (msg_opcode == 0x8) return WS_CLOSE_FRAME;
	if (msg_opcode == 0x9) return WS_PING_FRAME;
	if (msg_opcode == 0xA) return WS_PONG_FRAME;

	return WS_ERROR_FRAME;
}

/// 将发回给client的数据进行编码
/// msg        : 发回给client的数据
/// msglen     : 发回的数据大小
/// outbuf     : 编码缓冲区
/// outsize    : 编码缓冲区大小（建议outsize > insize + 10）
int encodeFrame(WsFrameType frame_type, const char* msg, int msg_length, char* outbuf, int bufsize)
{
	//std::cout << "makeFrame : " <<  frame_type << "\t" << msg << "\t" << msg_length << "\n";
	int pos = 0;
	int size = msg_length;
	outbuf[pos++] = (unsigned char)frame_type; // text frame
	if (size <= PAYLOAD_SIZE_BASIC)  // 125
	{
		outbuf[pos++] = size;
	}
	else if (size <= PAYLOAD_SIZE_EXTENDED)   // 65535
	{
		outbuf[pos++] = 126; //16 bit length
		outbuf[pos++] = (size >> 8) & 0xFF; // rightmost first
		outbuf[pos++] = size & 0xFF;
	}
	else  // >2^16-1
	{
		outbuf[pos++] = 127; //64 bit length

							 // write 8 bytes length (significant first)
							 // since msg_length is int it can be no longer than 4 bytes = 2^32-1
							 // padd zeroes for the first 4 bytes
		for (int i = 3; i >= 0; i--)
		{
			outbuf[pos++] = 0;
		}
		// write the actual 32bit msg_length in the next 4 bytes
		for (int i = 3; i >= 0; i--)
		{
			outbuf[pos++] = ((size >> 8 * i) & 0xFF);
		}
	}
	memcpy((void*)(outbuf + pos), msg, size);
	//printf("makeFrame [%s]\n", outbuf);
	//printf("makeFrame [%s]\n", outbuf + 2);
	return (size + pos);
}


void WebSocketPacketSplitter(const char *data, int bytes, int& retrieveBytes)
{
	retrieveBytes = 0;

	if (bytes < 3)
		return ;

	const unsigned char* inp = (const unsigned char*)(data);

	unsigned char msg_opcode = inp[0] & 0x0F;
	unsigned char msg_fin = (inp[0] >> 7) & 0x01;
	unsigned char msg_masked = (inp[1] >> 7) & 0x01;

	int payload_length = 0;
	int pos = 2;
	int length_field = inp[1] & (~0x80);
	unsigned int mask = 0;

	if (length_field <= PAYLOAD_SIZE_BASIC)
	{
		payload_length = length_field;
	}
	else if (length_field == 126)  //msglen is 16bit!
	{
		payload_length = (inp[2] << 8) + inp[3];
		pos += 2;
	}
	else if (length_field == 127)  //msglen is 64bit!
	{
		payload_length = ((unsigned long long)inp[2] << 56) + ((unsigned long long)inp[3] << 48) +
			((unsigned long long)inp[4] << 40) + ((unsigned long long)inp[5] << 32) +
			((unsigned long long)inp[6] << 24) + ((unsigned long long)inp[7] << 16) +
			((unsigned long long)inp[8] << 8) + ((unsigned long long)inp[9] << 0);
		payload_length = payload_length / 2;
		pos += 8;
	}

	if (bytes < payload_length + pos)
	{
		return;
	}

	retrieveBytes = payload_length + pos + 4;
}

// 无论收到多少字节都立即获取的分包器
const PacketSplitter WEBSOCKET_PACKET_SPLITTER = &WebSocketPacketSplitter;

WebSocketServer::WebSocketServer(std::shared_ptr<IoService> service,
								TcpCallbacks* _callback,
								WORD port) : 
	m_TcpServer(service,this,port),
	m_callback(_callback)
{
}

WebSocketServer::~WebSocketServer()
{



}

void WebSocketServer::open()
{
	m_TcpServer.open();
}
void WebSocketServer::close()
{
	m_TcpServer.close();
}

// 接受了一个新的TCP连接
void WebSocketServer::onTcpConnected(const TcpConnectionPtr& connection)
{
	INFO_LOG("new websocket connected %x", connection.get());
	connection->setContext(WebConnContextPtr(new WebConnContext()));
	connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT, DEF_HEART_BEAT_TIME);

}

// 断开了一个TCP连接
void WebSocketServer::onTcpDisconnected(const TcpConnectionPtr& connection)
{
	if (m_callback)
	{
		INFO_LOG("websocket disconnected %x", connection.get());
		m_callback->onTcpDisconnected(connection);
	}
}
// TCP连接上的一个接收任务已完成
void WebSocketServer::onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
	int packetSize, const Context& context)
{
	if (!IsHandSharked(connection))
	{
		bool res = handshark(connection, packetBuffer, packetSize);
		if (!res)
		{
			connection->recv(LINE_PACKET_SPLITTER, EMPTY_CONTEXT, DEF_HEART_BEAT_TIME);
		}
		else
		{
			if (m_callback)
			{
				m_callback->onTcpConnected(connection);
			}

			connection->recv(WEBSOCKET_PACKET_SPLITTER, EMPTY_CONTEXT, DEF_HEART_BEAT_TIME);
		}
	}
	else
	{
		WebConnContextPtr connContext = connection->getContext().AnyCast<WebConnContextPtr>();

		std::vector<char> outbuf;
		WsFrameType type = decodeFrame((const char*)packetBuffer, packetSize, &outbuf);

		connContext->buff.insert(connContext->buff.end(), outbuf.begin(), outbuf.end());

		switch (type)
		{
		case WS_TEXT_FRAME:
		case WS_BINARY_FRAME:
			{
				if (m_callback && connContext->buff.size() > 0)
				{
					m_callback->onTcpRecvComplete(connection, 
						reinterpret_cast<char*>(connContext->buff.data()),
						connContext->buff.size(),
						context
					);
				}
				connContext->buff.clear();
			}
			break;
		case WS_CLOSE_FRAME:
			{
				connection->shutdown();
				connContext->buff.clear();
			}
			return;
		case WS_INCOMPLETE_TEXT_FRAME:
		case WS_INCOMPLETE_BINARY_FRAME:
			break;
		default:
			WARN_LOG("No this [%d] opcode handler", type);
			break;
		}

		connection->recv(WEBSOCKET_PACKET_SPLITTER, EMPTY_CONTEXT, DEF_HEART_BEAT_TIME);
	}
}


// TCP连接上的一个发送任务已完成
void WebSocketServer::onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context)
{
	if (m_callback)
	{
		m_callback->onTcpSendComplete(connection, context);
	}
}

bool		WebSocketServer::handshark(const TcpConnectionPtr& connection,void *packetBuffer, int packetSize)
{
	WebConnContextPtr connContext = connection->getContext().AnyCast<WebConnContextPtr>();

	std::string line((const char*)packetBuffer, packetSize);
	line = trimString(line);

	if (!line.empty())
	{
		StrList parts;
		splitString(line, ':', parts, true);
		if (parts.getCount() == 2 && !parts[0].empty())
		{
			connContext->argStr.add(parts[0], parts[1]);
		}
	}
	else
	{
		parse_str(connection);
		connContext->handshaked = true;
		return true;
	}

	return false;
}

bool WebSocketServer::IsHandSharked(const TcpConnectionPtr& connection)
{
	WebConnContextPtr connContext = connection->getContext().AnyCast<WebConnContextPtr>();

	return connContext->handshaked;
}


void	WebSocketServer::parse_str(const TcpConnectionPtr& connection )
{
	WebConnContextPtr connContext = connection->getContext().AnyCast<WebConnContextPtr>();

	StrList strList;

	strList.add(formatString("HTTP/1.1 101 Switching Protocols"));
	strList.add(formatString("Connection: upgrade"));

	std::string server_key;
	bool res = connContext->argStr.getValue("Sec-WebSocket-Key", server_key);
	server_key += WEBSOCKET_MAGIC_KEY;
	
	SHA1 sha;
	unsigned int message_digest[5];
	sha.Reset();
	sha << server_key.c_str();

	sha.Result(message_digest);
	for (int i = 0; i < 5; i++) {
		message_digest[i] = htonl(message_digest[i]);
	}
	server_key = base64_encode(reinterpret_cast<const unsigned char*>(message_digest), 20);

	strList.add(formatString("Sec-WebSocket-Accept:%s", server_key.c_str()));
	
	strList.add(formatString("Upgrade: websocket\r\n"));

	std::string msg = strList.getText();

	connection->send(msg.data(),msg.size());
}

void	WebSocketServer::sendText(const TcpConnectionPtr& conn, const char* data, size_t size)
{
	senddata(conn, data, size, WS_TEXT_FRAME);
}
void	WebSocketServer::sendBinary(const TcpConnectionPtr& conn, const char* data, size_t size)
{
	senddata(conn, data, size, WS_BINARY_FRAME);
}

void	WebSocketServer::senddata(const TcpConnectionPtr& conn, const char* data, size_t size, WsFrameType type)
{
	LOCAL_CACHE_DATA(cache, size);
	int encodesize = encodeFrame(type, data, size, cache, size);
	conn->send(cache, encodesize);
}


void	WebSocketServer::closeTcpConnection(const TcpConnectionPtr& conn, WsCloseReason code)
{
	size_t len = 0;
	char buf[128];
	unsigned short code_be = htons(code);
	memcpy(buf, &code_be, 2);
	len += 2;
	senddata(conn, buf, len, WS_CLOSE_FRAME);
}