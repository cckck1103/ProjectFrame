///////////////////////////////////////////////////////////////////////////////
// BaseSocket.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _BASE_SOCKET_H_
#define _BASE_SOCKET_H_

#include "Options.h"
#include "StringList.h"
#include "UtilClass.h"

#ifdef _COMPILER_WIN
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#endif

#ifdef _COMPILER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <string>
#endif

#include "UtilClass.h"
#include "ServiceThread.h"
#include "NonCopyable.h"
#include "GlobalDefs.h"

///////////////////////////////////////////////////////////////////////////////
// 提前声明

class InetAddress;
class Socket;
class UdpSocket;
class BaseUdpClient;
class BaseUdpServer;
class TcpSocket;
class BaseTcpConnection;
class BaseTcpClient;
class BaseTcpServer;
class ListenerThread;
class UdpListenerThread;
class UdpListenerThreadPool;
class TcpListenerThread;

///////////////////////////////////////////////////////////////////////////////
// 常量定义

#ifdef _COMPILER_WIN
const int SS_SD_RECV            = 0;
const int SS_SD_SEND            = 1;
const int SS_SD_BOTH            = 2;

const int SS_EINTR              = WSAEINTR;
const int SS_EBADF              = WSAEBADF;
const int SS_EACCES             = WSAEACCES;
const int SS_EFAULT             = WSAEFAULT;
const int SS_EINVAL             = WSAEINVAL;
const int SS_EMFILE             = WSAEMFILE;
const int SS_EWOULDBLOCK        = WSAEWOULDBLOCK;
const int SS_EINPROGRESS        = WSAEINPROGRESS;
const int SS_EALREADY           = WSAEALREADY;
const int SS_ENOTSOCK           = WSAENOTSOCK;
const int SS_EDESTADDRREQ       = WSAEDESTADDRREQ;
const int SS_EMSGSIZE           = WSAEMSGSIZE;
const int SS_EPROTOTYPE         = WSAEPROTOTYPE;
const int SS_ENOPROTOOPT        = WSAENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = WSAEPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = WSAESOCKTNOSUPPORT;
const int SS_EOPNOTSUPP         = WSAEOPNOTSUPP;
const int SS_EPFNOSUPPORT       = WSAEPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = WSAEAFNOSUPPORT;
const int SS_EADDRINUSE         = WSAEADDRINUSE;
const int SS_EADDRNOTAVAIL      = WSAEADDRNOTAVAIL;
const int SS_ENETDOWN           = WSAENETDOWN;
const int SS_ENETUNREACH        = WSAENETUNREACH;
const int SS_ENETRESET          = WSAENETRESET;
const int SS_ECONNABORTED       = WSAECONNABORTED;
const int SS_ECONNRESET         = WSAECONNRESET;
const int SS_ENOBUFS            = WSAENOBUFS;
const int SS_EISCONN            = WSAEISCONN;
const int SS_ENOTCONN           = WSAENOTCONN;
const int SS_ESHUTDOWN          = WSAESHUTDOWN;
const int SS_ETOOMANYREFS       = WSAETOOMANYREFS;
const int SS_ETIMEDOUT          = WSAETIMEDOUT;
const int SS_ECONNREFUSED       = WSAECONNREFUSED;
const int SS_ELOOP              = WSAELOOP;
const int SS_ENAMETOOLONG       = WSAENAMETOOLONG;
const int SS_EHOSTDOWN          = WSAEHOSTDOWN;
const int SS_EHOSTUNREACH       = WSAEHOSTUNREACH;
const int SS_ENOTEMPTY          = WSAENOTEMPTY;
#endif

#ifdef _COMPILER_LINUX
const int SS_SD_RECV            = SHUT_RD;
const int SS_SD_SEND            = SHUT_WR;
const int SS_SD_BOTH            = SHUT_RDWR;

