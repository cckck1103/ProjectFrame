///////////////////////////////////////////////////////////////////////////////
// TCPServer.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// 说明:
//
// * 收到连接后(onTcpConnected)，即使用户不调用 connection->recv()，也会在后台
//   自动接收数据。接收到的数据暂存于缓存中。
//
// * 即使用户在连接上无任何动作(既不 send 也不 recv)，当对方断开连接 (close/shutdown) 时，
//   我方也能够感知，并通过 onTcpDisconnected() 通知用户。
//
// * 关于连接的断开:
//   connection->disconnect() 会立即关闭 (shutdown) 发送通道，而不管该连接的
//   缓存中有没有未发送完的数据。由于没有关闭接收通道，所以此时程序仍可以接
//   收对方的数据。正常情况下，对方在检测到我方关闭发送 (read 返回 0) 后，应断
//   开连接 (close)，这样我方才能引发接收错误，进入 errorOccurred()，既而销毁连接。
//
//   如果希望把缓存中的数据发送完毕后再 disconnect()，可在 onTcpSendComplete()
//   中进行断开操作。
//
//   TcpConnection 提供了更灵活的 shutdown(bool closeSend, bool closeRecv) 方法。
//   用户如果希望断开连接时双向关闭，可直接调用 connection->shutdown() 方法，
//   而不是 connection->disconnect()。
//
//   以下情况会立即双向关闭 (shutdown(true, true)) 连接:
//   1. 连接上有错误发生 (errorOccurred())；
//   2. 发送或接收超时 (checkTimeout())；
//   3. 程序退出时关闭现存连接 (clearConnections())。
//
// * 连接对象 (TcpConnection) 采用 std::shared_ptr 管理，由以下几个角色持有:
//   1. TcpEventLoop.
//      由 TcpEventLoop::tcpConnMap_ 持有，TcpEventLoop::removeConnection() 时释放。
//      当调用 TcpConnection::setEventLoop(NULL) 时，将引发 removeConnection()。
//      程序正常退出 (kill 或 App().setTerminated(true)) 时，将清理全部连接
//      (TcpEventLoop::clearConnections())，从而使 TcpEventLoop 释放它持有的全部
//      std::shared_ptr。
//   2. WINDOWS::IOCP.
//      由 IocpTaskData::callback_ 持有。callback_ 作为一个 std::function，保存
//      了由 TcpConnection::shared_from_this() 传递过来的 shared_ptr。
//      当一次IOCP轮循完毕后，会调用 IocpObject::destroyOverlappedData()，此处会
//      析构 callback_，从而释放 shared_ptr。
//      这说明要想销毁一个连接对象，至少应保证在该连接上投递给IOCP的请求在轮循中
//      得到了处理。只有处理完请求，IOCP 才会释放 shared_ptr。
//   3. 用户.
//      业务接口中 Business::onTcpXXX() 系列函数将连接对象以 shared_ptr 形式
//      传递给用户。大部分情况下，对连接对象的自动管理能满足实际需要，但用户
//      仍然可以持有 shared_ptr，以免连接被自动销毁。
//
// * 连接对象 (TcpConnection) 的几个销毁场景:
//   1. onTcpConnected() 之后，用户调用了 connection->disconnect()，
//      引发 IOCP/EPoll 错误，调用 errorOccurred()，执行 onTcpDisconnected() 和
//      TcpConnecton::setEventLoop(NULL)。
//   2. onTcpConnected() 之后，用户没有任何动作，但对方断开了连接，
//      我方检测到后引发 IOCP/EPoll 错误，之后同1。
//   3. 程序在 Linux 下被 kill，或程序中执行了 App().setTerminated(true)，
//      当前剩余连接全部在 TcpEventLoop::clearConnections() 中 被 disconnect()，
//      在接下来的轮循中引发了 IOCP/EPoll 错误，之后同1。


#ifndef _TCP_SERVER__H_
#define _TCP_SERVER__H_

