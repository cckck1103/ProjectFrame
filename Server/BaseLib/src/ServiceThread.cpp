///////////////////////////////////////////////////////////////////////////////
// �ļ�����: ServiceThread.cpp
// ��������: �߳���
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
// ����: ֪ͨ�߳��˳�
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
// ����: ȡ�ôӵ��� terminate ����ǰ����������ʱ��(��)
//-----------------------------------------------------------------------------
int ThreadImpl::getTermElapsedSecs() const
{
    int result = 0;

    // ����Ѿ�֪ͨ�˳������̻߳�����
    if (terminated_ && threadId_ != 0)
    {
        result = (int)(time(NULL) - termTime_);
    }

    return result;
}

//-----------------------------------------------------------------------------
// ����: �����Ƿ� terminate
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
// ����: ����˯��״̬ (˯�߹����л��� terminated_ ��״̬)
// ����:
//   seconds - ˯�ߵ���������ΪС�����ɾ�ȷ������
// ע��:
//   ���ڽ�˯��ʱ��ֳ������ɷݣ�ÿ��˯��ʱ���С����ۼ���������������
//-----------------------------------------------------------------------------
void ThreadImpl::sleep(double seconds)
{
    const double SLEEP_INTERVAL = 0.5;      // ÿ��˯�ߵ�ʱ��(��)
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
// ����: ����߳������У����׳��쳣
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
// ����: �߳�ִ�к���
// ����:
//   param - �̲߳������˴�ָ�� WinThreadImpl ����
//-----------------------------------------------------------------------------
UINT __stdcall threadExecProc(void *param)
{
    WinThreadImpl *threadImpl = (WinThreadImpl*)param;
    int returnValue = 0;

    {
        threadImpl->setExecuting(true);

        // ���� finalizer �����Զ����ƺ���
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

            // �����̷߳���ֵ
            returnValue = threadImpl->returnValue_;
        }
    }

    // ע��: ����� _endthreadex �� ExitThread ������
    _endthreadex(returnValue);
    //ExitThread(returnValue);

    return returnValue;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
//-----------------------------------------------------------------------------
WinThreadImpl::WinThreadImpl(Thread *thread) :
    ThreadImpl(thread),
    handle_(0),
    priority_(THREAD_PRI_NORMAL)
{
    // nothing
}

//-----------------------------------------------------------------------------
// ����: ��������
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
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void WinThreadImpl::run()
{
    checkNotRunning();
    isRunCalled_ = true;

    // ע��: ����� CRT::_beginthreadex �� API::CreateThread ������ǰ�߼�����CRT��
    handle_ = (HANDLE)_beginthreadex(NULL, 0, threadExecProc, (LPVOID)this,
        CREATE_SUSPENDED, (UINT*)&threadId_);
    /*
    handle_ = CreateThread(NULL, 0, threadExecProc, (LPVOID)this, CREATE_SUSPENDED, (LPDWORD)&threadId_);
    */

    checkThreadError(handle_ != 0);

    // �����߳����ȼ�
    if (priority_ != THREAD_PRI_NORMAL)
        setPriority(priority_);

    ::ResumeThread(handle_);
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void WinThreadImpl::terminate()
{
    ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺����󣬶��߳�������һ�в����Բ�����(terminate(); waitFor(); delete thread; ��)��
//   2. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� beforeKill �н��С�
//   3. Windows ��ǿɱ�̣߳��߳�ִ�й����е�ջ���󲻻�������
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
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
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
// ����: �����̵߳����ȼ�
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
// ����: �̴߳�����
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
// ����: �߳�������
// ����:
//   param - �̲߳������˴�ָ�� LinuxThreadImpl ����
//-----------------------------------------------------------------------------
void threadFinalProc(void *param)
{
    LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;

    threadImpl->setExecuting(false);
    if (threadImpl->isAutoDelete())
        delete threadImpl->getThread();
}

//-----------------------------------------------------------------------------
// ����: �߳�ִ�к���
// ����:
//   param - �̲߳������˴�ָ�� LinuxThreadImpl ����
//-----------------------------------------------------------------------------
void* threadExecProc(void *param)
{
    LinuxThreadImpl *threadImpl = (LinuxThreadImpl*)param;
    int returnValue = 0;

    {
        // �ȴ��̶߳���׼������
        threadImpl->execSem_->wait();
        delete threadImpl->execSem_;
        threadImpl->execSem_ = NULL;

        // ��ȡ�߳��ں�ID
        threadImpl->threadId_ = getCurThreadId();

        threadImpl->setExecuting(true);

        // �̶߳� cancel �źŵ���Ӧ��ʽ������: (1)����Ӧ (2)�Ƴٵ�ȡ��������Ӧ (3)����������Ӧ��
        // �˴������߳�Ϊ��(3)�ַ�ʽ���������ϱ� cancel �ź���ֹ��
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

        // ע��������
        pthread_cleanup_push(threadFinalProc, threadImpl);

        if (!threadImpl->terminated_)
        {
            // �߳���ִ�й��������յ� cancel �źţ����׳�һ���쳣(������ʲô���͵��쳣��)��
            // ���쳣ǧ�򲻿�ȥ������( try{}catch(...){} )��ϵͳ���������쳣�Ƿ񱻳���������
            // ���ǣ���ᷢ�� SIGABRT �źţ������ն���� "FATAL: exception not rethrown"��
            // ���Դ˴��Ĳ�����ֻ���� Exception �쳣(�����쳣�Դ� Exception �̳�)��
            // �� thread->execute() ��ִ�й����У��û�Ӧ��ע����������:
            // 1. ����մ˴�������ȥ�����쳣�����в��������������͵��쳣( ��catch(...) );
            // 2. �����׳� Exception ��������֮����쳣�������׳�һ������( �� throw 5; )��
            //    ϵͳ����Ϊû�д��쳣�Ĵ����������� abort��(������ˣ�threadFinalProc
            //    �Ի��� pthread_cleanup_push ����ŵ��������ִ�е���)
            {
                AutoFinalizer finalizer(std::bind(&LinuxThreadImpl::afterExecute, threadImpl));
                try { threadImpl->execute(); } catch (Exception& e) {}
            }

            // �����̷߳���ֵ
            returnValue = threadImpl->returnValue_;
        }

        pthread_cleanup_pop(1);

        // ���� cancel �źţ����˵�ǰ scope �󽫲��ɱ�ǿ����ֹ
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    }

    pthread_exit((void*)returnValue);
    return NULL;
}

//-----------------------------------------------------------------------------
// ����: ���캯��
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
// ����: ��������
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
// ����: �����̲߳�ִ��
// ע��: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
//-----------------------------------------------------------------------------
void LinuxThreadImpl::run()
{
    checkNotRunning();
    isRunCalled_ = true;

    delete execSem_;
    execSem_ = new Semaphore(0);

    // �����߳�
    checkThreadError(pthread_create((pthread_t*)&pthreadId_, NULL, threadExecProc, (void*)this));

    // �����̵߳��Ȳ���
    if (policy_ != THREAD_POL_DEFAULT)
        setPolicy(policy_);
    // �����߳����ȼ�
    if (priority_ != THREAD_PRI_DEFAULT)
        setPriority(priority_);

    // �̶߳�����׼���������̺߳������Կ�ʼ����
    execSem_->increase();
}

//-----------------------------------------------------------------------------
// ����: ֪ͨ�߳��˳�
//-----------------------------------------------------------------------------
void LinuxThreadImpl::terminate()
{
    ThreadImpl::terminate();
}

//-----------------------------------------------------------------------------
// ����: ǿ��ɱ���߳�
// ע��:
//   1. ���ô˺������̶߳��󼴱����٣����߳�������һ�в����Բ�����(terminate();
//      waitFor(); delete thread; ��)��
//   2. ��ɱ���߳�ǰ��isAutoDelete_ ���Զ���Ϊ true���Ա�������Զ��ͷš�
//   3. �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ (��δ���ü�����
//      �㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� beforeKill �н��С�
//   4. pthread û�й涨���߳��յ� cancel �źź��Ƿ���� C++ stack unwinding��Ҳ����˵ջ����
//      ������������һ����ִ�С�ʵ��������� RedHat AS (glibc-2.3.4) �»���� stack unwinding��
//      ���� Debian 4.0 (glibc-2.3.6) ���򲻻ᡣ
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
// ����: �ȴ��߳��˳�
// ����: �̷߳���ֵ
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
// ����: �����̵߳ĵ��Ȳ���
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
// ����: �����̵߳����ȼ�
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
// ����: �̴߳�����
//-----------------------------------------------------------------------------
void LinuxThreadImpl::checkThreadError(int errorCode)
{
    if (errorCode != 0)
    {
        string errMsg = sysErrorMessage(errorCode);
		ERROR_LOG(errMsg.c_str());
        ThrowThreadException(errMsg.c_str());
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
// class Thread

//-----------------------------------------------------------------------------
// ����: ����һ���̲߳�����ִ��
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
// ����: ֪ͨ�����߳��˳�
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
// ����: �ȴ������߳��˳�
// ����:
//   maxWaitSecs - ��ȴ�ʱ��(��) (Ϊ -1 ��ʾ���޵ȴ�)
//   killedCount - ��������ǿɱ�˶��ٸ��߳�
// ע��:
//   �˺���Ҫ���б��и��߳����˳�ʱ�Զ������Լ������б����Ƴ���
//-----------------------------------------------------------------------------
void ThreadList::waitForAllThreads(int maxWaitSecs, int *killedCount)
{
    const double SLEEP_INTERVAL = 0.1;  // (��)
    double waitSecs = 0;
    int killedThreadCount = 0;

    // ֪ͨ�����߳��˳�
    terminateAllThreads();

    // �ȴ��߳��˳�
    while (maxWaitSecs < 0 || waitSecs < maxWaitSecs)
    {
        if (items_.getCount() == 0) break;
        sleepSeconds(SLEEP_INTERVAL, true);
        waitSecs += SLEEP_INTERVAL;
    }

    // ���ȴ���ʱ����ǿ��ɱ�����߳�
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