const int SS_EINTR              = EINTR;
const int SS_EBADF              = EBADF;
const int SS_EACCES             = EACCES;
const int SS_EFAULT             = EFAULT;
const int SS_EINVAL             = EINVAL;
const int SS_EMFILE             = EMFILE;
const int SS_EWOULDBLOCK        = EWOULDBLOCK;
const int SS_EINPROGRESS        = EINPROGRESS;
const int SS_EALREADY           = EALREADY;
const int SS_ENOTSOCK           = ENOTSOCK;
const int SS_EDESTADDRREQ       = EDESTADDRREQ;
const int SS_EMSGSIZE           = EMSGSIZE;
const int SS_EPROTOTYPE         = EPROTOTYPE;
const int SS_ENOPROTOOPT        = ENOPROTOOPT;
const int SS_EPROTONOSUPPORT    = EPROTONOSUPPORT;
const int SS_ESOCKTNOSUPPORT    = ESOCKTNOSUPPORT;

const int SS_EOPNOTSUPP         = EOPNOTSUPP;
const int SS_EPFNOSUPPORT       = EPFNOSUPPORT;
const int SS_EAFNOSUPPORT       = EAFNOSUPPORT;
const int SS_EADDRINUSE         = EADDRINUSE;
const int SS_EADDRNOTAVAIL      = EADDRNOTAVAIL;
const int SS_ENETDOWN           = ENETDOWN;
const int SS_ENETUNREACH        = ENETUNREACH;
const int SS_ENETRESET          = ENETRESET;
const int SS_ECONNABORTED       = ECONNABORTED;
const int SS_ECONNRESET         = ECONNRESET;
const int SS_ENOBUFS            = ENOBUFS;
const int SS_EISCONN            = EISCONN;
const int SS_ENOTCONN           = ENOTCONN;
const int SS_ESHUTDOWN          = ESHUTDOWN;
const int SS_ETOOMANYREFS       = ETOOMANYREFS;
const int SS_ETIMEDOUT          = ETIMEDOUT;
const int SS_ECONNREFUSED       = ECONNREFUSED;
const int SS_ELOOP              = ELOOP;
const int SS_ENAMETOOLONG       = ENAMETOOLONG;
const int SS_EHOSTDOWN          = EHOSTDOWN;
const int SS_EHOSTUNREACH       = EHOSTUNREACH;
const int SS_ENOTEMPTY          = ENOTEMPTY;
#endif

///////////////////////////////////////////////////////////////////////////////
// 错误信息 (Socket Error Message)

const char* const SSEM_ERROR             = "Socket Error #%d: %s";
const char* const SSEM_SOCKETERROR       = "Socket error";
const char* const SSEM_TCPSENDTIMEOUT    = "TCP send timeout";
const char* const SSEM_TCPRECVTIMEOUT    = "TCP recv timeout";

