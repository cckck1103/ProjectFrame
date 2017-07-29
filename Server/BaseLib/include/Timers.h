///////////////////////////////////////////////////////////////////////////////
// Timers.h
///////////////////////////////////////////////////////////////////////////////

#ifndef __TIMER_H_
#define __TIMER_H_

#include "Options.h"
#include "Singleton.h"
#include "Exceptions.h"
#include "DataTime.h"

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class EventLoop;
class EventLoopList;
class IoService;

///////////////////////////////////////////////////////////////////////////////
// class Timer

class Timer : noncopyable
{
public:
    Timer(Timestamp expiration, INT64 interval, const TimerCallback& callback);

    Timestamp expiration() const { return expiration_; }
    INT64 interval() const { return interval_; }
    bool repeat() const { return repeat_; }
    TimerId timerId() const { return timerId_; }
    void invokeCallback();

    void restart(Timestamp now);

private:
    Timestamp expiration_;
    const INT64 interval_;  // millisecond
    const bool repeat_;
    TimerId timerId_;
    const TimerCallback callback_;

    static SeqNumberAlloc s_timerIdAlloc;
};

///////////////////////////////////////////////////////////////////////////////
// class TimerQueue

class TimerQueue : noncopyable
{
public:
    TimerQueue();
    ~TimerQueue();

    void addTimer(Timer *timer);
    void cancelTimer(TimerId timerId);
    bool getNearestExpiration(Timestamp& expiration);
    void processExpiredTimers(Timestamp now);

private:
    void clearTimers();

private:
    typedef std::pair<Timestamp, Timer*> TimerItem;
    typedef std::set<TimerItem> TimerList;
    typedef std::map<TimerId, Timer*> TimerIdMap;
    typedef std::set<TimerId> TimerIds;

    TimerList timerList_;
    TimerIdMap timerIdMap_;
    bool callingExpiredTimers_;
    TimerIds cancelingTimers_;
};

///////////////////////////////////////////////////////////////////////////////
// class TimerManager

class TimerManager : public Singleton<TimerManager>
{
public:
    TimerManager();
    ~TimerManager();
public:
	void Init(std::shared_ptr<IoService> service);
    TimerId executeAt(Timestamp time, const TimerCallback& callback);
    TimerId executeAfter(INT64 delay, const TimerCallback& callback);
    TimerId executeEvery(INT64 interval, const TimerCallback& callback);
    void cancelTimer(TimerId timerId);
private:
    EventLoop& getTimerEventLoop();
private:
    EventLoopList *eventLoopList_;
	std::shared_ptr<IoService> m_ioService;
};

///////////////////////////////////////////////////////////////////////////////
#endif // _ISE_TIMER_H_
