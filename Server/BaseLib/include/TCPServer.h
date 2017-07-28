///////////////////////////////////////////////////////////////////////////////
// TCPServer.h
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// ˵��:
//
// * �յ����Ӻ�(onTcpConnected)����ʹ�û������� connection->recv()��Ҳ���ں�̨
//   �Զ��������ݡ����յ��������ݴ��ڻ����С�
//
// * ��ʹ�û������������κζ���(�Ȳ� send Ҳ�� recv)�����Է��Ͽ����� (close/shutdown) ʱ��
//   �ҷ�Ҳ�ܹ���֪����ͨ�� onTcpDisconnected() ֪ͨ�û���
//
// * �������ӵĶϿ�:
//   connection->disconnect() �������ر� (shutdown) ����ͨ���������ܸ����ӵ�
//   ��������û��δ����������ݡ�����û�йرս���ͨ�������Դ�ʱ�����Կ��Խ�
//   �նԷ������ݡ���������£��Է��ڼ�⵽�ҷ��رշ��� (read ���� 0) ��Ӧ��
//   ������ (close)�������ҷ������������մ��󣬽��� errorOccurred()���ȶ��������ӡ�
//
//   ���ϣ���ѻ����е����ݷ�����Ϻ��� disconnect()������ onTcpSendComplete()
//   �н��жϿ�������
//
//   TcpConnection �ṩ�˸����� shutdown(bool closeSend, bool closeRecv) ������
//   �û����ϣ���Ͽ�����ʱ˫��رգ���ֱ�ӵ��� connection->shutdown() ������
//   ������ connection->disconnect()��
//
//   �������������˫��ر� (shutdown(true, true)) ����:
//   1. �������д����� (errorOccurred())��
//   2. ���ͻ���ճ�ʱ (checkTimeout())��
//   3. �����˳�ʱ�ر��ִ����� (clearConnections())��
//
// * ���Ӷ��� (TcpConnection) ���� std::shared_ptr ���������¼�����ɫ����:
//   1. TcpEventLoop.
//      �� TcpEventLoop::tcpConnMap_ ���У�TcpEventLoop::removeConnection() ʱ�ͷš�
//      ������ TcpConnection::setEventLoop(NULL) ʱ�������� removeConnection()��
//      ���������˳� (kill �� App().setTerminated(true)) ʱ��������ȫ������
//      (TcpEventLoop::clearConnections())���Ӷ�ʹ TcpEventLoop �ͷ������е�ȫ��
//      std::shared_ptr��
//   2. WINDOWS::IOCP.
//      �� IocpTaskData::callback_ ���С�callback_ ��Ϊһ�� std::function������
//      ���� TcpConnection::shared_from_this() ���ݹ����� shared_ptr��
//      ��һ��IOCP��ѭ��Ϻ󣬻���� IocpObject::destroyOverlappedData()���˴���
//      ���� callback_���Ӷ��ͷ� shared_ptr��
//      ��˵��Ҫ������һ�����Ӷ�������Ӧ��֤�ڸ�������Ͷ�ݸ�IOCP����������ѭ��
//      �õ��˴���ֻ�д���������IOCP �Ż��ͷ� shared_ptr��
//   3. �û�.
//      ҵ��ӿ��� Business::onTcpXXX() ϵ�к��������Ӷ����� shared_ptr ��ʽ
//      ���ݸ��û����󲿷�����£������Ӷ�����Զ�����������ʵ����Ҫ�����û�
//      ��Ȼ���Գ��� shared_ptr���������ӱ��Զ����١�
//
// * ���Ӷ��� (TcpConnection) �ļ������ٳ���:
//   1. onTcpConnected() ֮���û������� connection->disconnect()��
//      ���� IOCP/EPoll ���󣬵��� errorOccurred()��ִ�� onTcpDisconnected() ��
//      TcpConnecton::setEventLoop(NULL)��
//   2. onTcpConnected() ֮���û�û���κζ��������Է��Ͽ������ӣ�
//      �ҷ���⵽������ IOCP/EPoll ����֮��ͬ1��
//   3. ������ Linux �±� kill���������ִ���� App().setTerminated(true)��
//      ��ǰʣ������ȫ���� TcpEventLoop::clearConnections() �� �� disconnect()��
//      �ڽ���������ѭ�������� IOCP/EPoll ����֮��ͬ1��


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
// ��ǰ����

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
// ���Ͷ���

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

