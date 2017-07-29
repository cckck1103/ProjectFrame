///////////////////////////////////////////////////////////////////////////////
// 文件名称: ServiceThread.cpp
// 功能描述: 线程类
///////////////////////////////////////////////////////////////////////////////

#include "ServiceThread.h"
#include "SysUtils.h"
#include "Exceptions.h"
#include "GlobalDefs.h"
#include "LogManager.h"

using namespace std::placeholders;
///////////////////////////////////////////////////////////////////////////////
// class ThreadImpl

ThreadImpl::ThreadImpl(Thread *thread) :
    thread_(*thread),
    threadId_(0),
    isExecuting_(false),
    isRunCalled_(false),
    termTime_(0),
    isAutoDelete_(false),
    terminated_(false),
    isSleepInterrupted_(false),
    returnValue_(0)
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void ThreadImpl::terminate()
{
    if (!terminated_)
    {
        beforeTerminate();
        termTime_ = (int)time(NULL);
        terminated_ = true;
    }
}

//-----------------------------------------------------------------------------
// 描述: 取得从调用 terminate 到当前共经过多少时间(秒)
//-----------------------------------------------------------------------------
int ThreadImpl::getTermElapsedSecs() const
{
    int result = 0;

    // 如果已经通知退出，但线程还活着
    if (terminated_ && threadId_ != 0)
    {
        result = (int)(time(NULL) - termTime_);
    }

    return result;
}

//-----------------------------------------------------------------------------
// 描述: 设置是否 terminate
//-----------------------------------------------------------------------------
void ThreadImpl::setTerminated(bool value)
{
    if (value != terminated_)
    {
        if (value)
            terminate();
        else
        {
            terminated_ = false;
            termTime_ = 0;
        }
    }
}

//-----------------------------------------------------------------------------
// 描述: 进入睡眠状态 (睡眠过程中会检测 terminated_ 的状态)
// 参数:
//   seconds - 睡眠的秒数，可为小数，可精确到毫秒
// 注意:
//   由于将睡眠时间分成了若干份，每次睡眠时间的小误差累加起来将扩大总误差。
//-----------------------------------------------------------------------------
void ThreadImpl::sleep(double seconds)
{
    const double SLEEP_INTERVAL = 0.5;      // 每次睡眠的时间(秒)
    double onceSecs;

    isSleepInterrupted_ = false;

    while (!isTerminated() && seconds > 0 && !isSleepInterrupted_)
    {
        onceSecs = (seconds >= SLEEP_INTERVAL ? SLEEP_INTERVAL : seconds);
        seconds -= onceSecs;

        sleepSeconds(onceSecs, true);
    }
}

//-----------------------------------------------------------------------------

void ThreadImpl::execute()
{
    thread_.execute();
}

//-----------------------------------------------------------------------------

void ThreadImpl::afterExecute()
{
    thread_.afterExecute();
}

//-----------------------------------------------------------------------------

void ThreadImpl::beforeTerminate()
{
    thread_.beforeTerminate();
}

//-----------------------------------------------------------------------------

void ThreadImpl::beforeKill()
{
    thread_.beforeKill();
}

//-----------------------------------------------------------------------------
// 描述: 如果线程已运行，则抛出异常
//-----------------------------------------------------------------------------
void ThreadImpl::checkNotRunning()
{
    if (isRunCalled_)
        ThrowThreadException(SEM_THREAD_RUN_ONCE);
}

///////////////////////////////////////////////////////////////////////////////
// class WinThreadImpl

#ifdef _COMPILER_WIN

//-----------------------------------------------------------------------------
// 描述: 线程执行函数
// 参数:
//   param - 线程参数，此处指向 WinThreadImpl 对象
//-----------------------------------------------------------------------------
UINT __stdcall threadExecProc(void *param)
{
    WinThreadImpl *threadImpl = (WinThreadImpl*)param;
    int returnValue = 0;

    {
        threadImpl->setExecuting(true);

        // 对象 finalizer 进行自动化善后工作
        struct sAutoFinalizer
        {
            WinThreadImpl *threadImpl_;
			sAutoFinalizer(WinThreadImpl *threadImpl) { threadImpl_ = threadImpl; }
            ~sAutoFinalizer()
            {
                threadImpl_->setExecuting(false);
                if (threadImpl_->isAutoDelete())
                    delete threadImpl_->getThread();
            }
        } finalizer(threadImpl);

        if (!threadImpl->terminated_)
        {
            {
                AutoFinalizer finalizer(std::bind(&WinThreadImpl::afterExecute, threadImpl));

                try { threadImpl->execute(); } catch (Exception&) {}
            }

            // 记下线程返回值
            returnValue = threadImpl->returnValue_;
        }
    }

    // 注意: 请查阅 _endthreadex 和 ExitThread 的区别
    _endthreadex(returnValue);
    //ExitThread(returnValue);

    return returnValue;
}