#include "Options.h"
#include "UtilClass.h"
#include "ServiceThread.h"
#include "SysUtils.h"
#include "BaseSocket.h"
#include "Exceptions.h"
#include "Timers.h"
#include "EventLoop.h"
#include "Singleton.h"

#ifdef _COMPILER_WIN
#include <windows.h>
#endif

#ifdef _COMPILER_LINUX
#include <sys/epoll.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class IoBuffer;
class TcpEventLoop;
class TcpEventLoopList;
class TcpConnection;
class TcpClient;
class TcpServer;
class TcpConnector;

#ifdef _COMPILER_WIN
class WinTcpConnection;
class WinTcpEventLoop;
#endif

#ifdef _COMPILER_LINUX
class LinuxTcpConnection;
class LinuxTcpEventLoop;
#endif

class MainTcpServer;

#define DEF_TCP_CONT_MAX_BUFF_SIZE   1024*1024*64
#define DEF_HEART_BEAT_TIME  60*1000
///////////////////////////////////////////////////////////////////////////////
// 类型定义

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

///////////////////////////////////////////////////////////////////////////////
// interfaces

class TcpCallbacks
{
public:
	virtual ~TcpCallbacks() {}

	// 接受了一个新的TCP连接
	virtual void onTcpConnected(const TcpConnectionPtr& connection) = 0;
	// 断开了一个TCP连接
	virtual void onTcpDisconnected(const TcpConnectionPtr& connection) = 0;
	// TCP连接上的一个接收任务已完成
	virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
		int packetSize, const Context& context) = 0;
	// TCP连接上的一个发送任务已完成
	virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context) = 0;
};


// 分包器
typedef std::function<void (
    const char *data,   // 缓存中可用数据的首字节指针
    int bytes,          // 缓存中可用数据的字节数
    int& retrieveBytes  // 返回分离出来的数据包大小，返回0表示现存数据中尚不足以分离出一个完整数据包
)> PacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// 预定义分包器

void bytePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void linePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void nullTerminatedPacketSplitter(const char *data, int bytes, int& retrieveBytes);
void anyPacketSplitter(const char *data, int bytes, int& retrieveBytes);

// 每次接收一个字节的分包器
const PacketSplitter BYTE_PACKET_SPLITTER = &bytePacketSplitter;
// 以 '\r'或'\n' 或其组合为分界字符的分包器
const PacketSplitter LINE_PACKET_SPLITTER = &linePacketSplitter;
// 以 '\0' 为分界字符的分包器
const PacketSplitter NULL_TERMINATED_PACKET_SPLITTER = &nullTerminatedPacketSplitter;
// 无论收到多少字节都立即获取的分包器
const PacketSplitter ANY_PACKET_SPLITTER = &anyPacketSplitter;



///////////////////////////////////////////////////////////////////////////////
// class TcpInspectInfo

class TcpInspectInfo : public Singleton<TcpInspectInfo>
{
public:
    AtomicInt tcpConnCreateCount;    // TcpConnection 对象的创建次数
    AtomicInt tcpConnDestroyCount;   // TcpConnection 对象的销毁次数
    AtomicInt errorOccurredCount;    // TcpConnection::errorOccurred() 的调用次数
    AtomicInt addConnCount;          // TcpEventLoop::addConnection() 的调用次数
    AtomicInt removeConnCount;       // TcpEventLoop::removeConnection() 的调用次数
};

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer - 输入输出缓存
//
// +-----------------+------------------+------------------+
// |  useless bytes  |  readable bytes  |  writable bytes  |
// |                 |     (CONTENT)    |                  |
// +-----------------+------------------+------------------+
// |                 |                  |                  |
// 0     <=     readerIndex   <=   writerIndex    <=    size

class IoBuffer
{
public:
    enum { INITIAL_SIZE = 1024 };

public:
    IoBuffer();
    ~IoBuffer();

    int getReadableBytes() const { return writerIndex_ - readerIndex_; }
    int getWritableBytes() const { return (int)buffer_.size() - writerIndex_; }
    int getUselessBytes() const { return readerIndex_; }

