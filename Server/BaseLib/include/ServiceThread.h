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
/* ˵��

	һ��Windowsƽ̨�º�Linuxƽ̨���̵߳���Ҫ����:

	1. Windows�߳�ӵ��Handle��ThreadId����Linux�߳�ֻ��ThreadId��
	2. Windows�߳�ֻ��ThreadPriority����Linux�߳���ThreadPolicy��ThreadPriority��

*/
///////////////////////////////////////////////////////////////////////////////

	class Thread;

	///////////////////////////////////////////////////////////////////////////////
	// ���Ͷ���

#ifdef _COMPILER_WIN
	// �߳����ȼ�
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
	// �̵߳��Ȳ���
	enum
	{
		THREAD_POL_DEFAULT = SCHED_OTHER,
		THREAD_POL_RR = SCHED_RR,
		THREAD_POL_FIFO = SCHED_FIFO
	};

	// �߳����ȼ�
	enum
	{
		THREAD_PRI_DEFAULT = 0,
		THREAD_PRI_MIN = 0,
		THREAD_PRI_MAX = 99,
		THREAD_PRI_HIGH = 80
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class ThreadImpl - ƽ̨�߳�ʵ�ֻ���

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

		// ���� (getter)
		Thread* getThread() { return (Thread*)&thread_; }
		THREAD_ID getThreadId() const { return threadId_; }
		int isTerminated() const { return terminated_; }
		int getReturnValue() const { return returnValue_; }
		bool isAutoDelete() const { return isAutoDelete_; }
		int getTermElapsedSecs() const;
		// ���� (setter)
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
		Thread& thread_;              // ������� Thread ����
		THREAD_ID threadId_;          // �߳�ID (�߳����ں��е�ID)
		bool isExecuting_;            // �߳��Ƿ�����ִ���̺߳���
		bool isRunCalled_;            // run() �����Ƿ��ѱ����ù�
		int termTime_;                // ���� terminate() ʱ��ʱ���
		bool isAutoDelete_;           // �߳��˳�ʱ�Ƿ�ͬʱ�ͷ������
		bool terminated_;             // �Ƿ�Ӧ�˳��ı�־
		bool isSleepInterrupted_;     // ˯���Ƿ��ж�
		int returnValue_;             // �̷߳���ֵ (���� execute �������޸Ĵ�ֵ������ waitFor ���ش�ֵ)
	};

	///////////////////////////////////////////////////////////////////////////////
	// class WinThreadImpl - Windowsƽ̨�߳�ʵ����

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
		HANDLE handle_;               // �߳̾��
		int priority_;                // �߳����ȼ�
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class LinuxThreadImpl - Linuxƽ̨�߳�ʵ����

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
		pthread_t pthreadId_;         // POSIX�߳�ID
		int policy_;                  // �̵߳��Ȳ��� (THREAD_POLICY_XXX)
		int priority_;                // �߳����ȼ� (0..99)
		Semaphore *execSem_;          // ���������̺߳���ʱ��ʱ����
	};
#endif

	///////////////////////////////////////////////////////////////////////////////
	// class Thread - �߳���

	class Thread : noncopyable
	{
	public:
		typedef std::function<void(Thread& thread)> ThreadProc;

	public:
		Thread() : threadImpl_(this) {}
		virtual ~Thread() {}

		// ����һ���̲߳�����ִ��
		static Thread* create(const ThreadProc& threadProc);

		// ������ִ���̡߳�
		// ע: �˳�Ա�����ڶ�������������ֻ�ɵ���һ�Ρ�
		void run() { threadImpl_.run(); }

		// ֪ͨ�߳��˳� (��Ը�˳�����)
		// ע: ���߳�����ĳЩ����ʽ�����ٳٲ��˳����ɵ��� kill() ǿ���˳���
		void terminate() { threadImpl_.terminate(); }

		// ǿ��ɱ���߳� (ǿ���˳�����)
		void kill() { threadImpl_.kill(); }

		// �ȴ��߳��˳�
		int waitFor() { return threadImpl_.waitFor(); }

		// ����˯��״̬ (˯�߹����л��� terminated_ ��״̬)
		// ע: �˺����������߳��Լ����÷�����Ч��
		void sleep(double seconds) { threadImpl_.sleep(seconds); }
		// ���˯��
		void interruptSleep() { threadImpl_.interruptSleep(); }

		// �ж��߳��Ƿ���������
		bool isRunning() { return threadImpl_.isRunning(); }

		// ���� (getter)
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
		// ���� (setter)
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
		// �̵߳�ִ�к��������������д��
		virtual void execute() { if (threadProc_) threadProc_(*this); }

		// ��ִ���� execute() ����Զ��ִ�д˺�����
		virtual void afterExecute() {}

		// ִ�� terminate() ǰ�ĸ��Ӳ�����
		// ע: ���� terminate() ������Ը�˳����ƣ�Ϊ�������߳��ܾ����˳�������
		// terminated_ ��־����Ϊ true ֮�⣬��ʱ��Ӧ������һЩ���ӵĲ�����
		// �������߳̾�������������н��ѳ�����
		virtual void beforeTerminate() {}

		// ִ�� kill() ǰ�ĸ��Ӳ�����
		// ע: �̱߳�ɱ�����û��������ĳЩ��Ҫ��Դ����δ�ܵõ��ͷţ���������Դ
		// (��δ���ü������㱻ɱ��)��������Ҫ��Դ���ͷŹ��������� beforeKill �н��С�
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
	// class ThreadList - �߳��б�

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
	// class ThreadPool - �̳߳�

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
#endif // 