const char* const SSEM_EINTR             = "Interrupted system call.";
const char* const SSEM_EBADF             = "Bad file number.";
const char* const SSEM_EACCES            = "Access denied.";
const char* const SSEM_EFAULT            = "Buffer fault.";
const char* const SSEM_EINVAL            = "Invalid argument.";
const char* const SSEM_EMFILE            = "Too many open files.";
const char* const SSEM_EWOULDBLOCK       = "Operation would block.";
const char* const SSEM_EINPROGRESS       = "Operation now in progress.";
const char* const SSEM_EALREADY          = "Operation already in progress.";
const char* const SSEM_ENOTSOCK          = "Socket operation on non-socket.";
const char* const SSEM_EDESTADDRREQ      = "Destination address required.";
const char* const SSEM_EMSGSIZE          = "Message too long.";
const char* const SSEM_EPROTOTYPE        = "Protocol wrong type for socket.";
const char* const SSEM_ENOPROTOOPT       = "Bad protocol option.";
const char* const SSEM_EPROTONOSUPPORT   = "Protocol not supported.";
const char* const SSEM_ESOCKTNOSUPPORT   = "Socket type not supported.";
const char* const SSEM_EOPNOTSUPP        = "Operation not supported on socket.";
const char* const SSEM_EPFNOSUPPORT      = "Protocol family not supported.";
const char* const SSEM_EAFNOSUPPORT      = "Address family not supported by protocol family.";
const char* const SSEM_EADDRINUSE        = "Address already in use.";
const char* const SSEM_EADDRNOTAVAIL     = "Cannot assign requested address.";
const char* const SSEM_ENETDOWN          = "Network is down.";
const char* const SSEM_ENETUNREACH       = "Network is unreachable.";
const char* const SSEM_ENETRESET         = "Net dropped connection or reset.";
const char* const SSEM_ECONNABORTED      = "Software caused connection abort.";
const char* const SSEM_ECONNRESET        = "Connection reset by peer.";
const char* const SSEM_ENOBUFS           = "No buffer space available.";
const char* const SSEM_EISCONN           = "Socket is already connected.";
const char* const SSEM_ENOTCONN          = "Socket is not connected.";
const char* const SSEM_ESHUTDOWN         = "Cannot send or receive after socket is closed.";
const char* const SSEM_ETOOMANYREFS      = "Too many references, cannot splice.";
const char* const SSEM_ETIMEDOUT         = "Connection timed out.";
const char* const SSEM_ECONNREFUSED      = "Connection refused.";
const char* const SSEM_ELOOP             = "Too many levels of symbolic links.";
const char* const SSEM_ENAMETOOLONG      = "File name too long.";
const char* const SSEM_EHOSTDOWN         = "Host is down.";
const char* const SSEM_EHOSTUNREACH      = "No route to host.";
const char* const SSEM_ENOTEMPTY         = "Directory not empty";

///////////////////////////////////////////////////////////////////////////////
// 类型定义

#ifdef _COMPILER_WIN
typedef int socklen_t;
#endif
#ifdef _COMPILER_LINUX
typedef int SOCKET;
#define INVALID_SOCKET ((SOCKET)(-1))
#endif

typedef struct sockaddr_in SockAddr;

// 网络协议类型(UDP|TCP)
enum NET_PROTO_TYPE
{
    NPT_UDP,        // UDP
    NPT_TCP         // TCP
};

// DTP 协议类型(TCP|UTP)
enum DTP_PROTO_TYPE
{
    DPT_TCP,        // TCP
    DPT_UTP         // UTP (UDP Transfer Protocol)
};

// 异步连接的状态
enum ASYNC_CONNECT_STATE
{
    ACS_NONE,       // 尚未发起连接
    ACS_CONNECTING, // 尚未连接完毕，且尚未发生错误
    ACS_CONNECTED,  // 连接已建立成功
    ACS_FAILED      // 连接过程中发生了错误，导致连接失败
};

///////////////////////////////////////////////////////////////////////////////
// 杂项函数

//网络初始化
void networkInitialize();

//网络结束化
void networkFinalize();

//判断网络是否初始化
bool isNetworkInited();

//初始化网络
void ensureNetworkInited();

//获取最后一次出错错误码
int SocketGetLastError();

//获取错误信息
std::string SocketGetErrorMsg(int errorCode);

//取得最后错误的对应信息
std::string SocketGetLastErrMsg();

//关闭socket
void CloseSocket(SOCKET handle);

// 整形IP(主机字节顺序) -> 串型IP
std::string ipToString(UINT ip);

// 串型IP -> 整形IP(主机字节顺序)
UINT stringToIp(const std::string& str);

//取得套接字的“本地地址”
InetAddress getSocketLocalAddr(SOCKET handle);

//取得套接字的“对端地址”
InetAddress getSocketPeerAddr(SOCKET handle);

//取得空闲端口号
// 参数:
//   proto      - 网络协议(UDP,TCP)
//   startPort  - 起始端口号
//   checkTimes - 检测次数
// 返回:
//   空闲端口号 (若失败则返回 0)
int getFreePort(NET_PROTO_TYPE proto, int startPort, int checkTimes);

// 取得本机IP列表
void getLocalIpList(StrList& ipList);

//取得本机IP
std::string getLocalIp();

//域名地址 -> IP地址   若失败，则返回空字符串。
std::string lookupHostAddr(const std::string& host);

//取最后的错误码并抛出异常
void ThrowSocketLastError();

