///////////////////////////////////////////////////////////////////////////////
// 文件名称: Exceptions.cpp
// 功能描述: 异常类
///////////////////////////////////////////////////////////////////////////////

#include "Exceptions.h"
#include "Options.h"
#include "SysUtils.h"

///////////////////////////////////////////////////////////////////////////////

const char* const EXCEPTION_LOG_PREFIX = "ERROR: ";

///////////////////////////////////////////////////////////////////////////////
// class Exception

//-----------------------------------------------------------------------------
// 描述: 用于 std::exception 返回错误信息
//-----------------------------------------------------------------------------
const char* Exception::what() const throw()
{
    what_ = getErrorMessage();
    return what_.c_str();
}

//-----------------------------------------------------------------------------
// 描述: 返回用于 Log 的字符串
//-----------------------------------------------------------------------------
std::string Exception::makeLogStr() const
{
    return std::string(EXCEPTION_LOG_PREFIX) + getErrorMessage();
}

///////////////////////////////////////////////////////////////////////////////
// class SimpleException

SimpleException::SimpleException(const char *errorMsg,
    const char *srcFileName, int srcLineNumber)
{
    if (errorMsg)
        errorMsg_ = errorMsg;
    if (srcFileName)
        srcFileName_ = srcFileName;
    srcLineNumber_ = srcLineNumber;
}

//-----------------------------------------------------------------------------

std::string SimpleException::makeLogStr() const
{
	std::string result(getErrorMessage());

    if (!srcFileName_.empty() && srcLineNumber_ >= 0)
        result = result + " (" + srcFileName_ + ":" + intToStr(srcLineNumber_) + ")";

    result = std::string(EXCEPTION_LOG_PREFIX) + result;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// class FileException

FileException::FileException(const char *fileName, int errorCode, const char *errorMsg) :
    fileName_(fileName),
    errorCode_(errorCode)
{
    if (errorMsg == NULL)
        errorMsg_ = sysErrorMessage(errorCode);
    else
        errorMsg_ = errorMsg;
}

