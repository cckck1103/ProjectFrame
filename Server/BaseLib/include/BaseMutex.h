#ifndef _BASE_MUTEX_H_
#define _BASE_MUTEX_H_

#include "Options.h"
#include "NonCopyable.h"
#include "GlobalDefs.h"
#include "ErrMsgs.h"
#include "Exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <typeindex>

class AtomicInt;
class AtomicInt64;
class Mutex;
class Semaphore;
class Condition;
class AutoFinalizer;

///////////////////////////////////////////////////////////////////////////////
// class AutoInvokable/AutoInvoker - �Զ���������/�Զ�������
//
// ˵��:
// 1. ������������ʹ�ã������𵽺� "����ָ��" ���Ƶ����ã�������ջ�����Զ����ٵ����ԣ���ջ
//    ����������������Զ����� AutoInvokable::invokeInitialize() �� invokeFinalize()��
//    �˶���һ��ʹ������Ҫ��Դ�ĶԳ��Բ�������(�������/����)��
// 2. ʹ������̳� AutoInvokable �࣬��д invokeInitialize() �� invokeFinalize()
//    ������������Ҫ���õĵط����� AutoInvoker ��ջ����

class AutoInvokable
{
public:
	friend class AutoInvoker;
	virtual ~AutoInvokable() {}
protected:
	virtual void invokeInitialize() {}
	virtual void invokeFinalize() {}
};

///////////////////////////////////////////////////////////////////////////////
// class AutoFinalizer - ���� RAII ���Զ�������

class AutoFinalizer : noncopyable
{
public:
	AutoFinalizer(const Functor& f) : f_(f) {}
	~AutoFinalizer() { f_(); }
private:
	Functor f_;
};


class AutoInvoker : noncopyable
{
public:
	explicit AutoInvoker(AutoInvokable& object)
	{
		object_ = &object; object_->invokeInitialize();
	}

	explicit AutoInvoker(AutoInvokable *object)
	{
		object_ = object; if (object_) object_->invokeInitialize();
	}

	virtual ~AutoInvoker()
	{
		if (object_) object_->invokeFinalize();
	}

private:
	AutoInvokable *object_;
};

///////////////////////////////////////////////////////////////////////////////
// class AutoLocker - �߳��Զ�������
//
// ˵��:
// 1. ��������C++��ջ�����Զ����ٵ����ԣ��ڶ��̻߳����½��оֲ���Χ�ٽ������⣻
// 2. ʹ�÷���: ����Ҫ����ķ�Χ���Ծֲ�������ʽ���������󼴿ɣ�
//
// ʹ�÷���:
//   �����Ѷ���: Mutex mutex_;
//   �Զ������ͽ���:
//   {
//       AutoLocker locker(mutex_);
//       //...
//   }

typedef AutoInvoker AutoLocker;


///////////////////////////////////////////////////////////////////////////////
// class BaseMutex - ����������

class BaseMutex :
	public AutoInvokable,
	noncopyable
{
public:
	virtual void lock() = 0;
	virtual void unlock() = 0;
protected:
	virtual void invokeInitialize() { lock(); }
	virtual void invokeFinalize() { unlock(); }
};

///////////////////////////////////////////////////////////////////////////////
// class Mutex - �̻߳�����
//
// ˵��:
// 1. �������ڶ��̻߳������ٽ������⣬���������� lock��unlock��
// 2. �߳�������Ƕ�׵��� lock��Ƕ�׵��ú���������ͬ������ unlock �ſɽ�����

class Mutex : public BaseMutex
{
public:
	Mutex();
	virtual ~Mutex();

	virtual void lock();
	virtual void unlock();

private:
#ifdef _COMPILER_WIN
	CRITICAL_SECTION critiSect_;
#endif
#ifdef _COMPILER_LINUX
	pthread_mutex_t mutex_;
	friend class Condition;
#endif
};

///////////////////////////////////////////////////////////////////////////////
// class Semaphore - �ź�����

class Semaphore : noncopyable
{
public:
	explicit Semaphore(UINT initValue = 0);
	virtual ~Semaphore();

	void increase();
	void wait();
	void reset();

private:
	void doCreateSem();
	void doDestroySem();

private:
#ifdef _COMPILER_WIN
	HANDLE sem_;
	friend class Condition;
#endif
#ifdef _COMPILER_LINUX
	sem_t sem_;
#endif

	UINT initValue_;
};
///////////////////////////////////////////////////////////////////////////////
// class AtomicInteger - �ṩԭ�Ӳ�����������

#ifdef _COMPILER_WIN

class AtomicInt : noncopyable
{
public:
	AtomicInt() : value_(0) {}

