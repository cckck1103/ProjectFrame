///////////////////////////////////////////////////////////////////////////////
// win_iocp.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _WIN_IOCP_H_
#define _WIN_IOCP_H_

#include "Options.h"
#include "UtilClass.h"
#include "SysUtils.h"
#include "BaseSocket.h"
#include "Exceptions.h"

#ifdef _COMPILER_WIN
#include <windows.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// classes

#ifdef _COMPILER_WIN
class IocpTaskData;
class IocpBufferAllocator;
class IocpPendingCounter;
class IocpObject;
#endif

// 提前声明
class EventLoop;

///////////////////////////////////////////////////////////////////////////////

#ifdef _COMPILER_WIN

///////////////////////////////////////////////////////////////////////////////
// 类型定义

enum IOCP_TASK_TYPE
{
    ITT_SEND = 1,
    ITT_RECV = 2,
};

typedef std::function<void (const IocpTaskData& taskData)> IocpCallback;

///////////////////////////////////////////////////////////////////////////////
// class IocpTaskData

class IocpTaskData
{
public:
    IocpTaskData();

    HANDLE getIocpHandle() const { return iocpHandle_; }
    HANDLE getFileHandle() const { return fileHandle_; }
    IOCP_TASK_TYPE getTaskType() const { return taskType_; }
    UINT getTaskSeqNum() const { return taskSeqNum_; }
    PVOID getCaller() const { return caller_; }
    const Context& getContext() const { return context_; }
    char* getEntireDataBuf() const { return (char*)entireDataBuf_; }
    int getEntireDataSize() const { return entireDataSize_; }
    char* getDataBuf() const { return (char*)wsaBuffer_.buf; }
    int getDataSize() const { return wsaBuffer_.len; }
    int getBytesTrans() const { return bytesTrans_; }
    int getErrorCode() const { return errorCode_; }
    const IocpCallback& getCallback() const { return callback_; }

private:
    HANDLE iocpHandle_;
    HANDLE fileHandle_;
    IOCP_TASK_TYPE taskType_;
    UINT taskSeqNum_;
    PVOID caller_;
    Context context_;
    PVOID entireDataBuf_;
    int entireDataSize_;
    WSABUF wsaBuffer_;
    int bytesTrans_;
    int errorCode_;
    IocpCallback callback_;

    friend class IocpObject;
};

#pragma pack(1)
struct IocpOverlappedData
{
    OVERLAPPED overlapped;
    IocpTaskData taskData;
};
#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class IocpBufferAllocator

class IocpBufferAllocator : noncopyable
{
public:
    IocpBufferAllocator(int bufferSize);
    ~IocpBufferAllocator();

    PVOID allocBuffer();
    void returnBuffer(PVOID buffer);

    int getUsedCount() const { return usedCount_; }

private:
    void clear();

private:
    int bufferSize_;
    PointerList items_;
    int usedCount_;
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpPendingCounter

class IocpPendingCounter : noncopyable
{
public:
    IocpPendingCounter() {}
    virtual ~IocpPendingCounter() {}

    void inc(PVOID caller, IOCP_TASK_TYPE taskType);
    void dec(PVOID caller, IOCP_TASK_TYPE taskType);
    int get(PVOID caller);
    int get(IOCP_TASK_TYPE taskType);

private:
    struct CountData
    {
        int sendCount;
        int recvCount;
    };

    typedef std::map<PVOID, CountData> Items;   // <caller, CountData>

    Items items_;
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////
// class IocpObject

class IocpObject : noncopyable
{
public:
    IocpObject(EventLoop *eventLoop);
    virtual ~IocpObject();

    bool associateHandle(SOCKET socketHandle);
    bool isComplete(PVOID caller);

    void work();
    void wakeup();

    void send(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void recv(SOCKET socketHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);

private:
    void initialize();
    void finalize();
    void throwGeneralError();
    IocpOverlappedData* createOverlappedData(IOCP_TASK_TYPE taskType,
        HANDLE fileHandle, PVOID buffer, int size, int offset,
        const IocpCallback& callback, PVOID caller, const Context& context);
    void destroyOverlappedData(IocpOverlappedData *ovDataPtr);
    void postError(int errorCode, IocpOverlappedData *ovDataPtr);
    void invokeCallback(const IocpTaskData& taskData);

private:
    static IocpBufferAllocator bufferAlloc_;
    static SeqNumberAlloc taskSeqAlloc_;
    static IocpPendingCounter pendingCounter_;

    EventLoop *eventLoop_;
    HANDLE iocpHandle_;

    friend class AutoFinalizer;
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef _COMPILER_LINUX */

///////////////////////////////////////////////////////////////////////////////
#endif 