    void append(const std::string& str);
    void append(const void *data, int bytes);
    void append(int bytes);

    void retrieve(int bytes);
    void retrieveAll(std::string& str);
    void retrieveAll();

    void swap(IoBuffer& rhs);
    const char* peek() const { return getBufferPtr() + readerIndex_; }

private:
    char* getBufferPtr() const { return (char*)&*buffer_.begin(); }
    char* getWriterPtr() const { return getBufferPtr() + writerIndex_; }
    void makeSpace(int moreBytes);

private:
    std::vector<char> buffer_;
    int readerIndex_;
    int writerIndex_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoop - 事件循环类

class TcpEventLoop : public OsEventLoop
{
public:
    typedef std::map<std::string, TcpConnectionPtr> TcpConnectionMap;  // <connectionName, TcpConnectionPtr>

public:
    TcpEventLoop();
    virtual ~TcpEventLoop();

	void addConnection(TcpConnection *connection,TcpCallbacks* _callback);
    void removeConnection(TcpConnection *connection);
    void clearConnections();

protected:
    virtual void runLoop(Thread *thread);
    virtual void registerConnection(TcpConnection *connection) = 0;
    virtual void unregisterConnection(TcpConnection *connection) = 0;

private:
    void checkTimeout();
private:
    TcpConnectionMap tcpConnMap_;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpEventLoopList - 事件循环列表

class TcpEventLoopList : public EventLoopList
{
public:
    explicit TcpEventLoopList(int loopCount);
    virtual ~TcpEventLoopList();

    bool registerToEventLoop(BaseTcpConnection *connection, int eventLoopIndex = -1);

    TcpEventLoop* getItem(int index) { return (TcpEventLoop*)EventLoopList::getItem(index); }
    TcpEventLoop* operator[] (int index) { return getItem(index); }

protected:
    virtual EventLoop* createEventLoop();
};


class IoService
{
public:
	IoService(int loopCount);
	virtual ~IoService();

	bool registerToEventLoop(BaseTcpConnection *connection, int eventLoopIndex = -1);

	TcpEventLoopList& GetTcpEventLoopList() {
		return eventLoopList_;
	};
private:
	void  init();

	void  release();
private:
	TcpEventLoopList eventLoopList_;
};

std::shared_ptr<IoService> CreateIOService(int loopCount);
///////////////////////////////////////////////////////////////////////////////
// class TcpConnection - Proactor模型下的TCP连接

class TcpConnection :
    public BaseTcpConnection,
    public std::enable_shared_from_this<TcpConnection>
{
public:
    struct SendTask
    {
    public:
        int bytes;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        SendTask()
        {
            bytes = 0;
            timeout = 0;
            startTicks = 0;
        }
    };

    struct RecvTask
    {
    public:
        PacketSplitter packetSplitter;
        Context context;
        int timeout;
        UINT startTicks;
    public:
        RecvTask()
        {
            timeout = 0;
            startTicks = 0;
        }
    };

    typedef std::deque<SendTask> SendTaskQueue;
    typedef std::deque<RecvTask> RecvTaskQueue;

public:
    TcpConnection(TcpCallbacks* _callback,  int _maxbufsize);
    TcpConnection(TcpCallbacks* _callback, int _maxbufsize,TcpServer *tcpServer, SOCKET socketHandle);
    virtual ~TcpConnection();

    void send(
        const void *buffer,
        size_t size,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    void recv(
        const PacketSplitter& packetSplitter = ANY_PACKET_SPLITTER,
        const Context& context = EMPTY_CONTEXT,
        int timeout = TIMEOUT_INFINITE
        );

    bool isFromClient() const { return (tcpServer_ == NULL);}
    bool isFromServer() const { return (tcpServer_ != NULL);}
    const std::string& getConnectionName() const;
    int getServerIndex() const;
    int getServerPort() const;
    int getServerConnCount() const;

protected:
    virtual void doDisconnect();
    virtual void eventLoopChanged() {}
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout) = 0;
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout) = 0;

protected:
    void errorOccurred();
    void checkTimeout(UINT curTicks);
    void setEventLoop(TcpEventLoop *eventLoop);
    TcpEventLoop* getEventLoop() { return eventLoop_; }

private:
    void init();

protected:
    TcpServer *tcpServer_;                // 所属 TcpServer
    TcpEventLoop *eventLoop_;             // 所属 TcpEventLoop
    mutable std::string connectionName_;       // 连接名称
    IoBuffer sendBuffer_;                 // 数据发送缓存
    IoBuffer recvBuffer_;                 // 数据接收缓存
    SendTaskQueue sendTaskQueue_;         // 发送任务队列
    RecvTaskQueue recvTaskQueue_;         // 接收任务队列
    bool isErrorOccurred_;                // 连接上是否发生了错误
	TcpCallbacks* m_callback;			  // 回调接口
	int	  m_maxbuffszie;
    friend class TcpEventLoop;
    friend class TcpEventLoopList;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpClient

class TcpClient : public BaseTcpClient
{
public:
	TcpClient(TcpCallbacks* _callback,int _maxuffsize = DEF_TCP_CONT_MAX_BUFF_SIZE);
	virtual ~TcpClient();
    TcpConnection& getConnection() { return *static_cast<TcpConnection*>(connection_); }
protected:
    virtual BaseTcpConnection* createConnection();
private:
    bool registerToEventLoop(std::shared_ptr<IoService> service_, int index = -1);
private:
    friend class TcpConnector;

