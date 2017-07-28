#include "UtilClass.h"
#include "LogManager.h"

//-----------------------------------------------------------------------------
LocalCacheData::LocalCacheData(int size, const std::string& filename, UINT32 line) :
	buf(localbuff)
{
	memset(localbuff, 0, sizeof(localbuff));

	if (size > LOCAL_CACHE_DATA_SIZE)
	{
		buf = new char[size];

		if (NULL == buf)
		{
			WARN_LOG("Local cache new data big size err %s:%d !!!", filename.c_str(), line);
			ThrowMemoryException();
		}
	}
}

LocalCacheData::~LocalCacheData()
{
	if (buf != localbuff)
	{
		delete[] buf;
	}
}


///////////////////////////////////////////////////////////////////////////////
// class SeqNumberAlloc

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   startId - 起始序列号
//-----------------------------------------------------------------------------
SeqNumberAlloc::SeqNumberAlloc(UINT64 startId)
{
	currentId_ = startId;
}

//-----------------------------------------------------------------------------
// 描述: 返回一个新分配的ID
//-----------------------------------------------------------------------------
UINT64 SeqNumberAlloc::allocId()
{
	AutoLocker locker(mutex_);
	return currentId_++;
}



///////////////////////////////////////////////////////////////////////////////
// class SignalMasker

#ifdef _COMPILER_LINUX

SignalMasker::SignalMasker(bool isAutoRestore) :
	isBlock_(false),
	isAutoRestore_(isAutoRestore)
{
	sigemptyset(&oldSet_);
	sigemptyset(&newSet_);
}

//-----------------------------------------------------------------------------

SignalMasker::~SignalMasker()
{
	if (isAutoRestore_) restore();
}

//-----------------------------------------------------------------------------
// 描述: 设置 Block/UnBlock 操作所需的信号集合
//-----------------------------------------------------------------------------
void SignalMasker::setSignals(int sigCount, va_list argList)
{
	sigemptyset(&newSet_);
	for (int i = 0; i < sigCount; i++)
		sigaddset(&newSet_, va_arg(argList, int));
}

//-----------------------------------------------------------------------------

