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
// class AutoInvokable/AutoInvoker - 自动被调对象/自动调用者
//
// 说明:
// 1. 这两个类联合使用，可以起到和 "智能指针" 类似的作用，即利用栈对象自动销毁的特性，在栈
//    对象的生命周期中自动调用 AutoInvokable::invokeInitialize() 和 invokeFinalize()。
//    此二类一般使用在重要资源的对称性操作场合(比如加锁/解锁)。
// 2. 使用者需继承 AutoInvokable 类，重写 invokeInitialize() 和 invokeFinalize()
//    函数。并在需要调用的地方定义 AutoInvoker 的栈对象。

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
// class AutoFinalizer - 基于 RAII 的自动析构器

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
// class AutoLocker - 线程自动互斥类
//
// 说明:
// 1. 此类利用C++的栈对象自动销毁的特性，在多线程环境下进行局部范围临界区互斥；
// 2. 使用方法: 在需要互斥的范围中以局部变量方式定义此类对象即可；
//
// 使用范例:
//   假设已定义: Mutex mutex_;
//   自动加锁和解锁:
//   {
//       AutoLocker locker(mutex_);
//       //...
//   }

typedef AutoInvoker AutoLocker;


///////////////////////////////////////////////////////////////////////////////
// class BaseMutex - 互斥器基类

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
// class Mutex - 线程互斥类
//
// 说明:
// 1. 此类用于多线程环境下临界区互斥，基本操作有 lock、unlock；
// 2. 线程内允许嵌套调用 lock，嵌套调用后必须调用相同次数的 unlock 才可解锁；

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
// class Semaphore - 信号量类

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
// class AtomicInteger - 提供原子操作的整数类

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
// class Condition - 条件变量类
//
// 说明:
// * 条件变量的使用总是和一个互斥锁 (Condition::Mutex) 结合在一起。
// * 由于 Mutex 类的 Windows 实现采用了非内核对象的 CRITICAL_SECTION，故不能
//   与 Condition 配合使用。为了统一，无论 Linux 平台还是 Windows 平台，都一律使用
//   Condition::Mutex 与 Condition 搭配。
// * 调用 wait() 时，会原子地 unlock mutex 并进入等待，执行完毕时会自动重新 lock mutex.
// * 应该采用循环方式调用 wait()，以防虚假唤醒 (spurious wakeup)。
// * 典型使用方式是:
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