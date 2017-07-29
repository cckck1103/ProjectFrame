///////////////////////////////////////////////////////////////////////////////
// 文件名称: timers.cpp
// 功能描述: 定时器的实现
///////////////////////////////////////////////////////////////////////////////

#include "Timers.h"
#include "ErrMsgs.h"
#include "EventLoop.h"
#include "LogManager.h"
#include "TCPServer.h"

///////////////////////////////////////////////////////////////////////////////
// class Timer

SeqNumberAlloc Timer::s_timerIdAlloc(1);

Timer::Timer(Timestamp expiration, INT64 interval, const TimerCallback& callback) :
    expiration_(expiration),
    interval_(interval),
    repeat_(interval > 0.0),
    timerId_(s_timerIdAlloc.allocId()),
    callback_(callback)
{
    // nothing
}

//-----------------------------------------------------------------------------

void Timer::invokeCallback()
{
    try
    {
        if (callback_)
            callback_();
    }
    catch (Exception& e)
    {
        ERROR_LOG("%s",e.makeLogStr().c_str());
    }
    catch (...)
    {}
}

//-----------------------------------------------------------------------------

void Timer::restart(Timestamp now)
{
    if (repeat_)
        expiration_ = now + interval_;
    else
        expiration_ = Timestamp(0);
}

///////////////////////////////////////////////////////////////////////////////
// class TimerQueue

TimerQueue::TimerQueue() :
    callingExpiredTimers_(false)
{
    // nothing
}

TimerQueue::~TimerQueue()
{
    clearTimers();
}

//-----------------------------------------------------------------------------

void TimerQueue::addTimer(Timer *timer)
{
    bool success = timerList_.insert(TimerItem(timer->expiration(), timer)).second;
    ASSERT_X(success);
    (void)success;

    success = timerIdMap_.insert(std::make_pair(timer->timerId(), timer)).second;
	ASSERT_X(success);

	ASSERT_X(timerList_.size() == timerIdMap_.size());
}

//-----------------------------------------------------------------------------

void TimerQueue::cancelTimer(TimerId timerId)
{
    TimerIdMap::iterator iter = timerIdMap_.find(timerId);
    if (iter != timerIdMap_.end())
    {
        Timer *timer = iter->second;
        timerList_.erase(TimerItem(timer->expiration(), timer));
        timerIdMap_.erase(iter);

        delete timer;

		ASSERT_X(timerList_.size() == timerIdMap_.size());
    }
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timerId);
    }
}

//-----------------------------------------------------------------------------

bool TimerQueue::getNearestExpiration(Timestamp& expiration)
{
    bool result = !timerList_.empty();
    if (result)
        expiration = timerList_.begin()->first;
    return result;
}

//-----------------------------------------------------------------------------

void TimerQueue::processExpiredTimers(Timestamp now)
{
    std::vector<Timer*> expiredTimers;
    TimerItem timerItem(now, reinterpret_cast<Timer*>(std::numeric_limits<uintptr_t>::max()));

    TimerList::iterator bound = timerList_.upper_bound(timerItem);
    for (TimerList::iterator iter = timerList_.begin(); iter != bound;)
    {
        Timer *timer = iter->second;
        expiredTimers.push_back(timer);

        timerList_.erase(iter++);
        timerIdMap_.erase(timer->timerId());

		ASSERT_X(timerList_.size() == timerIdMap_.size());
    }

    for (size_t i = 0; i < expiredTimers.size(); i++)
    {
        Timer *timer = expiredTimers[i];

        cancelingTimers_.clear();
        callingExpiredTimers_ = true;
        timer->invokeCallback();
        callingExpiredTimers_ = false;

        if (timer->repeat() &&
            cancelingTimers_.find(timer->timerId()) == cancelingTimers_.end())
        {
            timer->restart(now);
            addTimer(timer);
        }
        else
            delete timer;
    }
}

//-----------------------------------------------------------------------------

void TimerQueue::clearTimers()
{
    for (TimerList::iterator iter = timerList_.begin(); iter != timerList_.end(); ++iter)
        delete iter->second;
    timerList_.clear();
    timerIdMap_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// class TimerManager

TimerManager::TimerManager()
{
	
}

TimerManager::~TimerManager()
{
	if (eventLoopList_)
	{
		eventLoopList_->stop();
		delete eventLoopList_;
	}
}

void TimerManager::Init(std::shared_ptr<IoService> service_)
{
	m_ioService = service_;
	eventLoopList_ = NULL;
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (指定时间执行)
//-----------------------------------------------------------------------------
TimerId TimerManager::executeAt(Timestamp time, const TimerCallback& callback)
{
    return getTimerEventLoop().executeAt(time, callback);
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (在 delay 毫秒后执行)
//-----------------------------------------------------------------------------
TimerId TimerManager::executeAfter(INT64 delay, const TimerCallback& callback)
{
    return getTimerEventLoop().executeAfter(delay, callback);
}

//-----------------------------------------------------------------------------
// 描述: 添加定时器 (每 interval 毫秒循环执行)
//-----------------------------------------------------------------------------
TimerId TimerManager::executeEvery(INT64 interval, const TimerCallback& callback)
{
    return getTimerEventLoop().executeEvery(interval, callback);
}

//-----------------------------------------------------------------------------
// 描述: 取消定时器
//-----------------------------------------------------------------------------
void TimerManager::cancelTimer(TimerId timerId)
{
    getTimerEventLoop().cancelTimer(timerId);
}

//-----------------------------------------------------------------------------
// 描述: 取得用于放置定时器的事件循环
//-----------------------------------------------------------------------------
EventLoop& TimerManager::getTimerEventLoop()
{
    EventLoop *result = NULL;

	if (m_ioService)
		result = m_ioService->GetTcpEventLoopList().findEventLoop(getCurThreadId());

	if (!result)
	{
		if (eventLoopList_ == NULL)
		{
			eventLoopList_ = new EventLoopList(1);
			eventLoopList_->start();
		}
		result = eventLoopList_->getItem(0);
	}

    return *result;
}

///////////////////////////////////////////////////////////////////////////////