void SignalMasker::setSignals(int sigCount, ...)
{
	va_list argList;
	va_start(argList, sigCount);
	setSignals(sigCount, argList);
	va_end(argList);
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中添加 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::block()
{
	sigProcMask(SIG_BLOCK, &newSet_, &oldSet_);
	isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中解除 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::unBlock()
{
	sigProcMask(SIG_UNBLOCK, &newSet_, &oldSet_);
	isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 将进程阻塞信号集恢复为 Block/UnBlock 之前的状态
//-----------------------------------------------------------------------------
void SignalMasker::restore()
{
	if (isBlock_)
	{
		sigProcMask(SIG_SETMASK, &oldSet_, NULL);
		isBlock_ = false;
	}
}

//-----------------------------------------------------------------------------

int SignalMasker::sigProcMask(int how, const sigset_t *newSet, sigset_t *oldSet)
{
	int result;
	if ((result = sigprocmask(how, newSet, oldSet)) < 0)
		ThrowException(strerror(errno));

	return result;
}

#endif


///////////////////////////////////////////////////////////////////////////////
// class Url

Url::Url(const std::string& url)
{
	setUrl(url);
}

Url::Url(const Url& src)
{
	(*this) = src;
}

//-----------------------------------------------------------------------------

void Url::clear()
{
	protocol_.clear();
	host_.clear();
	port_.clear();
	path_.clear();
	fileName_.clear();
	bookmark_.clear();
	userName_.clear();
	password_.clear();
	params_.clear();
}

//-----------------------------------------------------------------------------

Url& Url::operator = (const Url& rhs)
{
	if (this == &rhs) return *this;

	protocol_ = rhs.protocol_;
	host_ = rhs.host_;
	port_ = rhs.port_;
	path_ = rhs.path_;
	fileName_ = rhs.fileName_;
	bookmark_ = rhs.bookmark_;
	userName_ = rhs.userName_;
	password_ = rhs.password_;
	params_ = rhs.params_;

	return *this;
}

//-----------------------------------------------------------------------------

std::string Url::getUrl() const
{
	const char SEP_CHAR = '/';
	std::string result;

	if (!protocol_.empty())
		result = protocol_ + "://";

	if (!userName_.empty())
	{
		result += userName_;
		if (!password_.empty())
			result = result + ":" + password_;
		result += "@";
	}

	result += host_;

	if (!port_.empty())
	{
		if (sameText(protocol_, "HTTP"))
		{
			if (port_ != "80")
				result = result + ":" + port_;
		}
		else if (sameText(protocol_, "HTTPS"))
		{
			if (port_ != "443")
				result = result + ":" + port_;
		}
		else if (sameText(protocol_, "FTP"))
		{
			if (port_ != "21")
				result = result + ":" + port_;
		}
	}

	// path and filename
	std::string str = path_;
	if (!str.empty() && str[str.length() - 1] != SEP_CHAR)
		str += SEP_CHAR;
	str += fileName_;

	if (!str.empty())
	{
		if (!result.empty() && str[0] == SEP_CHAR) str.erase(0, 1);
		if (!host_.empty() && result[result.length() - 1] != SEP_CHAR)
			result += SEP_CHAR;
		result += str;
	}

	if (!params_.empty())
		result = result + "?" + params_;

	if (!bookmark_.empty())
		result = result + "#" + bookmark_;

	return result;
}

//-----------------------------------------------------------------------------

std::string Url::getUrl(UINT parts)
{
	Url url(*this);

	if (!(parts & URL_PROTOCOL)) url.setProtocol("");
	if (!(parts & URL_HOST)) url.setHost("");
	if (!(parts & URL_PORT)) url.setPort("");
	if (!(parts & URL_PATH)) url.setPath("");
	if (!(parts & URL_FILENAME)) url.setFileName("");
	if (!(parts & URL_BOOKMARK)) url.setBookmark("");
	if (!(parts & URL_USERNAME)) url.setUserName("");
	if (!(parts & URL_PASSWORD)) url.setPassword("");
	if (!(parts & URL_PARAMS)) url.setParams("");

	return url.getUrl();
}

//-----------------------------------------------------------------------------

void Url::setUrl(const std::string& value)
{
	clear();

	std::string url(value);
	if (url.empty()) return;

	// get the bookmark
	std::string::size_type pos = url.rfind('#');
	if (pos != std::string::npos)
	{
		bookmark_ = url.substr(pos + 1);
		url.erase(pos);
	}

	// get the parameters
	pos = url.find('?');
	if (pos != std::string::npos)
	{
		params_ = url.substr(pos + 1);
		url = url.substr(0, pos);
	}

	std::string buffer;
	pos = url.find("://");
	if (pos != std::string::npos)
	{
		protocol_ = url.substr(0, pos);
		url.erase(0, pos + 3);
		// get the user name, password, host and the port number
		buffer = fetchStr(url, '/', true);
		// get username and password
		pos = buffer.find('@');
		if (pos != std::string::npos)
		{
			password_ = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			userName_ = fetchStr(password_, ':');
			if (userName_.empty())
				password_.clear();
		}
		// get the host and the port number
		std::string::size_type p1, p2;
		if ((p1 = buffer.find('[')) != std::string::npos &&
			(p2 = buffer.find(']')) != std::string::npos &&
			p2 > p1)
		{
			// this is for IPv6 Hosts
			host_ = fetchStr(buffer, ']');
			fetchStr(host_, '[');
			fetchStr(buffer, ':');
		}
		else
			host_ = fetchStr(buffer, ':', true);
		port_ = buffer;
		// get the path
		pos = url.rfind('/');
		if (pos != std::string::npos)
		{
			path_ = "/" + url.substr(0, pos + 1);
			url.erase(0, pos + 1);
		}
		else
			path_ = "/";
	}
	else
	{
		// get the path
		pos = url.rfind('/');
		if (pos != std::string::npos)
		{
			path_ = url.substr(0, pos + 1);
			url.erase(0, pos + 1);
		}
	}

	// get the filename
	fileName_ = url;
}

