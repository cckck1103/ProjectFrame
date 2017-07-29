///////////////////////////////////////////////////////////////////////////////
// 文件名称: BaseApplication.cpp
// 功能描述: 服务器应用主单元
///////////////////////////////////////////////////////////////////////////////

#include "BaseApplication.h"
#include "ErrMsgs.h"
#include "LogManager.h"


///////////////////////////////////////////////////////////////////////////////
// 全局变量定义

#ifdef _COMPILER_LINUX
// 用于进程退出时长跳转
static sigjmp_buf procExitJmpBuf;
#endif

// 用于内存不足的情况下程序退出
static char *reservedMemoryForExit;

///////////////////////////////////////////////////////////////////////////////
// 信号处理器

#ifdef _COMPILER_LINUX
//-----------------------------------------------------------------------------
// 描述: 正常退出程序 信号处理器
//-----------------------------------------------------------------------------
void exitProgramSignalHandler(BaseApplication* baseApp,int sigNo)
{
	ASSERT_X(baseApp != NULL);

    static bool isInHandler = false;
    if (isInHandler) return;
	if (!baseApp) return;
    isInHandler = true;

	// 停止主线程循环
	baseApp->setTerminated(true);
	
	INFO_LOG(SEM_SIG_TERM, sigNo);

    siglongjmp(procExitJmpBuf, 1);
}

//-----------------------------------------------------------------------------
// 描述: 致命非法操作 信号处理器
//-----------------------------------------------------------------------------
void fatalErrorSignalHandler(BaseApplication* baseApp, int sigNo)
{
	ASSERT_X(baseApp != NULL);

    static bool isInHandler = false;
    if (isInHandler) return;
	if (!baseApp) return;
    isInHandler = true;
	
    // 停止主线程循环
	baseApp->setTerminated(true);

	INFO_LOG(SEM_SIG_FATAL_ERROR, sigNo);
    abort();
}
#endif

//-----------------------------------------------------------------------------
// 描述: 用户信号处理器
//-----------------------------------------------------------------------------
void userSignalHandler(BaseApplication* baseApp, int sigNo)
{
	ASSERT_X(baseApp != NULL);
	if (!baseApp) return;

    const CallbackList<UserSignalHandlerCallback>& callbackList = baseApp->onUserSignal_;

    for (int i = 0; i < callbackList.getCount(); i++)
    {
        const UserSignalHandlerCallback& callback = callbackList.getItem(i);
        if (callback)
            callback(sigNo);
    }
}

//-----------------------------------------------------------------------------
// 描述: 内存不足错误处理器
// 备注:
//   若未安装错误处理器(set_new_handler)，则当 new 操作失败时抛出 bad_alloc 异常；
//   而安装错误处理器后，new 操作将不再抛出异常，而是调用处理器函数。
//-----------------------------------------------------------------------------
void outOfMemoryHandler()
{
    static bool isInHandler = false;
    if (isInHandler) return;
    isInHandler = true;

    // 释放保留内存，以免程序退出过程中再次出现内存不足
    delete[] reservedMemoryForExit;
    reservedMemoryForExit = NULL;

	ERROR_LOG(SEM_OUT_OF_MEMORY);
    abort();
}

///////////////////////////////////////////////////////////////////////////////
// class BaseApplication

BaseApplication::BaseApplication() :
    appStartTime_(time(NULL)),
    initialized_(false),
    terminated_(false)
{
  
}

BaseApplication::~BaseApplication()
{
    Finalize();
}

//-----------------------------------------------------------------------------
// 描述: 解释命令行参数
// 返回:
//   true  - 允许程序继执行
//   false - 程序应退出 (比如命令行参数不正确)
//-----------------------------------------------------------------------------
bool BaseApplication::parseArguments(int argc, char *argv[])
{
    // 先记录命令行参数
    argList_.clear();
    for (int i = 1; i < argc; i++)
        argList_.add(argv[i]);

	return true;
}

//-----------------------------------------------------------------------------
// 描述: 应用程序初始化 (若初始化失败则抛出异常)
//-----------------------------------------------------------------------------
void BaseApplication::Initialize()
{
    try
    {
#ifdef _COMPILER_LINUX
        // 在初始化阶段要屏蔽退出信号
        SignalMasker sigMasker(true);
        sigMasker.setSignals(1, SIGTERM);
        sigMasker.block();
#endif

        networkInitialize();
        initExeName();
        checkMultiInstance();
        initSignals();
        initNewOperHandler();
		OnInitialize();
        initialized_ = true;
    }
    catch (Exception& e)
    {
        openTerminal();
        doFinalize();
		ThrowException(e.makeLogStr().c_str());
    }
}

//-----------------------------------------------------------------------------
// 描述: 应用程序结束化
//-----------------------------------------------------------------------------
void BaseApplication::Finalize()
{
    if (initialized_)
    {
        openTerminal();
		doFinalize();
        initialized_ = false;
    }
}

