///////////////////////////////////////////////////////////////////////////////
// exceptions.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _EXCEPTIONS_H_
#define _EXCEPTIONS_H_

#include "Options.h"
#include "GlobalDefs.h"
#include "ErrMsgs.h"

///////////////////////////////////////////////////////////////////////////////
// class Exception - “Ï≥£ª˘¿‡

class Exception : std::exception
{
public:
    Exception() {}
    virtual ~Exception() throw() {}

    virtual const char* what() const throw();
    virtual std::string getErrorMessage() const { return ""; }
    virtual std::string makeLogStr() const;
private:
    mutable std::string what_;
};

///////////////////////////////////////////////////////////////////////////////
// SimpleException - Simple exception

class SimpleException : public Exception
{
public:
    SimpleException() : srcLineNumber_(-1) {}
    SimpleException(const char *errorMsg,
        const char *srcFileName = NULL,
        int srcLineNumber = -1);
    virtual ~SimpleException() throw() {}

    virtual std::string getErrorMessage() const { return errorMsg_; }
    virtual std::string makeLogStr() const;
    void setErrorMesssage(const char *msg) { errorMsg_ = msg; }

protected:
	std::string errorMsg_;
	std::string srcFileName_;
    int srcLineNumber_;
};

///////////////////////////////////////////////////////////////////////////////
// MemoryException - Memory exception

class MemoryException : public SimpleException
{
public:
    MemoryException() {}
    explicit MemoryException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~MemoryException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// StreamException - Stream exception

class StreamException : public SimpleException
{
public:
    StreamException() {}
    explicit StreamException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~StreamException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// FileException - File exception

class FileException : public SimpleException
{
public:
    FileException() : errorCode_(0) {}
    FileException(const char *fileName, int errorCode, const char *errorMsg = NULL);
    virtual ~FileException() throw() {}
protected:
	std::string fileName_;
    int errorCode_;
};

///////////////////////////////////////////////////////////////////////////////
// ThreadException - Thread exception

class ThreadException : public SimpleException
{
public:
    ThreadException() {}
    explicit ThreadException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~ThreadException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// SocketException - Socket exception

class SocketException : public SimpleException
{
public:
    SocketException() {}
    explicit SocketException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~SocketException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// DbException - Database exception

class DbException : public SimpleException
{
public:
    DbException() {}
    explicit DbException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~DbException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// DataAlgoException - DataAlgo exception

class DataAlgoException : public SimpleException
{
public:
    DataAlgoException() {}
    explicit DataAlgoException(const char *errorMsg) : SimpleException(errorMsg) {}
    virtual ~DataAlgoException() throw() {}
};

///////////////////////////////////////////////////////////////////////////////
// Exception throwing rountines.

#define THROW_EXCEPTION(msg)  ThrowException(msg, __FILE__, __LINE__)

// Throws a SimpleException exception.
inline void ThrowException(const char *msg,
    const char *srcFileName = NULL, int srcLineNumber = -1)
{
    throw SimpleException(msg, srcFileName, srcLineNumber);
}

// Throws a MemoryException exception.
inline void ThrowMemoryException()
{
    throw MemoryException(SEM_OUT_OF_MEMORY);
}

// Throws a StreamException exception.
inline void ThrowStreamException(const char *msg)
{
    throw StreamException(msg);
}

// Throws a FileException exception.
inline void ThrowFileException(const char *fileName, int errorCode,
    const char *errorMsg = NULL)
{
    throw FileException(fileName, errorCode, errorMsg);
}

// Throws a ThreadException exception.
inline void ThrowThreadException(const char *msg)
{
    throw ThreadException(msg);
}

// Throws a SocketException exception.
inline void ThrowSocketException(const char *msg)
{
    throw SocketException(msg);
}

// Throws a DbException exception.
inline void ThrowDbException(const char *msg)
{
    throw DbException(msg);
}

// Throws a DataAlgoException exception.
inline void ThrowDataAlgoException(const char *msg)
{
    throw new DataAlgoException(msg);
}

///////////////////////////////////////////////////////////////////////////////
#endif 