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


struct Any
{
	Any(void) : m_tpIndex(std::type_index(typeid(void))) {}
	Any(const Any& that) : m_ptr(that.Clone()), m_tpIndex(that.m_tpIndex) {}
	Any(Any && that) : m_ptr(std::move(that.m_ptr)), m_tpIndex(that.m_tpIndex) {}

	//��������ָ��ʱ������һ������ͣ�ͨ��std::decay���Ƴ����ú�cv�����Ӷ���ȡԭʼ����
	template<typename U, class = typename std::enable_if<!std::is_same<typename std::decay<U>::type, Any>::value, U>::type> Any(U && value) : m_ptr(new Derived < typename std::decay<U>::type>(std::forward<U>(value))),
		m_tpIndex(std::type_index(typeid(typename std::decay<U>::type))) {}

	bool IsNull() const { return !bool(m_ptr); }

	template<class U> bool Is() const
	{
		return m_tpIndex == std::type_index(typeid(U));
	}

	//��Anyת��Ϊʵ�ʵ�����
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
// class ObjectContext - �Ӵ���̳и��������������

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
//ջ�ռ�ʹ��   һ���ڱ䳤Э����ʹ��
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
// class SeqNumberAlloc - �������кŷ�������
//
// ˵��:
// 1. �������̰߳�ȫ��ʽ����һ�����ϵ������������У��û�����ָ�����е���ʼֵ��
// 2. ����һ���������ݰ���˳��ſ��ƣ�

class SeqNumberAlloc : noncopyable
{
public:
	explicit SeqNumberAlloc(UINT64 startId = 0);

	// ����һ���·����ID
	UINT64 allocId();

private:
	Mutex mutex_;
	UINT64 currentId_;
};


///////////////////////////////////////////////////////////////////////////////
// class CallbackList - �ص��б�

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
// class BlockingQueue - ����������

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
// class SignalMasker - �ź�������

#ifdef _COMPILER_LINUX
class SignalMasker : noncopyable
{
public:
	explicit SignalMasker(bool isAutoRestore = false);
	virtual ~SignalMasker();

	// ���� Block/UnBlock ����������źż���
	void setSignals(int sigCount, ...);
	void setSignals(int sigCount, va_list argList);

	// �ڽ��̵�ǰ�����źż������ setSignals ���õ��ź�
	void block();
	// �ڽ��̵�ǰ�����źż��н�� setSignals ���õ��ź�
	void unBlock();

	// �����������źż��ָ�Ϊ Block/UnBlock ֮ǰ��״̬
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
// class Url - URL������

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