	TcpCallbacks* m_callback;
	int			  m_maxbuffsize;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpServer

class TcpServer : public BaseTcpServer
{
public:
    explicit TcpServer(std::shared_ptr<IoService> service,
				TcpCallbacks* _callback,
				WORD port,
				int maxbuffsize = DEF_TCP_CONT_MAX_BUFF_SIZE);

    int getConnectionCount() const {
		return connCount_.get(); 
	}

    virtual void open();
    virtual void close();

	TcpCallbacks* GetTcpCallbacks() {
		return m_callback;
	}
protected:
    virtual BaseTcpConnection* createConnection(SOCKET socketHandle);
    virtual void acceptConnection(BaseTcpConnection *connection);

private:
    void incConnCount() { connCount_.increment(); }
    void decConnCount() { connCount_.decrement(); }

private:
	std::shared_ptr<IoService> m_IoService;
    mutable AtomicInt connCount_;
	int				maxbufsize_;
	TcpCallbacks* m_callback;
    friend class TcpConnection;
    friend class MainTcpServer;
};

///////////////////////////////////////////////////////////////////////////////
// class TcpConnector - TCP连接器类

class TcpConnector : noncopyable
{
public:
    typedef std::function<void (bool success, TcpConnection *connection,
        const InetAddress& peerAddr, const Context& context)> CompleteCallback;

private:
    typedef std::vector<SOCKET> FdList;

	struct TaskItem
    {
		TaskItem(TcpCallbacks* _callback,int maxbuffsize) :tcpClient(_callback, maxbuffsize) {};
        TcpClient tcpClient;
        InetAddress peerAddr;
        CompleteCallback completeCallback;
        ASYNC_CONNECT_STATE state;
        Context context;
    };

    typedef ObjectList<TaskItem> TaskList;

    class WorkerThread : public Thread
    {
    public:
        WorkerThread(TcpConnector& owner) : owner_(owner) {}
    protected:
        virtual void execute() { owner_.work(*this); }
        virtual void afterExecute() { owner_.thread_ = NULL; }
    private:
        TcpConnector& owner_;
    };

    friend class WorkerThread;

public:
    TcpConnector(std::shared_ptr<IoService> service_);
    ~TcpConnector();

    void connect(const InetAddress& peerAddr,
		TcpCallbacks* _callback,
        const CompleteCallback& completeCallback,
        const Context& context = EMPTY_CONTEXT,
		int maxbuffsize = DEF_TCP_CONT_MAX_BUFF_SIZE);
    void clear();

private:
    void start();
    void stop();
    void work(WorkerThread& thread);

