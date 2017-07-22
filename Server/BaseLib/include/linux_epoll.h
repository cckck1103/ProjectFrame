///////////////////////////////////////////////////////////////////////////////
// linux_epoll.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _LINUX_EPOLL_H_
#define _LINUX_EPOLL_H_

#include "Options.h"
#include "UtilClass.h"
#include "BaseSocket.h"
#include "Exceptions.h"

#ifdef _COMPILER_LINUX
#include <sys/epoll.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// classes

#ifdef _COMPILER_LINUX
class EpollObject;
#endif

// 提前声明
class EventLoop;

///////////////////////////////////////////////////////////////////////////////

#ifdef _COMPILER_LINUX

///////////////////////////////////////////////////////////////////////////////
// class EpollObject - Linux EPoll 功能封装

class EpollObject
{
public:
    enum { INITIAL_EVENT_SIZE = 32 };

    enum EVENT_TYPE
    {
        ET_NONE        = 0,
        ET_ERROR       = 1,
        ET_ALLOW_SEND  = 2,
        ET_ALLOW_RECV  = 3,
    };

    typedef std::vector<struct epoll_event> EventList;
    typedef int EventPipe[2];

    typedef std::function<void (BaseTcpConnection *connection, EVENT_TYPE eventType)> NotifyEventCallback;

public:
    EpollObject(EventLoop *eventLoop);
    ~EpollObject();

    void poll();
    void wakeup();

    void addConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv);
    void updateConnection(BaseTcpConnection *connection, bool enableSend, bool enableRecv);
    void removeConnection(BaseTcpConnection *connection);

    void setNotifyEventCallback(const NotifyEventCallback& callback);

private:
    void createEpoll();
    void destroyEpoll();
    void createPipe();
    void destroyPipe();

    void epollControl(int operation, void *param, int handle, bool enableSend, bool enableRecv);

    void processPipeEvent();
    void processEvents(int eventCount);

private:
    EventLoop *eventLoop_;        // 所属 EventLoop
    int epollFd_;                 // EPoll 的文件描述符
    EventList events_;            // 存放 epoll_wait() 返回的事件
    EventPipe pipeFds_;           // 用于唤醒 epoll_wait() 的管道
    NotifyEventCallback onNotifyEvent_;
};

///////////////////////////////////////////////////////////////////////////////

#endif 

///////////////////////////////////////////////////////////////////////////////

#endif