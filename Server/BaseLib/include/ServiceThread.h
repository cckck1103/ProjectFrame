///////////////////////////////////////////////////////////////////////////////
// ServiceThread.h
///////////////////////////////////////////////////////////////////////////////

#ifndef _SERVICE_THREAD_H_
#define _SERVICE_THREAD_H_

#include "Options.h"
#include <deque>
#include <vector>

#ifdef _COMPILER_WIN
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <process.h>
#endif

#ifdef _COMPILER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#endif

#include <thread>

#include "GlobalDefs.h"
#include "ErrMsgs.h"
#include "NonCopyable.h"
#include "UtilClass.h"
#include "ObjectArray.h"
///////////////////////////////////////////////////////////////////////////////
/* 说明

	一、Windows平台下和Linux平台下线程的主要区别:

	1. Windows线程拥有Handle和ThreadId，而Linux线程只有ThreadId。
	2. Windows线程只有ThreadPriority，而Linux线程有ThreadPolicy和ThreadPriority。

*/
///////////////////////////////////////////////////////////////////////////////

	class Thread;

	///////////////////////////////////////////////////////////////////////////////
	// 类型定义

#ifdef _COMPILER_WIN
	// 线程优先级
	enum
	{
		THREAD_PRI_IDLE = 0,
		THREAD_PRI_LOWEST = 1,
		THREAD_PRI_NORMAL = 2,
		THREAD_PRI_HIGHER = 3,
		THREAD_PRI_HIGHEST = 4,
		THREAD_PRI_TIMECRITICAL = 5
	};
#endif

