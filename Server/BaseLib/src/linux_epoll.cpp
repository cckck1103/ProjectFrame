
///////////////////////////////////////////////////////////////////////////////
// �ļ�����: linux_epoll.cpp
// ��������: EPoll ʵ��
///////////////////////////////////////////////////////////////////////////////

#include "linux_epoll.h"
#include "ErrMsgs.h"
#include "LogManager.h"
#include "EventLoop.h"


///////////////////////////////////////////////////////////////////////////////

#ifdef _COMPILER_LINUX

///////////////////////////////////////////////////////////////////////////////
// class EpollObject

EpollObject::EpollObject(EventLoop *eventLoop) :
    eventLoop_(eventLoop)
{
    events_.resize(INITIAL_EVENT_SIZE);
    createEpoll();
    createPipe();
}

EpollObject::~EpollObject()
{
    destroyPipe();
    destroyEpoll();
}

//-----------------------------------------------------------------------------
// ����: ִ��һ�� EPoll ��ѭ
//-----------------------------------------------------------------------------
void EpollObject::poll()
{
    int timeout = eventLoop_->calcLoopWaitTimeout();

    int eventCount = ::epoll_wait(epollFd_, &events_[0], (int)events_.size(), timeout);

    if (timeout != TIMEOUT_INFINITE)
        eventLoop_->processExpiredTimers();

    if (eventCount > 0)
    {
        processEvents(eventCount);

        if (eventCount == (int)events_.size())
            events_.resize(eventCount * 2);
    }
    else if (eventCount < 0)
    {
		ERROR_LOG(SEM_EPOLL_WAIT_ERROR);
    }
}

//-----------------------------------------------------------------------------
// ����: �������������� Poll() ����
//-----------------------------------------------------------------------------
void EpollObject::wakeup()
{
    BYTE val = 0;
    ::write(pipeFds_[1], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// ����: �� EPoll �����һ������
//-----------------------------------------------------------------------------
void EpollObject::addConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_ADD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// ����: ���� EPoll �е�һ������
//-----------------------------------------------------------------------------
void EpollObject::updateConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv)
{
    epollControl(
        EPOLL_CTL_MOD, connection, connection->getSocket().getHandle(),
        enableSend, enableRecv);
}

//-----------------------------------------------------------------------------
// ����: �� EPoll ��ɾ��һ������
//-----------------------------------------------------------------------------
void EpollObject::removeConnection(BaseTcpConnection *connection)
{
    epollControl(
        EPOLL_CTL_DEL, connection, connection->getSocket().getHandle(),
        false, false);
}

//-----------------------------------------------------------------------------
// ����: ���ûص�
//-----------------------------------------------------------------------------
void EpollObject::setNotifyEventCallback(const NotifyEventCallback& callback)
{
    onNotifyEvent_ = callback;
}

//-----------------------------------------------------------------------------

void EpollObject::createEpoll()
{
    epollFd_ = ::epoll_create(1024);
    if (epollFd_ < 0)
		ERROR_LOG(SEM_CREATE_EPOLL_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyEpoll()
{
    if (epollFd_ > 0)
        ::close(epollFd_);
}

//-----------------------------------------------------------------------------

void EpollObject::createPipe()
{
    // pipeFds_[0] for reading, pipeFds_[1] for writing.
    memset(pipeFds_, 0, sizeof(pipeFds_));
    if (::pipe(pipeFds_) == 0)
        epollControl(EPOLL_CTL_ADD, NULL, pipeFds_[0], false, true);
    else
        logger().writeStr(SEM_CREATE_PIPE_ERROR);
}

//-----------------------------------------------------------------------------

void EpollObject::destroyPipe()
{
    epollControl(EPOLL_CTL_DEL, NULL, pipeFds_[0], false, false);

    if (pipeFds_[0]) close(pipeFds_[0]);
    if (pipeFds_[1]) close(pipeFds_[1]);

    memset(pipeFds_, 0, sizeof(pipeFds_));
}

//-----------------------------------------------------------------------------

void EpollObject::epollControl(int operation, void *param, int handle,
    bool enableSend, bool enableRecv)
{
    // ע: ���� Level Triggered (LT, Ҳ�� "��ƽ����") ģʽ

    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.ptr = param;
    if (enableSend)
        event.events |= EPOLLOUT;
    if (enableRecv)
        event.events |= (EPOLLIN | EPOLLPRI);

    if (::epoll_ctl(epollFd_, operation, handle, &event) < 0)
    {
		ERROR_LOG(SEM_EPOLL_CTRL_ERROR, operation);
    }
}

//-----------------------------------------------------------------------------
// ����: ����ܵ��¼�
//-----------------------------------------------------------------------------
void EpollObject::processPipeEvent()
{
    BYTE val;
    ::read(pipeFds_[0], &val, sizeof(BYTE));
}

//-----------------------------------------------------------------------------
// ����: ���� EPoll ��ѭ����¼�
//-----------------------------------------------------------------------------
void EpollObject::processEvents(int eventCount)
{
/*
// from epoll.h
enum EPOLL_EVENTS
{
    EPOLLIN      = 0x001,
    EPOLLPRI     = 0x002,
    EPOLLOUT     = 0x004,
    EPOLLRDNORM  = 0x040,
    EPOLLRDBAND  = 0x080,
    EPOLLWRNORM  = 0x100,
    EPOLLWRBAND  = 0x200,
    EPOLLMSG     = 0x400,
    EPOLLERR     = 0x008,
    EPOLLHUP     = 0x010,
    EPOLLRDHUP   = 0x2000,
    EPOLLONESHOT = (1 << 30),
    EPOLLET      = (1 << 31)
};
*/

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

    for (int i = 0; i < eventCount; i++)
    {
        epoll_event& ev = events_[i];
        if (ev.data.ptr == NULL)  // for pipe
        {
            processPipeEvent();
        }
        else
        {
            BaseTcpConnection *connection = (BaseTcpConnection*)ev.data.ptr;
            EVENT_TYPE eventType = ET_NONE;

			DEBUG_LOG("processEvents: %u", ev.events);  // debug

            if ((ev.events & EPOLLERR) || ((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN)))
                eventType = ET_ERROR;
            else if (ev.events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))
                eventType = ET_ALLOW_RECV;
            else if (ev.events & EPOLLOUT)
                eventType = ET_ALLOW_SEND;

            if (eventType != ET_NONE && onNotifyEvent_)
                onNotifyEvent_(connection, eventType);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

#endif 

///////////////////////////////////////////////////////////////////////////////