//-----------------------------------------------------------------------------
// 描述: 开始运行应用程序
//-----------------------------------------------------------------------------
void BaseApplication::run()
{
#ifdef _COMPILER_LINUX
    // 进程被终止时长跳转到此处并立即返回
    if (sigsetjmp(procExitJmpBuf, 0)) return;
#endif

	OnRun();
}

//-----------------------------------------------------------------------------
// 描述: 取得可执行文件所在的路径
//-----------------------------------------------------------------------------
std::string BaseApplication::getExePath()
{
    return extractFilePath(exeName_);
}

//-----------------------------------------------------------------------------
// 描述: 取得命令行参数字符串 (index: 0-based)
//-----------------------------------------------------------------------------
std::string BaseApplication::getArgString(int index)
{
    if (index >= 0 && index < (int)argList_.getCount())
        return argList_[index];
    else
        return "";
}

//-----------------------------------------------------------------------------
// 描述: 注册用户信号处理器
//-----------------------------------------------------------------------------
void BaseApplication::registerUserSignalHandler(const UserSignalHandlerCallback& callback)
{
    onUserSignal_.registerCallback(callback);
}

//-----------------------------------------------------------------------------
// 描述: 检查是否运行了多个程序实体
//-----------------------------------------------------------------------------
void BaseApplication::checkMultiInstance()
{
    if (multInstance_) return;

#ifdef _COMPILER_WIN
    CreateMutexA(NULL, false, getExeName().c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        ThrowException(SEM_ALREADY_RUNNING);
#endif
#ifdef _COMPILER_LINUX
    umask(0);
    int fd = open(getExeName().c_str(), O_RDONLY, 0666);
    if (fd >= 0 && flock(fd, LOCK_EX | LOCK_NB) != 0)
        ThrowException(SEM_ALREADY_RUNNING);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 取得自身文件的全名，并初始化 exeName_
//-----------------------------------------------------------------------------
void BaseApplication::initExeName()
{
    exeName_ = getAppExeName();
}


//-----------------------------------------------------------------------------
// 描述: 初始化信号 (信号的安装、忽略等)
//
//  信号名称    值                         信号说明
// ---------  ----  -----------------------------------------------------------
// # SIGHUP    1    本信号在用户终端连接(正常或非正常)结束时发出，通常是在终端的控制进程
//                  结束时，通知同一 session 内的各个作业，这时它们与控制终端不再关联。
// # SIGINT    2    程序终止(interrupt)信号，在用户键入INTR字符(通常是Ctrl-C)时发出。
// # SIGQUIT   3    和 SIGINT 类似，但由QUIT字符 (通常是 Ctrl-\) 来控制。进程在因收到
//                  此信号退出时会产生core文件，在这个意义上类似于一个程序错误信号。
// # SIGILL    4    执行了非法指令。通常是因为可执行文件本身出现错误，或者试图执行数据段。
//                  堆栈溢出时也有可能产生这个信号。
// # SIGTRAP   5    由断点指令或其它 trap 指令产生。由 debugger 使用。
// # SIGABRT   6    程序自己发现错误并调用 abort 时产生。
// # SIGIOT    6    在PDP-11上由iot指令产生。在其它机器上和 SIGABRT 一样。
// # SIGBUS    7    非法地址，包括内存地址对齐(alignment)出错。eg: 访问一个四个字长的
//                  整数，但其地址不是 4 的倍数。
// # SIGFPE    8    在发生致命的算术运算错误时发出。不仅包括浮点运算错误，还包括溢出及除
//                  数为 0 等其它所有的算术的错误。
// # SIGKILL   9    用来立即结束程序的运行。本信号不能被阻塞、处理和忽略。
// # SIGUSR1   10   留给用户使用。
// # SIGSEGV   11   试图访问未分配给自己的内存，或试图往没有写权限的内存地址写数据。
// # SIGUSR2   12   留给用户使用。
// # SIGPIPE   13   管道破裂(broken pipe)，写一个没有读端口的管道。
// # SIGALRM   14   时钟定时信号，计算的是实际的时间或时钟时间。alarm 函数使用该信号。
// # SIGTERM   15   程序结束(terminate)信号，与 SIGKILL 不同的是该信号可以被阻塞和处理。
//                  通常用来要求程序自己正常退出。shell 命令 kill 缺省产生这个信号。
// # SIGSTKFLT 16   协处理器堆栈错误(stack fault)。
// # SIGCHLD   17   子进程结束时，父进程会收到这个信号。
// # SIGCONT   18   让一个停止(stopped)的进程继续执行。本信号不能被阻塞。可以用一个
//                  handler 来让程序在由 stopped 状态变为继续执行时完成特定的工作。例如
//                  重新显示提示符。
// # SIGSTOP   19   停止(stopped)进程的执行。注意它和terminate以及interrupt的区别:
//                  该进程还未结束，只是暂停执行。本信号不能被阻塞、处理或忽略。
// # SIGTSTP   20   停止进程的运行，但该信号可以被处理和忽略。用户键入SUSP字符时(通常是^Z)
//                  发出这个信号。
// # SIGTTIN   21   当后台作业要从用户终端读数据时，该作业中的所有进程会收到此信号。缺省时
//                  这些进程会停止执行。
// # SIGTTOU   22   类似于SIGTTIN，但在写终端(或修改终端模式)时收到。
// # SIGURG    23   有 "紧急" 数据或带外(out-of-band) 数据到达 socket 时产生。
// # SIGXCPU   24   超过CPU时间资源限制。这个限制可以由getrlimit/setrlimit来读取和改变。
// # SIGXFSZ   25   超过文件大小资源限制。
// # SIGVTALRM 26   虚拟时钟信号。类似于 SIGALRM，但是计算的是该进程占用的CPU时间。
// # SIGPROF   27   类似于SIGALRM/SIGVTALRM，但包括该进程用的CPU时间以及系统调用的时间。
// # SIGWINCH  28   终端视窗的改变时发出。
// # SIGIO     29   文件描述符准备就绪，可以开始进行输入/输出操作。
// # SIGPWR    30   Power failure.
// # SIGSYS    31   非法的系统调用。
//-----------------------------------------------------------------------------
void BaseApplication::initSignals()
{
#ifdef _COMPILER_WIN
#endif
#ifdef _COMPILER_LINUX
    UINT i;

    // 忽略某些信号
    int ignoreSignals[] = {SIGHUP, SIGINT, SIGQUIT, SIGPIPE, SIGTSTP, SIGTTIN,
        SIGTTOU, SIGXCPU, SIGCHLD, SIGPWR, SIGALRM, SIGVTALRM, SIGIO};
    for (i = 0; i < sizeof(ignoreSignals)/sizeof(int); i++)
        signal(ignoreSignals[i], SIG_IGN);

    // 安装致命非法操作信号处理器
    int fatalSignals[] = {SIGILL, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGPROF, SIGSYS};
    for (i = 0; i < sizeof(fatalSignals)/sizeof(int); i++)
    {
        auto f= std::bind(fatalErrorSignalHandler,this,std::placeholders::_1);  
        
        signal(fatalSignals[i],(__sighandler_t)&f);
    }

    // 安装正常退出信号处理器
    int exitSignals[] = {SIGTERM/*, SIGABRT*/};
    for (i = 0; i < sizeof(exitSignals)/sizeof(int); i++)
    {
        auto f= std::bind(exitProgramSignalHandler,this,std::placeholders::_1);  
        signal(exitSignals[i], (__sighandler_t)&f);
    }

    // 安装用户信号处理器
    int userSignals[] = {SIGUSR1, SIGUSR2};
    for (i = 0; i < sizeof(userSignals)/sizeof(int); i++)
    {
        auto f= std::bind(userSignalHandler,this,std::placeholders::_1);  
        signal(userSignals[i],(__sighandler_t)&f);
    }
      
#endif
}

//-----------------------------------------------------------------------------
// 描述: 初始化 new 操作符的错误处理函数
//-----------------------------------------------------------------------------
void BaseApplication::initNewOperHandler()
{
    const int RESERVED_MEM_SIZE = 1024*1024*2;     // 2M

    std::set_new_handler(outOfMemoryHandler);

    // 用于内存不足的情况下程序退出
    reservedMemoryForExit = new char[RESERVED_MEM_SIZE];
}

//-----------------------------------------------------------------------------

#ifdef _COMPILER_LINUX
static int s_oldStdInFd = -1;
static int s_oldStdOutFd = -1;
static int s_oldStdErrFd = -1;
#endif

//-----------------------------------------------------------------------------
// 描述: 打开终端
//-----------------------------------------------------------------------------
void BaseApplication::openTerminal()
{
#ifdef _COMPILER_LINUX
    if (s_oldStdInFd != -1)
        dup2(s_oldStdInFd, 0);

    if (s_oldStdOutFd != -1)
        dup2(s_oldStdOutFd, 1);

    if (s_oldStdErrFd != -1)
        dup2(s_oldStdErrFd, 2);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 关闭终端
//-----------------------------------------------------------------------------
void BaseApplication::closeTerminal()
{
#ifdef _COMPILER_LINUX
    s_oldStdInFd = dup(0);
    s_oldStdOutFd = dup(1);
    s_oldStdErrFd = dup(2);

    int fd = open("/dev/null", O_RDWR);
    if (fd != -1)
    {
        dup2(fd, 0);  // stdin
        dup2(fd, 1);  // stdout
        dup2(fd, 2);  // stderr

        if (fd > 2)
            close(fd);
    }
#endif
}

//-----------------------------------------------------------------------------
// 描述: 应用程序结束化 (不检查 initialized_ 标志)
//-----------------------------------------------------------------------------
void BaseApplication::doFinalize()
{
	try { OnRelease(); } catch (...) {}
    try { networkFinalize(); } catch (...) {}
}
