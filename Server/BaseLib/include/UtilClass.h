#ifndef _UTILS_CLASS_H_
#define _UTILS_CLASS_H_

#include "Options.h"
#include "NonCopyable.h"
#include "GlobalDefs.h"
#include "ErrMsgs.h"
#include "Exceptions.h"
#include "ObjectArray.h"
#include <deque>

#ifdef _COMPILER_WIN
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
#endif

#ifdef _COMPILER_LINUX
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/timeb.h>
#include <sys/file.h>
#include <iostream>
#include <fstream>
#include <string>
#include <typeindex>
#endif

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


struct Any
{
	Any(void) : m_tpIndex(std::type_index(typeid(void))) {}
	Any(const Any& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}
	Any(Any && that) : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {}

	//创建智能指针时，对于一般的类型，通过std::decay来移除引用和cv符，从而获取原始类型
	template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> Any(U && value) : m_ptr(new Derived < typename std::decay<U>::type>(std::forward<U>(value))),
		m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))) {}

	bool IsNull() const { return !bool(m_ptr); }

	template<class U> bool Is() const
	{
		return m_tpIndex == std::type_index(typeid(U));
	}

	//将Any转换为实际的类型
	template<class U>
	U& AnyCast()
	{
		if (!Is<U>())
		{
			std::string msg = formatString("can not cast %s to %s", typeid(U).name(), m_tpIndex.name());
			THROW_EXCEPTION(msg.c_str());
		}

		auto derived = dynamic_cast<Derived<U>*> (m_ptr.get());
		return derived->m_value;
	}

	Any& operator=(const Any& a)
	{
		if (m_ptr == a.m_ptr)
			return *this;

		m_ptr = a.Clone();
		m_tpIndex = a.m_tpIndex;
		return *this;
	}

private:
	struct Base;
	typedef std::unique_ptr<Base> BasePtr;

	struct Base
	{
		virtual ~Base() {}
		virtual BasePtr Clone() const = 0;
	};

	template<typename T>
	struct Derived : Base
	{
		template<typename U>
		Derived(U && value) : m_value(std::forward<U>(value)) { }

		BasePtr Clone() const
		{
			return BasePtr(new Derived<T>(m_value));
		}

		T m_value;
	};

	BasePtr Clone() const
	{
		if (m_ptr != nullptr)
			return m_ptr->Clone();

		return nullptr;
	}

	BasePtr m_ptr;
	std::type_index m_tpIndex;
};


typedef Any Context;
const Context EMPTY_CONTEXT = Context();

///////////////////////////////////////////////////////////////////////////////
// class ObjectContext - 从此类继承给对象添加上下文

class ObjectContext
{
public:
	void setContext(const Any& value) { context_ = value; }
	const Any& getContext() const { return context_; }
	Any& getContext() { return context_; }
private:
	Any context_;
};

//////////////////////////////////////////////////////////////////////////
//栈空间使用   一般在变长协议中使用
#define  LOCAL_CACHE_DATA_SIZE  1024 * 32
class LocalCacheData
{
public :
	LocalCacheData(int size, const std::string& filename, UINT32 line);
	~LocalCacheData();

	template<typename T>
	operator T*() throw ()
	{ 
		return (T*)buf;
	}

private:
	char   *buf;
	char   localbuff[LOCAL_CACHE_DATA_SIZE];
};

#define LOCAL_CACHE_DATA(var,size) LocalCacheData var(size,__FILE__,__LINE__)



///////////////////////////////////////////////////////////////////////////////
// class SeqNumberAlloc - 整数序列号分配器类
//
// 说明:
// 1. 此类以线程安全方式生成一个不断递增的整数序列，用户可以指定序列的起始值；
// 2. 此类一般用于数据包的顺序号控制；

class SeqNumberAlloc : noncopyable
{
public:
	explicit SeqNumberAlloc(UINT64 startId = 0);

	// 返回一个新分配的ID
	UINT64 allocId();

private:
	Mutex mutex_;
	UINT64 currentId_;
};


///////////////////////////////////////////////////////////////////////////////
// class CallbackList - 回调列表