#ifdef _COMPILER_LINUX
	// 线程调度策略
	enum
	{
		THREAD_POL_DEFAULT = SCHED_OTHER,
		THREAD_POL_RR = SCHED_RR,
		THREAD_POL_FIFO = SCHED_FIFO
	};

	// 线程优先级
	enum
	{
		THREAD_PRI_DEFAULT = 0,
		THREAD_PRI_MIN = 0,
		THREAD_PRI_MAX = 99,
		THREAD_PRI_HIGH = 80
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class ThreadImpl - 平台线程实现基类

	class ThreadImpl : noncopyable
	{
	public:
		friend class Thread;

	public:
		ThreadImpl(Thread *thread);
		virtual ~ThreadImpl() {}

		virtual void run() = 0;
		virtual void terminate();
		virtual void kill() = 0;
		virtual int waitFor() = 0;

		void sleep(double seconds);
		void interruptSleep() { isSleepInterrupted_ = true; }
		bool isRunning() { return isExecuting_; }

		// 属性 (getter)
		Thread* getThread() { return (Thread*)&thread_; }
		THREAD_ID getThreadId() const { return threadId_; }
		int isTerminated() const { return terminated_; }
		int getReturnValue() const { return returnValue_; }
		bool isAutoDelete() const { return isAutoDelete_; }
		int getTermElapsedSecs() const;
		// 属性 (setter)
		void setThreadId(THREAD_ID value) { threadId_ = value; }
		void setExecuting(bool value) { isExecuting_ = value; }
		void setTerminated(bool value);
		void setReturnValue(int value) { returnValue_ = value; }
		void setAutoDelete(bool value) { isAutoDelete_ = value; }

	protected:
		void execute();
		void afterExecute();
		void beforeTerminate();
		void beforeKill();

		void checkNotRunning();

	protected:
		Thread& thread_;              // 相关联的 Thread 对象
		THREAD_ID threadId_;          // 线程ID (线程在内核中的ID)
		bool isExecuting_;            // 线程是否正在执行线程函数
		bool isRunCalled_;            // run() 函数是否已被调用过
		int termTime_;                // 调用 terminate() 时的时间戳
		bool isAutoDelete_;           // 线程退出时是否同时释放类对象
		bool terminated_;             // 是否应退出的标志
		bool isSleepInterrupted_;     // 睡眠是否被中断
		int returnValue_;             // 线程返回值 (可在 execute 函数中修改此值，函数 waitFor 返回此值)
	};

	///////////////////////////////////////////////////////////////////////////////
	// class WinThreadImpl - Windows平台线程实现类

#ifdef _COMPILER_WIN
	class WinThreadImpl : public ThreadImpl
	{
	public:
		friend UINT __stdcall threadExecProc(void *param);

	public:
		WinThreadImpl(Thread *thread);
		virtual ~WinThreadImpl();

		virtual void run();
		virtual void terminate();
		virtual void kill();
		virtual int waitFor();

		int getPriority() const { return priority_; }
		void setPriority(int value);

	private:
		void checkThreadError(bool success);

	protected:
		HANDLE handle_;               // 线程句柄
		int priority_;                // 线程优先级
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class LinuxThreadImpl - Linux平台线程实现类

#ifdef _COMPILER_LINUX
	class LinuxThreadImpl : public ThreadImpl
	{
	public:
		friend void threadFinalProc(void *param);
		friend void* threadExecProc(void *param);

	public:
		LinuxThreadImpl(Thread *thread);
		virtual ~LinuxThreadImpl();

		virtual void run();
		virtual void terminate();
		virtual void kill();
		virtual int waitFor();

		int getPolicy() const { return policy_; }
		int getPriority() const { return priority_; }
		void setPolicy(int value);
		void setPriority(int value);

	private:
		void checkThreadError(int errorCode);

	protected:
		pthread_t pthreadId_;         // POSIX线程ID
		int policy_;                  // 线程调度策略 (THREAD_POLICY_XXX)
		int priority_;                // 线程优先级 (0..99)
		Semaphore *execSem_;          // 用于启动线程函数时暂时阻塞
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class Thread - 线程类

	class Thread : noncopyable
	{
	public:
		typedef std::function<void(Thread& thread)> ThreadProc;

	public:
		Thread() : threadImpl_(this) {}
		virtual ~Thread() {}

		// 创建一个线程并马上执行
		static Thread* create(const ThreadProc& threadProc);

		// 创建并执行线程。
		// 注: 此成员方法在对象声明周期中只可调用一次。
		void run() { threadImpl_.run(); }

		// 通知线程退出 (自愿退出机制)
		// 注: 若线程由于某些阻塞式操作迟迟不退出，可调用 kill() 强行退出。
		void terminate() { threadImpl_.terminate(); }

		// 强行杀死线程 (强行退出机制)
		void kill() { threadImpl_.kill(); }

		// 等待线程退出
		int waitFor() { return threadImpl_.waitFor(); }

		// 进入睡眠状态 (睡眠过程中会检测 terminated_ 的状态)
		// 注: 此函数必须由线程自己调用方可生效。
		void sleep(double seconds) { threadImpl_.sleep(seconds); }
		// 打断睡眠
		void interruptSleep() { threadImpl_.interruptSleep(); }

		// 判断线程是否正在运行
		bool isRunning() { return threadImpl_.isRunning(); }

		// 属性 (getter)
		THREAD_ID getThreadId() const { return threadImpl_.getThreadId(); }
		int isTerminated() const { return threadImpl_.isTerminated(); }
		int getReturnValue() const { return threadImpl_.getReturnValue(); }
		bool isAutoDelete() const { return threadImpl_.isAutoDelete(); }
		int getTermElapsedSecs() const { return threadImpl_.getTermElapsedSecs(); }
#ifdef _COMPILER_WIN
		int getPriority() const { return threadImpl_.getPriority(); }
#endif
#ifdef _COMPILER_LINUX
		int getPolicy() const { return threadImpl_.getPolicy(); }
		int getPriority() const { return threadImpl_.getPriority(); }
#endif
		// 属性 (setter)
		void setTerminated(bool value) { threadImpl_.setTerminated(value); }
		void setReturnValue(int value) { threadImpl_.setReturnValue(value); }
		void setAutoDelete(bool value) { threadImpl_.setAutoDelete(value); }
#ifdef _COMPILER_WIN
		void setPriority(int value) { threadImpl_.setPriority(value); }
#endif
#ifdef _COMPILER_LINUX
		void setPolicy(int value) { threadImpl_.setPolicy(value); }
		void setPriority(int value) { threadImpl_.setPriority(value); }
#endif

	protected:
		// 线程的执行函数，子类必须重写。
		virtual void execute() { if (threadProc_) threadProc_(*this); }

		// 在执行完 execute() 后，永远会执行此函数。
		virtual void afterExecute() {}

		// 执行 terminate() 前的附加操作。
		// 注: 由于 terminate() 属于自愿退出机制，为了能让线程能尽快退出，除了
		// terminated_ 标志被设为 true 之外，有时还应当补充一些附加的操作以
		// 便能让线程尽快从阻塞操作中解脱出来。
		virtual void beforeTerminate() {}

		// 执行 kill() 前的附加操作。
		// 注: 线程被杀死后，用户所管理的某些重要资源可能未能得到释放，比如锁资源
		// (还未来得及解锁便被杀了)，所以重要资源的释放工作必须在 beforeKill 中进行。
		virtual void beforeKill() {}

	private:
#ifdef _COMPILER_WIN
		WinThreadImpl threadImpl_;
#endif
#ifdef _COMPILER_LINUX
		LinuxThreadImpl threadImpl_;
#endif

		ThreadProc threadProc_;

		friend class ThreadImpl;
	};

	///////////////////////////////////////////////////////////////////////////////
	// class ThreadList - 线程列表

	class ThreadList : noncopyable
	{
	public:
		ThreadList();
		virtual ~ThreadList();

		void add(Thread *thread);
		void remove(Thread *thread);
		bool exists(Thread *thread);
		void clear();

		void terminateAllThreads();
		void waitForAllThreads(int maxWaitSecs = TIMEOUT_INFINITE, int *killedCount = NULL);

		int getCount() const { return items_.getCount(); }
		Thread* getItem(int index) const { return items_[index]; }
		Thread* operator[] (int index) const { return getItem(index); }

		Mutex& getMutex() const { return mutex_; }

	protected:
		ObjectList<Thread> items_;
		mutable Mutex mutex_;
	};

	///////////////////////////////////////////////////////////////////////////////
	// class ThreadPool - 线程池

	class ThreadPool : noncopyable
	{
	public:
		typedef std::function<void(Thread& thread)> Task;

	public:
		ThreadPool();
		virtual ~ThreadPool();

		void start(int threadCount);
		void stop(int maxWaitSecs = TIMEOUT_INFINITE);

		void addTask(const Task& task);
		bool isRunning() const { return isRunning_; }

	private:
		bool takeTask(Task& task);
		void threadProc(Thread& thread);

	private:
		Condition::Mutex mutex_;
		Condition condition_;
		std::deque<Task> tasks_;
		ThreadList threadList_;
		bool isRunning_;
	};

	///////////////////////////////////////////////////////////////////////////////
	//CThreadMsgQueue

	class CThreadMsgQueue
	{
		CThreadMsgQueue();
		virtual ~CThreadMsgQueue();
		typedef std::function<void()> MsgTask;
	public:
		void   OnProcess();

		void   AddTask(const MsgTask& task);
	private:
		std::deque<MsgTask> msgQueue_;
		Mutex mutex_;
	};


	//////////////////////////////////////////////////////////////////////////
#endif // 
