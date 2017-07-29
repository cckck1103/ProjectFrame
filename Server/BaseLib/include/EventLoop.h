///////////////////////////////////////////////////////////////////////////////
// EventLoop.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _ISE_EVENT_LOOP_H_
#define _ISE_EVENT_LOOP_H_

#include "Options.h"
#include "UtilClass.h"
#include "SysUtils.h"
#include "Exceptions.h"
#include "ServiceThread.h"
#include "DataTime.h"
#include "Timers.h"

#ifdef _COMPILER_WIN
#include "win_iocp.h"
#endif
#ifdef _COMPILER_LINUX
#include "linux_epoll.h"
#endif


///////////////////////////////////////////////////////////////////////////////
// classes

class EventLoop;
class EventLoopThread;
class EventLoopList;
class OsEventLoop;

///////////////////////////////////////////////////////////////////////////////
// class EventLoop

class EventLoop : noncopyable
{
public:
    typedef std::vector<Functor> Functors;

    struct FunctorList
    {
        Functors items;
        Mutex mutex;
    };

public:
    EventLoop();
    virtual ~EventLoop();

    void start();
    void stop(bool force, bool waitFor);

    bool isRunning();
    bool isInLoopThread();
    void assertInLoopThread();
    void executeInLoop(const Functor& functor);
    void delegateToLoop(const Functor& functor);
    void addFinalizer(const Functor& finalizer);

    TimerId executeAt(Timestamp time, const TimerCallback& callback);
    TimerId executeAfter(INT64 delay, const TimerCallback& callback);
    TimerId executeEvery(INT64 interval, const TimerCallback& callback);
    void cancelTimer(TimerId timerId);

    THREAD_ID getLoopThreadId() const { return loopThreadId_; };

protected:
    virtual void runLoop(Thread *thread);
    virtual void doLoopWork(Thread *thread) = 0;
    virtual void wakeupLoop() {}

protected:
    void executeDelegatedFunctors();
    void executeFinalizer();

    int calcLoopWaitTimeout();
    void processExpiredTimers();

private:
    TimerId addTimer(Timestamp expiration, INT64 interval, const TimerCallback& callback);

protected:
    EventLoopThread *thread_;
    THREAD_ID loopThreadId_;
    FunctorList delegatedFunctors_;
    FunctorList finalizers_;
    UINT64 lastCheckTimeoutTicks_;
    TimerQueue timerQueue_;

    friend class EventLoopThread;
    friend class IocpObject;
    friend class EpollObject;
};

///////////////////////////////////////////////////////////////////////////////
// class EventLoopThread - 事件循环执行线程

class EventLoopThread : public Thread
{
public:
    EventLoopThread(EventLoop& eventLoop);
protected:
    virtual void execute();
    virtual void afterExecute();
private:
    EventLoop& eventLoop_;
};

///////////////////////////////////////////////////////////////////////////////
// class EventLoopList - 事件循环列表

class EventLoopList : noncopyable
{
public:
    enum { MAX_LOOP_COUNT = 64 };

public:
    explicit EventLoopList(int loopCount);
    virtual ~EventLoopList();

    void start();
    void stop();

    int getCount() { return items_.getCount(); }
    EventLoop* findEventLoop(THREAD_ID loopThreadId);

    EventLoop* getItem(int index) { return items_[index]; }
    EventLoop* operator[] (int index) { return getItem(index); }

protected:
    virtual EventLoop* createEventLoop();
private:
    void setCount(int count);
protected:
    ObjectList<EventLoop> items_;
    int wantLoopCount_;
    Mutex mutex_;
};

///////////////////////////////////////////////////////////////////////////////
// class OsEventLoop - 平台相关的事件循环基类

class OsEventLoop : public EventLoop
{
public:
    OsEventLoop();
    virtual ~OsEventLoop();

protected:
    virtual void doLoopWork(Thread *thread);
    virtual void wakeupLoop();

protected:
#ifdef _COMPILER_WIN
    IocpObject *iocpObject_;
#endif
#ifdef _COMPILER_LINUX
    EpollObject *epollObject_;
#endif
};

///////////////////////////////////////////////////////////////////////////////
#endif // _ISE_EVENT_LOOP_H_
