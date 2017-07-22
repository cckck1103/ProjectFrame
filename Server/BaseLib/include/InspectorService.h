///////////////////////////////////////////////////////////////////////////////
// InspectorService.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _INSPECTOR_SERVICE_H_
#define _INSPECTOR_SERVICE_H_

#include "Options.h"
#include "UtilClass.h"
#include "Exceptions.h"
#include "TcpServer.h"
#include "BaseHttp.h"

///////////////////////////////////////////////////////////////////////////////
// classes

class ServerInspector;
class PredefinedInspector;

///////////////////////////////////////////////////////////////////////////////
// class ServerInspector

class ServerInspector
{
public:
    typedef std::function<std::string (const PropertyList& args, std::string& contentType)> OutputCallback;

    struct CommandItem
    {
    public:
        std::string category;
        std::string command;
        OutputCallback outputCallback;
        std::string help;
    public:
        CommandItem() {}
        CommandItem(const std::string& category, const std::string& command,
            const OutputCallback& outputCallback, const std::string& help)
        {
            this->category = category;
            this->command = command;
            this->outputCallback = outputCallback;
            this->help = help;
        }
    };

    typedef std::vector<CommandItem> CommandItems;

public:
	ServerInspector(std::shared_ptr<IoService> service,int serverPort);

    void add(
        const std::string& category,
        const std::string& command,
        const OutputCallback& outputCallback,
        const std::string& help);

    void add(const CommandItems& items);

	void start();
	void close();
private:
    std::string outputHelpPage();
    bool parseCommandUrl(const HttpRequest& request, std::string& category,
        std::string& command, PropertyList& argList);
    CommandItem* findCommandItem(const std::string& category, const std::string& command);

    void onHttpSession(const HttpRequest& request, HttpResponse& response);

private:
    typedef std::map<std::string, CommandItem> CommandList;    // <command, CommandItem>
    typedef std::map<std::string, CommandList> InspectInfo;    // <category, CommandList>

    HttpServer httpServer_;
    InspectInfo inspectInfo_;
    Mutex mutex_;
    std::auto_ptr<PredefinedInspector> predefinedInspector_;
};

///////////////////////////////////////////////////////////////////////////////
// class PredefinedInspector

class PredefinedInspector : noncopyable
{
public:
	ServerInspector::CommandItems getItems() const;
private:

#ifdef _COMPILER_WIN
    static std::string getBasicInfo(const PropertyList& argList, std::string& contentType);
#endif

#ifdef _COMPILER_LINUX
    static std::string getBasicInfo(const PropertyList& argList, std::string& contentType);
    static std::string getProcStatus(const PropertyList& argList, std::string& contentType);
    static std::string getOpenedFileCount(const PropertyList& argList, std::string& contentType);
    static std::string getThreadCount(const PropertyList& argList, std::string& contentType);
#endif
};

///////////////////////////////////////////////////////////////////////////////
#endif // _ISE_INSPECTOR_H_
