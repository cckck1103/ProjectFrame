///////////////////////////////////////////////////////////////////////////////
// 文件名称: InspectorService.cpp
// 功能描述: 服务器状态监视器
///////////////////////////////////////////////////////////////////////////////

#include "InspectorService.h"
#include "SysUtils.h"

#ifdef _COMPILER_WIN
#include "Psapi.h"
#pragma comment (lib, "psapi.lib")
#endif

///////////////////////////////////////////////////////////////////////////////
// class ServerInspector

ServerInspector::ServerInspector(std::shared_ptr<IoService> service,int serverPort) : 
	httpServer_(service,serverPort),
    predefinedInspector_(new PredefinedInspector())
{
    httpServer_.setHttpSessionCallback(std::bind(&ServerInspector::onHttpSession, this, std::placeholders::_1, std::placeholders::_2));
    add(predefinedInspector_->getItems());
}

//-----------------------------------------------------------------------------

void ServerInspector::add(const std::string& category, const std::string& command,
    const OutputCallback& outputCallback, const std::string& help)
{
    AutoLocker locker(mutex_);

    CommandItem item(category, command, outputCallback, help);
    inspectInfo_[category][command] = item;
}

//-----------------------------------------------------------------------------

void ServerInspector::add(const CommandItems& items)
{
    for (size_t i = 0; i < items.size(); ++i)
    {
        const CommandItem& item = items[i];
        add(item.category, item.command, item.outputCallback, item.help);
    }
}

void ServerInspector::start()
{
	httpServer_.open();
}
void ServerInspector::close()
{
	httpServer_.close();
}
//-----------------------------------------------------------------------------

std::string ServerInspector::outputHelpPage()
{
	std::string result;

    for (InspectInfo::iterator i = inspectInfo_.begin(); i != inspectInfo_.end(); ++i)
    {
		std::string category = i->first;
        CommandList& commandList = i->second;

        for (CommandList::iterator j = commandList.begin(); j != commandList.end(); ++j)
        {
			std::string command = j->first;
            CommandItem& commandItem = j->second;

			std::string help;
            if (!commandItem.help.empty())
                help = formatString("<span style=\"color:#cfcfcf\">(%s)</span>", commandItem.help.c_str());

			std::string path = formatString("/%s/%s", category.c_str(), command.c_str());
			std::string line = formatString("<a href=\"%s\">[%s]</a> %s",
                path.c_str(), path.c_str(), help.c_str());
            if (!result.empty()) result += "</br>";
            result += line;
        }

		result += "</br>";
    }

    return result;
}

//-----------------------------------------------------------------------------

