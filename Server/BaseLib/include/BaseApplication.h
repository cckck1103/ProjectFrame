///////////////////////////////////////////////////////////////////////////////
// Application.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _BASE_APPLICATION_H_
#define _BASE_APPLICATION_H_

#include "Options.h"
#include "UtilClass.h"
#include "ServiceThread.h"
#include "SysUtils.h"
#include "BaseSocket.h"
#include "Exceptions.h"
#include "TCPServer.h"
#include "Timers.h"
#include "NonCopyable.h"

#ifdef _COMPILER_WIN
#include <stdarg.h>
#include <windows.h>
#endif

#ifdef _COMPILER_LINUX
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// 类型定义


// 用户信号处理回调
typedef std::function<void (int signalNumber)> UserSignalHandlerCallback;

///////////////////////////////////////////////////////////////////////////////
// class BaseApplication - 应用程序基础类
//
// 说明:
// 1. 此类是整个服务程序的主框架，全局单例对象(Application)在程序启动时即被创建；
// 2. 一般来说，服务程序是由外部发出命令(kill)而退出的。程序收到kill退出命令后
//    (此时当前执行点一定在 App().run() 中)，会触发 exitProgramSignalHandler
//    信号处理器，进而利用 longjmp 方法使执行点模拟从 run() 中退出，继而执行 finalize。
// 3. 若程序发生致命的非法操作错误，会先触发 fatalErrorSignalHandler 信号处理器，
//    然后同样按照正常的析构顺序退出。
// 4. 若程序内部想正常退出，不推荐使用exit函数。而是 setTeraminted(true).
//    这样才能让程序按照正常的析构顺序退出。

class BaseApplication : noncopyable
{
public:
	BaseApplication();
    virtual ~BaseApplication();
  
	virtual void OnInitialize()  = 0;
	virtual void OnRelease() = 0;
	virtual void OnRun() = 0;

	bool parseArguments(int argc, char *argv[]);
	void Initialize();
	void Finalize();
	//主循环
	void run();

    void setTerminated(bool value) { terminated_ = value; }
    bool isTerminated() { return terminated_; }

	void setMultiInstance(bool value) { multInstance_ = value; };

    // 取得可执行文件的全名(含绝对路径)
    std::string getExeName() { return exeName_; }
    // 取得可执行文件所在的路径
	std::string getExePath();
    // 取得命令行参数个数(首个参数为程序路径文件名)
    int getArgCount() { return argList_.getCount(); }
    // 取得命令行参数字符串 (index: 0-based)
	std::string getArgString(int index);
    // 取得程序启动时的时间
    time_t getAppStartTime() { return appStartTime_; }

    // 注册用户信号处理器
    void registerUserSignalHandler(const UserSignalHandlerCallback& callback);

private:
    void checkMultiInstance();
    void initExeName();
    void initSignals();
    void initNewOperHandler();
    void openTerminal();
    void closeTerminal();
    void doFinalize();

private:
    StrList argList_;										// 命令行参数 (不包括程序名 argv[0])
    std::string exeName_;									// 可执行文件的全名(含绝对路径)
    time_t appStartTime_;									// 程序启动时的时间
    bool initialized_;										// 是否成功初始化
    bool terminated_;										// 是否应退出的标志
	bool multInstance_;										// 是否允许多个实例运行
    CallbackList<UserSignalHandlerCallback> onUserSignal_;  // 用户信号处理回调
	
    friend void userSignalHandler(BaseApplication* baseApp, int sigNo);
};

#endif // _APPLICATION_H_
