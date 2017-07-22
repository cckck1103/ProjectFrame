#ifndef _LOG_MANAGER_H
#define _LOG_MANAGER_H

#include "Options.h"
#include "GlobalDefs.h"
#include "UtilClass.h"
#include "SysUtils.h"
#include "Singleton.h"
#include "StreamClass.h"
#include "ServiceThread.h"
#include <deque>

///////////////////////////////////////////////////////////////////////////////
// class Logger - 日志类   异步写入日志文件   线程安全

enum LOG_LVL
{
	TRACE_LVL,
	DEBUG_LVL,
	INFO_LVL,
	WARN_LVL,
	ERR_LVL,
	FATAL_LVL,
};

///////////////////////////////////////////////////////////////////////////////
// 全局函数

#define TRACE_LOG(f,...)			Logger::instance().trace(__FILE__, __LINE__,f,##__VA_ARGS__)
#define DEBUG_LOG(f,...)            Logger::instance().debug(__FILE__, __LINE__,f,##__VA_ARGS__)
#define INFO_LOG(f,...)  			Logger::instance().info(__FILE__, __LINE__,f,##__VA_ARGS__)
#define WARN_LOG(f,...)   			Logger::instance().warn(__FILE__, __LINE__,f,##__VA_ARGS__)
#define ERROR_LOG(f,...) 			Logger::instance().error(__FILE__, __LINE__,f,##__VA_ARGS__)
#define FATAL_LOG(f,...)  			Logger::instance().fatal(__FILE__, __LINE__,f,##__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////

class Logger : public Singleton<Logger> ,Thread 
{
	
public:
	void 	trace(const char *c_filename, long c_fileline, const char* pattern, ...);
	void 	debug(const char *c_filename, long c_fileline, const char* pattern, ...);
	void 	info(const char *c_filename, long c_fileline, const char* pattern, ...);
	void 	warn(const char *c_filename, long c_fileline, const char* pattern, ...);
	void 	error(const char *c_filename, long c_fileline, const char* pattern, ...);
	void 	fatal(const char *c_filename, long c_fileline, const char* pattern, ...);

public:
	void Init(const std::string& fileName, LOG_LVL lvl = INFO_LVL);
	void Release();
	void writeStr(const char *str);
	void writeStr(const std::string& str) { writeStr(str.c_str()); }
	void writeFmt(const char *format, ...);
	void writeException(const Exception& e);
	
	
protected:
	virtual void execute();

private:
	Logger();
	
private:
	std::string getLogFileName();
	bool openFile(FileStream& fileStream, const std::string& fileName);
	void PushLog(const std::string& str);
	void writeLogToFile(const std::string& str);
private:
	std::string				fileName_;       // 日志文件名
	LOG_LVL					m_lvl;
	std::deque<std::string> m_logDeque;
	Condition::Mutex		m_mutex;
	Condition				m_condition;
	friend class Singleton<Logger>;
};







#endif 