// Example: /category/command?arg1=value&arg2=value
bool ServerInspector::parseCommandUrl(const HttpRequest& request,
	std::string& category, std::string& command, PropertyList& argList)
{
    bool result = false;
	std::string url = request.getUrl();
	std::string argStr;
    StrList strList;

    if (!url.empty() && url[0] == '/')
        url.erase(0, 1);

    // find the splitter char ('?')
	std::string::size_type argPos = url.find('?');
    bool hasArgs = (argPos != std::string::npos);
    if (hasArgs)
    {
        argStr = url.substr(argPos + 1);
        url.erase(argPos);
    }

    // parse the string before the '?'
    splitString(url, '/', strList, true);
    if (strList.getCount() >= 2)
    {
        category = strList[0];
        command = strList[1];
        result = true;
    }

    // parse the args
    if (result)
    {
        argList.clear();
        splitString(argStr, '&', strList, true);
        for (int i = 0; i < strList.getCount(); ++i)
        {
            StrList parts;
            splitString(strList[i], '=', parts, true);
            if (parts.getCount() == 2 && !parts[0].empty())
            {
                argList.add(parts[0], parts[1]);
            }
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

ServerInspector::CommandItem* ServerInspector::findCommandItem(
    const std::string& category, const std::string& command)
{
    CommandItem *result = NULL;

    InspectInfo::iterator i = inspectInfo_.find(category);
    if (i != inspectInfo_.end())
    {
        CommandList& commandList = i->second;
        CommandList::iterator j = commandList.find(command);
        if (j != commandList.end())
        {
            CommandItem& commandItem = j->second;
            result = &commandItem;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------

void ServerInspector::onHttpSession(const HttpRequest& request, HttpResponse& response)
{
    response.setCacheControl("no-cache");
    response.setPragma(response.getCacheControl());
    response.setContentType("text/plain");

    if (request.getUrl() == "/")
    {
        AutoLocker locker(mutex_);
		std::string s = outputHelpPage();

        response.setStatusCode(200);
        response.setContentType("text/html");
        response.getContentStream()->write(s.c_str(), static_cast<int>(s.length()));
    }
    else
    {
        AutoLocker locker(mutex_);
		std::string category, command;
        CommandItem *commandItem;
        PropertyList argList;

        if (parseCommandUrl(request, category, command, argList) &&
            (commandItem = findCommandItem(category, command)))
        {
			std::string contentType = response.getContentType();
			std::string s = commandItem->outputCallback(argList, contentType);

            response.setStatusCode(200);
            response.setContentType(contentType);
            response.getContentStream()->write(s.c_str(), static_cast<int>(s.length()));
        }
        else
        {
			std::string s = "Not Found";
            response.setStatusCode(404);
            response.getContentStream()->write(s.c_str(), static_cast<int>(s.length()));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// class PredefinedInspector

#ifdef _COMPILER_WIN

ServerInspector::CommandItems PredefinedInspector::getItems() const
{
    typedef ServerInspector::CommandItem CommandItem;
    typedef ServerInspector::CommandItems CommandItems;

	std::string category("process");
    CommandItems items;

    items.push_back(CommandItem(category, "basic_info", PredefinedInspector::getBasicInfo, "show the basic info."));

    return items;
}

std::string PredefinedInspector::getBasicInfo(const PropertyList& argList,
	std::string& contentType)
{
    contentType = "text/plain";

    DWORD procId = ::GetCurrentProcessId();
    HANDLE handle = ::OpenProcess(PROCESS_QUERY_INFORMATION, false, procId);
    size_t memorySize = 0;

    PROCESS_MEMORY_COUNTERS pmc = {0};
    pmc.cb = sizeof(pmc);
    if (::GetProcessMemoryInfo(handle, &pmc, pmc.cb))
        memorySize = pmc.WorkingSetSize;

    ::CloseHandle(handle);

    StrList strList;
    strList.add(formatString("path: %s", getAppExeName().c_str()));
    strList.add(formatString("pid: %u", procId));
    strList.add(formatString("memory: %s bytes", addThousandSep(memorySize).c_str()));

    return strList.getText();
}

#endif

#ifdef _COMPILER_LINUX

ServerInspector::CommandItems PredefinedInspector::getItems() const
{
    typedef ServerInspector::CommandItem CommandItem;
    typedef ServerInspector::CommandItems CommandItems;

	std::string category("proc");
    CommandItems items;

    items.push_back(CommandItem(category, "basic_info", PredefinedInspector::getBasicInfo, "show the basic info."));
    items.push_back(CommandItem(category, "status", PredefinedInspector::getProcStatus, "print /proc/self/status."));
    items.push_back(CommandItem(category, "opened_file_count", PredefinedInspector::getOpenedFileCount, "count /proc/self/fd."));
    items.push_back(CommandItem(category, "thread_count", PredefinedInspector::getThreadCount, "count /proc/self/task."));

    return items;
}

std::string PredefinedInspector::getBasicInfo(const PropertyList& argList,
	std::string& contentType)
{
    contentType = "text/plain";

    StrList strList;
    strList.add(formatString("path: %s", getAppExeName().c_str()));
    strList.add(formatString("pid: %d", ::getpid()));

    return strList.getText();
}

std::string PredefinedInspector::getProcStatus(const PropertyList& argList,
	std::string& contentType)
{
    contentType = "text/plain";

    StrList strList;
    if (strList.loadFromFile("/proc/self/status"))
        return strList.getText();
    else
        return "";
}

std::string PredefinedInspector::getOpenedFileCount(const PropertyList& argList,
	std::string& contentType)
{
    contentType = "text/plain";

    int count = 0;
    FileFindResult findResult;
    findFiles("/proc/self/fd/*", FA_ANY_FILE, findResult);
    for (size_t i = 0; i < findResult.size(); ++i)
    {
        const std::string& name = findResult[i].fileName;
        if (isInt64Str(name))
            ++count;
    }

    return intToStr(count);
}

std::string PredefinedInspector::getThreadCount(const PropertyList& argList,
	std::string& contentType)
{
    contentType = "text/plain";

    int count = 0;
    FileFindResult findResult;
    findFiles("/proc/self/task/*", FA_ANY_FILE, findResult);
    for (size_t i = 0; i < findResult.size(); ++i)
    {
        const std::string& name = findResult[i].fileName;
        if (isInt64Str(name))
            ++count;
    }

    return intToStr(count);
}

#endif