    void tryConnect();
    void getPendingFdsFromTaskList(int& fromIndex, FdList& fds);
    void checkAsyncConnectState(const FdList& fds, FdList& connectedFds, FdList& failedFds);
    TaskItem* findTask(SOCKET fd);
    void invokeCompleteCallback();

private:
    TaskList taskList_;
    Mutex mutex_;
    WorkerThread *thread_;
	std::shared_ptr<IoService> m_IoService;
};

///////////////////////////////////////////////////////////////////////////////

#ifdef _COMPILER_WIN

///////////////////////////////////////////////////////////////////////////////
// class WinTcpConnection

class WinTcpConnection : public TcpConnection
{
public:
    WinTcpConnection(TcpCallbacks* _callback, int _maxbufsize);
    WinTcpConnection(TcpCallbacks* _callback,int _maxbufsize,TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    WinTcpEventLoop* getEventLoop() { return (WinTcpEventLoop*)eventLoop_; }

    void trySend();
    void tryRecv();

    static void onIocpCallback(const TcpConnectionPtr& thisObj, const IocpTaskData& taskData);
    void onSendCallback(const IocpTaskData& taskData);
    void onRecvCallback(const IocpTaskData& taskData);

private:
    bool isSending_;       // 是否已向IOCP提交发送任务但尚未收到回调通知
    bool isRecving_;       // 是否已向IOCP提交接收任务但尚未收到回调通知
    int bytesSent_;        // 自从上次发送任务完成回调以来共发送了多少字节
    int bytesRecved_;      // 自从上次接收任务完成回调以来共接收了多少字节
};

///////////////////////////////////////////////////////////////////////////////
// class WinTcpEventLoop

class WinTcpEventLoop : public TcpEventLoop
{
public:
	WinTcpEventLoop(){};
	virtual ~WinTcpEventLoop() {};
    IocpObject* getIocpObject() { return iocpObject_; }

protected:
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef _COMPILER_WIN */

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef _COMPILER_LINUX

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpConnection

class LinuxTcpConnection : public TcpConnection
{
public:
    LinuxTcpConnection(TcpCallbacks* _callback,int _maxbuffsize);
    LinuxTcpConnection(TcpCallbacks* _callback, int _maxbuffsize,TcpServer *tcpServer, SOCKET socketHandle);

protected:
    virtual void eventLoopChanged();
    virtual void postSendTask(const void *buffer, int size, const Context& context, int timeout);
    virtual void postRecvTask(const PacketSplitter& packetSplitter, const Context& context, int timeout);

private:
    void init();

    LinuxTcpEventLoop* getEventLoop() { return (LinuxTcpEventLoop*)eventLoop_; }

    void setSendEnabled(bool enabled);
    void setRecvEnabled(bool enabled);

    void trySend();
    void tryRecv();

    bool tryRetrievePacket();
    static void afterPostRecvTask(const TcpConnectionPtr& thisObj);

private:
    int bytesSent_;                  // 自从上次发送任务完成回调以来共发送了多少字节
    bool enableSend_;                // 是否监视可发送事件
    bool enableRecv_;                // 是否监视可接收事件

    friend class LinuxTcpEventLoop;
};

///////////////////////////////////////////////////////////////////////////////
// class LinuxTcpEventLoop

class LinuxTcpEventLoop : public TcpEventLoop
{
public:
    LinuxTcpEventLoop(){};
    virtual ~LinuxTcpEventLoop();

    void updateConnection(TcpConnection *connection, bool enableSend, bool enableRecv);

protected:
    virtual void registerConnection(TcpConnection *connection);
    virtual void unregisterConnection(TcpConnection *connection);

private:
    void onEpollNotifyEvent(BaseTcpConnection *connection, EpollObject::EVENT_TYPE eventType);
};

///////////////////////////////////////////////////////////////////////////////

#endif  /* ifdef _COMPILER_LINUX */

#endif // SERVER_TCP_H_