	LONG get() { return InterlockedCompareExchange(&value_, 0, 0); }
	LONG set(LONG newValue) { return InterlockedExchange(&value_, newValue); }
	LONG getAndAdd(LONG x) { return InterlockedExchangeAdd(&value_, x); }
	LONG addAndGet(LONG x) { return getAndAdd(x) + x; }
	LONG increment() { return InterlockedIncrement(&value_); }
	LONG decrement() { return InterlockedDecrement(&value_); }
	LONG getAndSet(LONG newValue) { return set(newValue); }

private:
	volatile LONG value_;
};

class AtomicInt64 : noncopyable
{
public:
	AtomicInt64() : value_(0) {}

	INT64 get()
	{
		AutoLocker locker(mutex_);
		return value_;
	}
	INT64 set(INT64 newValue)
	{
		AutoLocker locker(mutex_);
		INT64 temp = value_;
		value_ = newValue;
		return temp;
	}
	INT64 getAndAdd(INT64 x)
	{
		AutoLocker locker(mutex_);
		INT64 temp = value_;
		value_ += x;
		return temp;
	}
	INT64 addAndGet(INT64 x)
	{
		AutoLocker locker(mutex_);
		value_ += x;
		return value_;
	}
	INT64 increment()
	{
		AutoLocker locker(mutex_);
		++value_;
		return value_;
	}
	INT64 decrement()
	{
		AutoLocker locker(mutex_);
		--value_;
		return value_;
	}
	INT64 getAndSet(INT64 newValue)
	{
		return set(newValue);
	}

private:
	volatile INT64 value_;
	Mutex mutex_;
};

#endif

#ifdef _COMPILER_LINUX

template<typename T>
class AtomicInteger : noncopyable
{
public:
	AtomicInteger() : value_(0) {}

	T get() { return __sync_val_compare_and_swap(&value_, 0, 0); }
	T set(T newValue) { return __sync_lock_test_and_set(&value_, newValue); }
	T getAndAdd(T x) { return __sync_fetch_and_add(&value_, x); }
	T addAndGet(T x) { return __sync_add_and_fetch(&value_, x); }
	T increment() { return addAndGet(1); }
	T decrement() { return addAndGet(-1); }
	T getAndSet(T newValue) { return set(newValue); }

private:
	volatile T value_;
};

class AtomicInt : public AtomicInteger<long> {};
class AtomicInt64 : public AtomicInteger<INT64> {};

#endif


///////////////////////////////////////////////////////////////////////////////
// class Condition - ����������
//
// ˵��:
// * ����������ʹ�����Ǻ�һ�������� (Condition::Mutex) �����һ��
// * ���� Mutex ��� Windows ʵ�ֲ����˷��ں˶���� CRITICAL_SECTION���ʲ���
//   �� Condition ���ʹ�á�Ϊ��ͳһ������ Linux ƽ̨���� Windows ƽ̨����һ��ʹ��
//   Condition::Mutex �� Condition ���䡣
// * ���� wait() ʱ����ԭ�ӵ� unlock mutex ������ȴ���ִ�����ʱ���Զ����� lock mutex.
// * Ӧ�ò���ѭ����ʽ���� wait()���Է���ٻ��� (spurious wakeup)��
// * ����ʹ�÷�ʽ��:
//
//   class Example
//   {
//   public:
//       Example() : condition_(mutex_) {}
//
//       void addToQueue()
//       {
//           {
//               AutoLocker locker(mutex_);
//               queue_.push_back(...);
//           }
//           condition_.notify();
//       }
//
//       void extractFromQueue()
//       {
//           AutoLocker locker(mutex_);
//           while (queue_.empty())
//               condition_.wait();
//           assert(!queue_.empty());
//
//           int top = queue_.front();
//           queue_.pop_front();
//           ...
//       }
//
//   private:
//       Condition::Mutex mutex_;
//       Condition condition_;
//   };

class Condition : noncopyable
{
public:
	class Mutex : public BaseMutex
	{
	public:
		Mutex();
		virtual ~Mutex();

		virtual void lock();
		virtual void unlock();

	private:
#ifdef _COMPILER_WIN
		HANDLE mutex_;
#endif
#ifdef _COMPILER_LINUX
		Mutex mutex_;
#endif
		friend class Condition;
	};

public:
	Condition(Condition::Mutex& mutex);
	~Condition();

	void wait();
	void notify();
	void notifyAll();

	Condition::Mutex& getMutex() { return mutex_; }

private:
	Condition::Mutex& mutex_;

#ifdef _COMPILER_WIN
	Semaphore sem_;
	AtomicInt waiters_;
#endif

#ifdef _COMPILER_LINUX
	pthread_cond_t cond_;
#endif
};







#endif