template<typename T>
class CallbackList
{
public:
	void registerCallback(const T& callback)
	{
		AutoLocker locker(mutex_);
		if (callback)
			items_.push_back(callback);
	}

	int getCount() const { return static_cast<int>(items_.size()); }
	const T& getItem(int index) const { return items_[index]; }

private:
	std::vector<T> items_;
	Mutex mutex_;
};


///////////////////////////////////////////////////////////////////////////////
// class BlockingQueue - 阻塞队列类

template<typename T>
class BlockingQueue : noncopyable
{
public:
	BlockingQueue() : condition_(mutex_) {}

	void put(const T& item)
	{
		AutoLocker locker(mutex_);
		queue_.push_back(item);
		condition_.notify();
	}

	T take()
	{
		AutoLocker locker(mutex_);
		while (queue_.empty())
			condition_.wait();
		ASSERT_X(!queue_.empty());

		T item(queue_.front());
		queue_.pop_front();
		return item;
	}

	size_t getCount() const
	{
		AutoLocker locker(mutex_);
		return queue_.size();
	}

private:
	Condition::Mutex mutex_;
	Condition condition_;
	std::deque<T> queue_;
};

///////////////////////////////////////////////////////////////////////////////
// class SignalMasker - 信号屏蔽类

#ifdef _COMPILER_LINUX
class SignalMasker : noncopyable
{
public:
	explicit SignalMasker(bool isAutoRestore = false);
	virtual ~SignalMasker();

	// 设置 Block/UnBlock 操作所需的信号集合
	void setSignals(int sigCount, ...);
	void setSignals(int sigCount, va_list argList);

	// 在进程当前阻塞信号集中添加 setSignals 设置的信号
	void block();
	// 在进程当前阻塞信号集中解除 setSignals 设置的信号
	void unBlock();

	// 将进程阻塞信号集恢复为 Block/UnBlock 之前的状态
	void restore();

private:
	int sigProcMask(int how, const sigset_t *newSet, sigset_t *oldSet);

private:
	sigset_t oldSet_;
	sigset_t newSet_;
	bool isBlock_;
	bool isAutoRestore_;
};
#endif




///////////////////////////////////////////////////////////////////////////////
// class Url - URL解析类

class Url
{
public:
	// The URL parts.
	enum URL_PART
	{
		URL_PROTOCOL = 0x0001,
		URL_HOST = 0x0002,
		URL_PORT = 0x0004,
		URL_PATH = 0x0008,
		URL_FILENAME = 0x0010,
		URL_BOOKMARK = 0x0020,
		URL_USERNAME = 0x0040,
		URL_PASSWORD = 0x0080,
		URL_PARAMS = 0x0100,
		URL_ALL = 0xFFFF,
	};

public:
	Url(const std::string& url = "");
	Url(const Url& src);
	virtual ~Url() {}

	void clear();
	Url& operator = (const Url& rhs);

	std::string getUrl() const;
	std::string getUrl(UINT parts);
	void setUrl(const std::string& value);

	const std::string& getProtocol() const { return protocol_; }
	const std::string& getHost() const { return host_; }
	const std::string& getPort() const { return port_; }
	const std::string& getPath() const { return path_; }
	const std::string& getFileName() const { return fileName_; }
	const std::string& getBookmark() const { return bookmark_; }
	const std::string& getUserName() const { return userName_; }
	const std::string& getPassword() const { return password_; }
	const std::string& getParams() const { return params_; }

	void setProtocol(const std::string& value) { protocol_ = value; }
	void setHost(const std::string& value) { host_ = value; }
	void setPort(const std::string& value) { port_ = value; }
	void setPath(const std::string& value) { path_ = value; }
	void setFileName(const std::string& value) { fileName_ = value; }
	void setBookmark(const std::string& value) { bookmark_ = value; }
	void setUserName(const std::string& value) { userName_ = value; }
	void setPassword(const std::string& value) { password_ = value; }
	void setParams(const std::string& value) { params_ = value; }

private:
	std::string protocol_;
	std::string host_;
	std::string port_;
	std::string path_;
	std::string fileName_;
	std::string bookmark_;
	std::string userName_;
	std::string password_;
	std::string params_;
};


#endif

