///////////////////////////////////////////////////////////////////////////////
// �ļ�����: Exceptions.cpp
// ��������: �쳣��
///////////////////////////////////////////////////////////////////////////////

#include "Exceptions.h"
#include "Options.h"
#include "SysUtils.h"

///////////////////////////////////////////////////////////////////////////////

const char* const EXCEPTION_LOG_PREFIX = "ERROR: ";

///////////////////////////////////////////////////////////////////////////////
// class Exception

//-----------------------------------------------------------------------------
// ����: ���� std::exception ���ش�����Ϣ
//-----------------------------------------------------------------------------
const char* Exception::what() const throw()
{
    what_ = getErrorMessage();
    return what_.c_str();
}

//-----------------------------------------------------------------------------
// ����: �������� Log ���ַ���
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