///////////////////////////////////////////////////////////////////////////////
// class InetAddress - IPv4地址类

#pragma pack(1)     // 1字节对齐

// 地址信息
class InetAddress
{
public:
    UINT ip;        // IP   (主机字节顺序)
    WORD port;      // 端口 (主机字节顺序)
public:
    InetAddress() : ip(0), port(0) {}
    InetAddress(UINT _ip, WORD _port) : ip(_ip), port(_port) {}
    InetAddress(const std::string& _ip, WORD _port)
    {
        ip = stringToIp(_ip);
        port = _port;
    }

    InetAddress(const SockAddr& sockAddr)
    {
        ip = ntohl(sockAddr.sin_addr.s_addr);
        port = ntohs(sockAddr.sin_port);
    }

    bool operator == (const InetAddress& rhs) const
        { return (ip == rhs.ip && port == rhs.port); }
    bool operator != (const InetAddress& rhs) const
        { return !((*this) == rhs); }

    SockAddr getSockAddr() const
    {
        SockAddr result;
        memset(&result, 0, sizeof(result));
        result.sin_family = AF_INET;
        result.sin_addr.s_addr = htonl(ip);
        result.sin_port = htons(port);
        return result;
    }

    void clear() { ip = 0; port = 0; }
    bool isEmpty() const { return (ip == 0) && (port == 0); }
    std::string getDisplayStr() const;
};

#pragma pack()

///////////////////////////////////////////////////////////////////////////////
// class Socket - 套接字类

class Socket : noncopyable
{
public:
    friend class BaseTcpServer;

public:
    Socket();
    virtual ~Socket();

    virtual void open();
    virtual void close();

    bool isActive() const { return isActive_; }
    SOCKET getHandle() const { return handle_; }
    InetAddress getLocalAddr() const;
    InetAddress getPeerAddr() const;
    bool isBlockMode() const { return isBlockMode_; }
    void setBlockMode(bool value);
    void setHandle(SOCKET value);

protected:
    void setActive(bool value);
    void setDomain(int value);
    void setType(int value);
    void setProtocol(int value);

    void bind(WORD port);

private:
    void doSetBlockMode(SOCKET handle, bool value);
    void doClose();

protected:
    bool isActive_;     // 套接字是否准备就绪
    SOCKET handle_;     // 套接字句柄
    int domain_;        // 套接字的协议家族 (PF_UNIX, PF_INET, PF_INET6, PF_IPX, ...)
    int type_;          // 套接字类型，必须指定 (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, SOCK_RDM, SOCK_SEQPACKET)
    int protocol_;      // 套接字所用协议，可为0 (IPPROTO_IP, IPPROTO_UDP, IPPROTO_TCP, ...)
    bool isBlockMode_;  // 是否为阻塞模式 (缺省为阻塞模式)
};

///////////////////////////////////////////////////////////////////////////////
// class TcpSocket - TCP 套接字类

class TcpSocket : public Socket
{
public:
    TcpSocket()
    {
        type_ = SOCK_STREAM;
        protocol_ = IPPROTO_TCP;
        isBlockMode_ = false;
    }

    void shutdown(bool closeSend = true, bool closeRecv = true);
};

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpConnection - TCP Connection 基类

class BaseTcpConnection :
    noncopyable,
    public ObjectContext
{
public:
    BaseTcpConnection();
    BaseTcpConnection(SOCKET socketHandle);
    virtual ~BaseTcpConnection() {}

    virtual bool isConnected() const;
    void disconnect();
    void shutdown(bool closeSend = true, bool closeRecv = true);

    void setNoDelay(bool value);
    void setKeepAlive(bool value);

    TcpSocket& getSocket() { return socket_; }
    const TcpSocket& getSocket() const { return socket_; }
    const InetAddress& getLocalAddr() const;
    const InetAddress& getPeerAddr() const;

protected:
    virtual void doDisconnect();

protected:
    int sendBuffer(void *buffer, int size, bool syncMode = false, int timeoutMSecs = -1);
    int recvBuffer(void *buffer, int size, bool syncMode = false, int timeoutMSecs = -1);

private:
    int doSyncSendBuffer(void *buffer, int size, int timeoutMSecs = -1);
    int doSyncRecvBuffer(void *buffer, int size, int timeoutMSecs = -1);
    int doAsyncSendBuffer(void *buffer, int size);
    int doAsyncRecvBuffer(void *buffer, int size);

protected:
    TcpSocket socket_;
private:
    bool isDisconnected_;
    mutable InetAddress localAddr_;
    mutable InetAddress peerAddr_;
};

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpClient - TCP Client 基类