///////////////////////////////////////////////////////////////////////////////
// interfaces

class TcpCallbacks
{
public:
	virtual ~TcpCallbacks() {}

	// ������һ���µ�TCP����
	virtual void onTcpConnected(const TcpConnectionPtr& connection) = 0;
	// �Ͽ���һ��TCP����
	virtual void onTcpDisconnected(const TcpConnectionPtr& connection) = 0;
	// TCP�����ϵ�һ���������������
	virtual void onTcpRecvComplete(const TcpConnectionPtr& connection, void *packetBuffer,
		int packetSize, const Context& context) = 0;
	// TCP�����ϵ�һ���������������
	virtual void onTcpSendComplete(const TcpConnectionPtr& connection, const Context& context) = 0;
};


// �ְ���
typedef std::function<void (
    const char *data,   // �����п������ݵ����ֽ�ָ��
    int bytes,          // �����п������ݵ��ֽ���
    int& retrieveBytes  // ���ط�����������ݰ���С������0��ʾ�ִ��������в����Է����һ���������ݰ�
)> PacketSplitter;

///////////////////////////////////////////////////////////////////////////////
// Ԥ����ְ���

void bytePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void linePacketSplitter(const char *data, int bytes, int& retrieveBytes);
void nullTerminatedPacketSplitter(const char *data, int bytes, int& retrieveBytes);
void anyPacketSplitter(const char *data, int bytes, int& retrieveBytes);

// ÿ�ν���һ���ֽڵķְ���
const PacketSplitter BYTE_PACKET_SPLITTER = &bytePacketSplitter;
// �� '\r'��'\n' �������Ϊ�ֽ��ַ��ķְ���
const PacketSplitter LINE_PACKET_SPLITTER = &linePacketSplitter;
// �� '\0' Ϊ�ֽ��ַ��ķְ���
const PacketSplitter NULL_TERMINATED_PACKET_SPLITTER = &nullTerminatedPacketSplitter;
// �����յ������ֽڶ�������ȡ�ķְ���
const PacketSplitter ANY_PACKET_SPLITTER = &anyPacketSplitter;



///////////////////////////////////////////////////////////////////////////////
// class TcpInspectInfo

class TcpInspectInfo : public Singleton<TcpInspectInfo>
{
public:
    AtomicInt tcpConnCreateCount;    // TcpConnection ����Ĵ�������
    AtomicInt tcpConnDestroyCount;   // TcpConnection ��������ٴ���
    AtomicInt errorOccurredCount;    // TcpConnection::errorOccurred() �ĵ��ô���
    AtomicInt addConnCount;          // TcpEventLoop::addConnection() �ĵ��ô���
    AtomicInt removeConnCount;       // TcpEventLoop::removeConnection() �ĵ��ô���
};

///////////////////////////////////////////////////////////////////////////////
// class IoBuffer - �����������
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
// class TcpEventLoop - �¼�ѭ����

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
// class TcpEventLoopList - �¼�ѭ���б�

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
// class TcpConnection - Proactorģ���µ�TCP����

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
    TcpServer *tcpServer_;                // ���� TcpServer
    TcpEventLoop *eventLoop_;             // ���� TcpEventLoop
    mutable std::string connectionName_;       // ��������
    IoBuffer sendBuffer_;                 // ���ݷ��ͻ���
    IoBuffer recvBuffer_;                 // ���ݽ��ջ���
    SendTaskQueue sendTaskQueue_;         // �����������
    RecvTaskQueue recvTaskQueue_;         // �����������
    bool isErrorOccurred_;                // �������Ƿ����˴���
	TcpCallbacks* m_callback;			  // �ص��ӿ�
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
// class TcpConnector - TCP��������

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
    bool isSending_;       // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
    bool isRecving_;       // �Ƿ�����IOCP�ύ����������δ�յ��ص�֪ͨ
    int bytesSent_;        // �Դ��ϴη���������ɻص������������˶����ֽ�
    int bytesRecved_;      // �Դ��ϴν���������ɻص������������˶����ֽ�
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
    int bytesSent_;                  // �Դ��ϴη���������ɻص������������˶����ֽ�
    bool enableSend_;                // �Ƿ���ӿɷ����¼�
    bool enableRecv_;                // �Ƿ���ӿɽ����¼�

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
