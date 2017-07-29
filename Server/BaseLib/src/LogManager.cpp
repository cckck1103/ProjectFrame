#include "LogManager.h"
#include "DataTime.h"


///////////////////////////////////////////////////////////////////////////////
// class Logger

Logger::Logger() : m_condition(m_mutex)
{
	// nothing
}

//-----------------------------------------------------------------------------
// 描述: 设置日志文件名
// 参数:
//   fileName   - 日志文件名 (含路径)
//-----------------------------------------------------------------------------
void Logger::Init(const std::string& fileName, LOG_LVL lvl)
{
	fileName_ = fileName;
	m_lvl = lvl;
	run();
}

void Logger::Release()
{
	terminate();
}

//-----------------------------------------------------------------------------
// 描述: 将文本写入日志
//-----------------------------------------------------------------------------
void Logger::writeStr(const char *str)
{
	std::string text;
	UINT processId, threadId;

#ifdef _COMPILER_WIN
	processId = GetCurrentProcessId();
	threadId = GetCurrentThreadId();
#endif
#ifdef _COMPILER_LINUX
	processId = getpid();
	threadId = pthread_self();
#endif

	text = formatString("[%s](%05d|%05u) %s%s",
		DateTime::now().toDateTimeString().c_str(),
		processId, threadId, str, S_CRLF);

	PushLog(text);

	printf("%s\n", text.c_str());
}

//-----------------------------------------------------------------------------
// 描述: 将文本写入日志
//-----------------------------------------------------------------------------
void Logger::writeFmt(const char *format, ...)
{
	std::string text;

	va_list argList;
	va_start(argList, format);
	formatStringV(text, format, argList);
	va_end(argList);

	writeStr(text.c_str());
}

//-----------------------------------------------------------------------------
// 描述: 将异常信息写入日志
//-----------------------------------------------------------------------------
void Logger::writeException(const Exception& e)
{
	writeStr(e.makeLogStr().c_str());
}

//-----------------------------------------------------------------------------

std::string Logger::getLogFileName()
{
	std::string result = fileName_;

	if (result.empty())
		result = getAppPath() + "log.txt";

	std::string fileExt = extractFileExt(result);
	result = result.substr(0, result.length() - fileExt.length()) + ".";

	DateTime now = DateTime::now();

	std::string strDate;
	int year, month, day,hour;

	now.decodeDateTime(&year, &month, &day, &hour, NULL, NULL, NULL);
	strDate = formatString("%04d-%02d-%02d %02d",
		year, month, day, hour);
	result += strDate;
	result += fileExt;
	
	return result;
}

//-----------------------------------------------------------------------------

bool Logger::openFile(FileStream& fileStream, const std::string& fileName)
{
	return
		fileStream.open(fileName, FM_OPEN_WRITE | FM_SHARE_DENY_NONE) ||
		fileStream.open(fileName, FM_CREATE | FM_SHARE_DENY_NONE);
}

//-----------------------------------------------------------------------------
// 描述: 将字符串写入文件
//-----------------------------------------------------------------------------
void Logger::PushLog(const std::string& str)
{
	{
		AutoLocker locker(m_mutex);
		m_logDeque.push_back(str);
	}
	m_condition.notify();
}


void 	Logger::trace(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > TRACE_LVL)
	{
		return;
	}

	std::string filename = extractFileName(c_filename);

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[TRACE][%s:%ld] %s", extractFileName(c_filename).c_str(),c_fileline, text.c_str());

	writeStr(buf.c_str());
}

void 	Logger::debug(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > DEBUG_LVL)
	{
		return;
	}

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[DEBUG][%s:%ld] %s", extractFileName(c_filename).c_str(), c_fileline, text.c_str());

	writeStr(buf.c_str());
}

void 	Logger::info(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > INFO_LVL)
	{
		return;
	}

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[INFO][%s:%ld] %s", extractFileName(c_filename).c_str(), c_fileline, text.c_str());

	writeStr(buf.c_str());

}

void 	Logger::warn(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > WARN_LVL)
	{
		return;
	}

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[WARN][%s:%ld] %s", extractFileName(c_filename).c_str(), c_fileline, text.c_str());

	writeStr(buf.c_str());

}

void 	Logger::error(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > ERR_LVL)
	{
		return;
	}

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[ERR][%s:%ld] %s", extractFileName(c_filename).c_str(), c_fileline, text.c_str());

	writeStr(buf.c_str());

}

void 	Logger::fatal(const char *c_filename, long c_fileline, const char* pattern, ...)
{
	if (m_lvl > FATAL_LVL)
	{
		return;
	}

	std::string buf;

	std::string text;
	va_list argList;
	va_start(argList, pattern);
	formatStringV(text, pattern, argList);
	va_end(argList);

	buf = formatString("[FATAL][%s:%ld] %s", extractFileName(c_filename).c_str(), c_fileline, text.c_str());

	writeStr(buf.c_str());

}

void Logger::writeLogToFile(const std::string& str)
{
	std::string fileName = getLogFileName();
	FileStream fs;
	if (!openFile(fs, fileName))
	{
		std::string path = extractFilePath(fileName);
		if (!path.empty())
		{
			forceDirectories(path);
			openFile(fs, fileName);
		}
	}

	if (fs.isOpen())
	{
		fs.seek(0, SO_END);
		fs.write(str.c_str(), (int)str.length());
	}
}

void Logger::execute()
{
	while (!isTerminated())
	try
	{
		{
			AutoLocker locker(m_mutex);
			while (m_logDeque.empty())
			{
				m_condition.wait();
			}
		}

		std::string topStr;
		{
			AutoLocker locker(m_mutex);
			topStr = m_logDeque.front();
			m_logDeque.pop_front();
		}

		if (!topStr.empty())
		{
			writeLogToFile(topStr);
		}

	}
	catch (Exception&)
	{
		writeLogToFile("log manager thread exception and terminate !!!");
	}
}

///////////////////////////////////////////////////////////////////////////////