class BaseTcpClient : noncopyable
{
public:
    BaseTcpClient();
    virtual ~BaseTcpClient();

    // 阻塞式连接
    void connect(const std::string& ip, int port);
    // 异步(非阻塞式)连接 (返回 enum ASYNC_CONNECT_STATE)
    int asyncConnect(const std::string& ip, int port, int timeoutMSecs = -1);
    // 检查异步连接的状态 (返回 enum ASYNC_CONNECT_STATE)
    int checkAsyncConnectState(int timeoutMSecs = -1);

    bool isConnected() { return connection_ && connection_->isConnected(); }
    void disconnect();
    BaseTcpConnection& getConnection();

protected:
    virtual BaseTcpConnection* createConnection() { return new BaseTcpConnection(); }
private:
    void ensureConnCreated();
    TcpSocket& getSocket();
protected:
    BaseTcpConnection *connection_;
};

///////////////////////////////////////////////////////////////////////////////
// class BaseTcpServer - TCP Server 基类

class BaseTcpServer :
    noncopyable,
    public ObjectContext
{
public:
    friend class TcpListenerThread;
    typedef std::function<void (BaseTcpServer *tcpServer, SOCKET socketHandle,
        BaseTcpConnection*& connection)> TcpSvrCreateConnCallback;
    typedef std::function<void (BaseTcpServer *tcpServer,
        BaseTcpConnection *connection)> TcpSvrAcceptConnCallback;

public:
    enum { LISTEN_QUEUE_SIZE = 30 };   // TCP监听队列长度

public:
    BaseTcpServer();
    virtual ~BaseTcpServer();

    virtual void open();
    virtual void close();

    bool isActive() const { return socket_.isActive(); }
    void setActive(bool value);

    WORD getLocalPort() const { return localPort_; }
    void setLocalPort(WORD value);

    const TcpSocket& getSocket() const { return socket_; }

    void setCreateConnCallback(const TcpSvrCreateConnCallback& callback);
    void setAcceptConnCallback(const TcpSvrAcceptConnCallback& callback);

protected:
    virtual void startListenerThread();
    virtual void stopListenerThread();

    virtual BaseTcpConnection* createConnection(SOCKET socketHandle);
    virtual void acceptConnection(BaseTcpConnection *connection);

private:
    TcpSocket socket_;
    WORD localPort_;
    TcpListenerThread *listenerThread_;
    TcpSvrCreateConnCallback onCreateConn_;
    TcpSvrAcceptConnCallback onAcceptConn_;
};

///////////////////////////////////////////////////////////////////////////////
// class ListenerThread - 监听线程类

class ListenerThread : public Thread
{
public:
    ListenerThread()
    {
#ifdef _COMPILER_WIN
        setPriority(THREAD_PRI_HIGHEST);
#endif
#ifdef _COMPILER_LINUX
        setPolicy(THREAD_POL_RR);
        setPriority(THREAD_PRI_HIGH);
#endif
    }
    virtual ~ListenerThread() {}

protected:
    virtual void execute() {}
};


///////////////////////////////////////////////////////////////////////////////
// class TcpListenerThread - TCP服务器监听线程类

class TcpListenerThread : public ListenerThread
{
public:
    explicit TcpListenerThread(BaseTcpServer *tcpServer);
protected:
    virtual void execute();
private:
    BaseTcpServer *tcpServer_;
};

///////////////////////////////////////////////////////////////////////////////

#endif 