//-----------------------------------------------------------------------------
// 描述: 构造函数
//-----------------------------------------------------------------------------
WinThreadImpl::WinThreadImpl(Thread *thread) :
    ThreadImpl(thread),
    handle_(0),
    priority_(THREAD_PRI_NORMAL)
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 析构函数
//-----------------------------------------------------------------------------
WinThreadImpl::~WinThreadImpl()
{
    if (threadId_ != 0)
    {
        if (isExecuting_)
            terminate();
        if (!isAutoDelete_)
            waitFor();
    }

    if (threadId_ != 0)
    {
        CloseHandle(handle_);
    }
}

//-----------------------------------------------------------------------------
// 描述: 创建线程并执行
// 注意: 此成员方法在对象声明周期中只可调用一次。
//-----------------------------------------------------------------------------
void WinThreadImpl::run()
{
    checkNotRunning();
    isRunCalled_ = true;

    // 注意: 请查阅 CRT::_beginthreadex 和 API::CreateThread 的区别，前者兼容于CRT。
    handle_ = (HANDLE)_beginthreadex(NULL, 0, threadExecProc, (LPVOID)this,
        CREATE_SUSPENDED, (UINT*)&threadId_);
    /*
    handle_ = CreateThread(NULL, 0, threadExecProc, (LPVOID)this, CREATE_SUSPENDED, (LPDWORD)&threadId_);
    */

    checkThreadError(handle_ != 0);

    // 设置线程优先级
    if (priority_ != THREAD_PRI_NORMAL)
        setPriority(priority_);

    ::ResumeThread(handle_);
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void WinThreadImpl::terminate()
{
    ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死线程
// 注意:
//   1. 调用此函数后，对线程类对象的一切操作皆不可用(terminate(); waitFor(); delete thread; 等)。
//   2. 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源 (还未来得及解锁
//      便被杀了)，所以重要资源的释放工作必须在 beforeKill 中进行。
//   3. Windows 下强杀线程，线程执行过程中的栈对象不会析构。
//-----------------------------------------------------------------------------
void WinThreadImpl::kill()
{
    if (threadId_ != 0)
    {
        beforeKill();

        threadId_ = 0;
        TerminateThread(handle_, 0);
        ::CloseHandle(handle_);
    }

    delete (Thread*)&thread_;
}

//-----------------------------------------------------------------------------
// 描述: 等待线程退出
// 返回: 线程返回值
//-----------------------------------------------------------------------------
int WinThreadImpl::waitFor()
{
    ASSERT_X(isAutoDelete_ == false);

    if (threadId_ != 0)
    {
        WaitForSingleObject(handle_, INFINITE);
        GetExitCodeThread(handle_, (LPDWORD)&returnValue_);
    }

    return returnValue_;
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的优先级
//-----------------------------------------------------------------------------
void WinThreadImpl::setPriority(int value)
{
    int priorities[7] = {
        THREAD_PRIORITY_IDLE,
        THREAD_PRIORITY_LOWEST,
        THREAD_PRIORITY_BELOW_NORMAL,
        THREAD_PRIORITY_NORMAL,
        THREAD_PRIORITY_ABOVE_NORMAL,
        THREAD_PRIORITY_HIGHEST,
        THREAD_PRIORITY_TIME_CRITICAL
    };

    value = (int)ensureRange((int)value, 0, 6);
    priority_ = value;
    if (threadId_ != 0)
        SetThreadPriority(handle_, priorities[value]);
}

//-----------------------------------------------------------------------------
// 描述: 线程错误处理
//-----------------------------------------------------------------------------
void WinThreadImpl::checkThreadError(bool success)
{
    if (!success)
    {
        std::string errMsg = sysErrorMessage(GetLastError());
		ERROR_LOG(errMsg.c_str());
        ThrowThreadException(errMsg.c_str());
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class LinuxThreadImpl

#ifdef _COMPILER_LINUX

//-----------------------------------------------------------------------------
// 描述: 线程清理函数
// 参数:
//   param - 线程参数，此处指向 LinuxThreadImpl 对象
//-----------------------------------------------------------------------------
void threadFinalProc(void *param)
{
    LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;

    threadImpl->setExecuting(false);
    if (threadImpl->isAutoDelete())
        delete threadImpl->getThread();
}

//-----------------------------------------------------------------------------
// 描述: 线程执行函数
// 参数:
//   param - 线程参数，此处指向 LinuxThreadImpl 对象
//-----------------------------------------------------------------------------
void* threadExecProc(void *param)
{
    LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;
    int returnValue = 0;

    {
        // 等待线程对象准备就绪
        threadImpl->execSem_->wait();
        delete threadImpl->execSem_;
        threadImpl->execSem_ = NULL;

        // 获取线程内核ID
        threadImpl->threadId_ = getCurThreadId();

        threadImpl->setExecuting(true);

        // 线程对 cancel 信号的响应方式有三种: (1)不响应 (2)推迟到取消点再响应 (3)尽量立即响应。
        // 此处设置线程为第(3)种方式，即可马上被 cancel 信号终止。
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        // 注册清理函数
        pthread_cleanup_push(threadFinalProc, threadImpl);

        if (!threadImpl->terminated_)
        {
            // 线程在执行过程中若收到 cancel 信号，会抛出一个异常(究竟是什么类型的异常？)，
            // 此异常千万不可去阻拦它( try{}catch(...){} )，系统能侦测出此异常是否被彻底阻拦，
            // 若是，则会发出 SIGABRT 信号，并在终端输出 "FATAL: exception not rethrown"。
            // 所以此处的策略是只阻拦 Exception 异常(所有异常皆从 Exception 继承)。
            // 在 thread->execute() 的执行过程中，用户应该注意如下事项:
            // 1. 请参照此处的做法去拦截异常，而切不可阻拦所有类型的异常( 即catch(...) );
            // 2. 不可抛出 Exception 及其子类之外的异常。假如抛出一个整数( 如 throw 5; )，
            //    系统会因为没有此异常的处理程序而调用 abort。(尽管如此，threadFinalProc
            //    仍会象 pthread_cleanup_push 所承诺的那样被执行到。)
            {
                AutoFinalizer finalizer(std::bind(&LinuxThreadImpl::afterExecute, threadImpl));
                try { threadImpl->execute(); } catch (Exception& e) {}
            }

            // 记下线程返回值
            returnValue = threadImpl->returnValue_;
        }

        pthread_cleanup_pop(1);

        // 屏蔽 cancel 信号，出了当前 scope 后将不可被强行终止
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    pthread_exit((void*)returnValue);
    return NULL;
}

//-----------------------------------------------------------------------------
// 描述: 构造函数
//-----------------------------------------------------------------------------
LinuxThreadImpl::LinuxThreadImpl(Thread *thread) :
    ThreadImpl(thread),
    pthreadId_(0),
    policy_(THREAD_POL_DEFAULT),
    priority_(THREAD_PRI_DEFAULT),
    execSem_(NULL)
{
    // nothing
}

//-----------------------------------------------------------------------------
// 描述: 析构函数
//-----------------------------------------------------------------------------
LinuxThreadImpl::~LinuxThreadImpl()
{
    delete execSem_;
    execSem_ = NULL;

    if (pthreadId_ != 0)
    {
        if (isExecuting_)
            terminate();
        if (!isAutoDelete_)
            waitFor();
    }

    if (pthreadId_ != 0)
    {
        pthread_detach(pthreadId_);
    }
}

//-----------------------------------------------------------------------------
// 描述: 创建线程并执行
// 注意: 此成员方法在对象声明周期中只可调用一次。
//-----------------------------------------------------------------------------
void LinuxThreadImpl::run()
{
    checkNotRunning();
    isRunCalled_ = true;

    delete execSem_;
    execSem_ = new Semaphore(0);

    // 创建线程
    checkThreadError(pthread_create((pthread_t*)&pthreadId_, NULL, threadExecProc, (void*)this));

    // 设置线程调度策略
    if (policy_ != THREAD_POL_DEFAULT)
        setPolicy(policy_);
    // 设置线程优先级
    if (priority_ != THREAD_PRI_DEFAULT)
        setPriority(priority_);

    // 线程对象已准备就绪，线程函数可以开始运行
    execSem_->increase();
}

//-----------------------------------------------------------------------------
// 描述: 通知线程退出
//-----------------------------------------------------------------------------
void LinuxThreadImpl::terminate()
{
    ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// 描述: 强行杀死线程
// 注意:
//   1. 调用此函数后，线程对象即被销毁，对线程类对象的一切操作皆不可用(terminate();
//      waitFor(); delete thread; 等)。
//   2. 在杀死线程前，isAutoDelete_ 会自动设为 true，以便对象能自动释放。
//   3. 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源 (还未来得及解锁
//      便被杀了)，所以重要资源的释放工作必须在 beforeKill 中进行。
//   4. pthread 没有规定在线程收到 cancel 信号后是否进行 C++ stack unwinding，也就是说栈对象
//      的析构函数不一定会执行。实验表明，在 RedHat AS (glibc-2.3.4) 下会进行 stack unwinding，
//      而在 Debian 4.0 (glibc-2.3.6) 下则不会。
//-----------------------------------------------------------------------------
void LinuxThreadImpl::kill()
{
    if (pthreadId_ != 0)
    {
        beforeKill();

        if (isExecuting_)
        {
            setAutoDelete(true);
            pthread_cancel(pthreadId_);
            return;
        }
    }

    delete this;
}

//-----------------------------------------------------------------------------
// 描述: 等待线程退出
// 返回: 线程返回值
//-----------------------------------------------------------------------------
int LinuxThreadImpl::waitFor()
{
    ASSERT_X(isAutoDelete_ == false);

    pthread_t threadId = pthreadId_;

    if (pthreadId_ != 0)
    {
        pthreadId_ = 0;
        threadId_ = 0;

        void *valuePtr = NULL;
        checkThreadError(pthread_join(threadId, &valuePtr));
        returnValue_ = (intptr_t)valuePtr;
    }

    return returnValue_;
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的调度策略
//-----------------------------------------------------------------------------
void LinuxThreadImpl::setPolicy(int value)
{
    if (value != THREAD_POL_DEFAULT &&
        value != THREAD_POL_RR &&
        value != THREAD_POL_FIFO)
    {
        value = THREAD_POL_DEFAULT;
    }

    policy_ = value;

    if (pthreadId_ != 0)
    {
        struct sched_param param;
        param.sched_priority = priority_;
        pthread_setschedparam(pthreadId_, policy_, &param);
    }
}

//-----------------------------------------------------------------------------
// 描述: 设置线程的优先级
//-----------------------------------------------------------------------------
void LinuxThreadImpl::setPriority(int value)
{
    if (value < THREAD_PRI_MIN || value > THREAD_PRI_MAX)
        value = THREAD_PRI_DEFAULT;

    priority_ = value;

    if (pthreadId_ != 0)
    {
        struct sched_param param;
        param.sched_priority = priority_;
        pthread_setschedparam(pthreadId_, policy_, &param);
    }
}

//-----------------------------------------------------------------------------
// 描述: 线程错误处理
//-----------------------------------------------------------------------------
void LinuxThreadImpl::checkThreadError(int errorCode)
{
    if (errorCode != 0)
    {
        std::string errMsg = sysErrorMessage(errorCode);
		ERROR_LOG(errMsg.c_str());
        ThrowThreadException(errMsg.c_str());
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class Thread

//-----------------------------------------------------------------------------
// 描述: 创建一个线程并马上执行
//-----------------------------------------------------------------------------
Thread* Thread::create(const ThreadProc& threadProc)
{
    Thread *thread = new Thread();

    thread->setAutoDelete(true);
    thread->threadProc_ = threadProc;
    thread->run();

    return thread;
}

///////////////////////////////////////////////////////////////////////////////
// class ThreadList

ThreadList::ThreadList() :
    items_(false, false)
{
    // nothing
}

ThreadList::~ThreadList()
{
    // nothing
}

//-----------------------------------------------------------------------------

void ThreadList::add(Thread *thread)
{
    AutoLocker locker(mutex_);
    items_.add(thread, false);
}

//-----------------------------------------------------------------------------

void ThreadList::remove(Thread *thread)
{
    AutoLocker locker(mutex_);
    items_.remove(thread);
}

//-----------------------------------------------------------------------------

bool ThreadList::exists(Thread *thread)
{
    AutoLocker locker(mutex_);
    return items_.exists(thread);
}

//-----------------------------------------------------------------------------

void ThreadList::clear()
{
    AutoLocker locker(mutex_);
    items_.clear();
}

//-----------------------------------------------------------------------------
// 描述: 通知所有线程退出
//-----------------------------------------------------------------------------
void ThreadList::terminateAllThreads()
{
    AutoLocker locker(mutex_);

    for (int i = 0; i < items_.getCount(); i++)
    {
        Thread *thread = items_[i];
        thread->terminate();
    }
}

//-----------------------------------------------------------------------------
// 描述: 等待所有线程退出
// 参数:
//   maxWaitSecs - 最长等待时间(秒) (为 -1 表示无限等待)
//   killedCount - 传回最终强杀了多少个线程
// 注意:
//   此函数要求列表中各线程在退出时自动销毁自己并从列表中移除。
//-----------------------------------------------------------------------------
void ThreadList::waitForAllThreads(int maxWaitSecs, int *killedCount)
{
    const double SLEEP_INTERVAL = 0.1;  // (秒)
    double waitSecs = 0;
    int killedThreadCount = 0;

    // 通知所有线程退出
    terminateAllThreads();

    // 等待线程退出
    while (maxWaitSecs < 0 || waitSecs < maxWaitSecs)
    {
        if (items_.getCount() == 0) break;
        sleepSeconds(SLEEP_INTERVAL, true);
        waitSecs += SLEEP_INTERVAL;
    }

    // 若等待超时，则强行杀死各线程
    if (items_.getCount() > 0)
    {
        AutoLocker locker(mutex_);

        killedThreadCount = items_.getCount();
        for (int i = 0; i < items_.getCount(); i++)
            items_[i]->kill();

        items_.clear();
    }

    if (killedCount)
        *killedCount = killedThreadCount;
}

///////////////////////////////////////////////////////////////////////////////
// class ThreadPool

ThreadPool::ThreadPool() :
    condition_(mutex_),
    isRunning_(false)
{
    // nothing
}

ThreadPool::~ThreadPool()
{
    stop();
}

//-----------------------------------------------------------------------------

void ThreadPool::start(int threadCount)
{
	ASSERT_X(!isRunning_);
    ASSERT_X(threadCount > 0);

    AutoLocker locker(mutex_);

    isRunning_ = true;
    for (int i = 0; i < threadCount; ++i)
        threadList_.add(Thread::create(std::bind(&ThreadPool::threadProc, this, _1)));
}

//-----------------------------------------------------------------------------

void ThreadPool::stop(int maxWaitSecs)
{
    AutoLocker locker(mutex_);

    if (isRunning_)
    {
        isRunning_ = false;
        condition_.notifyAll();

        threadList_.terminateAllThreads();
        threadList_.waitForAllThreads(maxWaitSecs);
    }
}

//-----------------------------------------------------------------------------

void ThreadPool::addTask(const Task& task)
{
    AutoLocker locker(mutex_);
    tasks_.push_back(task);
    condition_.notify();
}

//-----------------------------------------------------------------------------

bool ThreadPool::takeTask(Task& task)
{
    AutoLocker locker(mutex_);

    while (tasks_.empty() && isRunning_)
        condition_.wait();

    if (!tasks_.empty())
    {
        task = tasks_.front();
        tasks_.pop_front();

        return true;
    }
    else
        return false;
}

//-----------------------------------------------------------------------------

void ThreadPool::threadProc(Thread& thread)
{
    AutoFinalizer autoFinalizer(std::bind(&ThreadList::remove, &threadList_, &thread));

    while (!thread.isTerminated())
    {
        try
        {
            Task task;
            if (takeTask(task))
                task(thread);
        }
        catch (Exception& e)
        {
			ERROR_LOG(e.makeLogStr().c_str());
        }
    }
}



CThreadMsgQueue::CThreadMsgQueue() 
{

}

CThreadMsgQueue::~CThreadMsgQueue()
{

}

void   CThreadMsgQueue::OnProcess()
{
	if (msgQueue_.empty())
	{
		return;
	}

	std::deque<MsgTask> msgQueueTmp;

	{
		AutoLocker locker(mutex_);
		msgQueueTmp.swap(msgQueue_);
	}

	for (auto& it : msgQueueTmp)
	{
		it();
	}

}

void  CThreadMsgQueue::AddTask(const MsgTask& task)
{
	AutoLocker locker(mutex_);
	msgQueue_.push_back(task);
}