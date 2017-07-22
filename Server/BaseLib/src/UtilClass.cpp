#include "UtilClass.h"
#include "LogManager.h"
///////////////////////////////////////////////////////////////////////////////
// class Mutex

Mutex::Mutex()
{
#ifdef _COMPILER_WIN
	InitializeCriticalSection(&critiSect_);
#endif
#ifdef _COMPILER_LINUX
	pthread_mutexattr_t attr;

	// 锁属性说明:
	// PTHREAD_MUTEX_TIMED_NP:
	//   普通锁。同一线程内必须成对调用 Lock 和 Unlock。不可连续调用多次 Lock，否则会死锁。
	// PTHREAD_MUTEX_RECURSIVE_NP:
	//   嵌套锁。线程内可以嵌套调用 Lock，第一次生效，之后必须调用相同次数的 Unlock 方可解锁。
	// PTHREAD_MUTEX_ERRORCHECK_NP:
	//   检错锁。如果同一线程嵌套调用 Lock 则产生错误。
	// PTHREAD_MUTEX_ADAPTIVE_NP:
	//   适应锁。
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
	pthread_mutex_init(&mutex_, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
}

Mutex::~Mutex()
{
#ifdef _COMPILER_WIN
	DeleteCriticalSection(&critiSect_);
#endif
#ifdef _COMPILER_LINUX
	// 如果在未解锁的情况下 destroy，此函数会返回错误 EBUSY。
	// 在 linux 下，即使此函数返回错误，也不会有资源泄漏。
	pthread_mutex_destroy(&mutex_);
#endif
}


//-----------------------------------------------------------------------------
// 描述: 加锁
//-----------------------------------------------------------------------------
void Mutex::lock()
{
#ifdef _COMPILER_WIN
	EnterCriticalSection(&critiSect_);
#endif
#ifdef _COMPILER_LINUX
	pthread_mutex_lock(&mutex_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 解锁
//-----------------------------------------------------------------------------
void Mutex::unlock()
{
#ifdef _COMPILER_WIN
	LeaveCriticalSection(&critiSect_);
#endif
#ifdef _COMPILER_LINUX
	pthread_mutex_unlock(&mutex_);
#endif
}



///////////////////////////////////////////////////////////////////////////////
// class Semaphore

Semaphore::Semaphore(UINT initValue)
{
	initValue_ = initValue;
	doCreateSem();
}

//-----------------------------------------------------------------------------

Semaphore::~Semaphore()
{
	doDestroySem();
}

//-----------------------------------------------------------------------------
// 描述: 将旗标值原子地加1，表示增加一个可访问的资源
//-----------------------------------------------------------------------------
void Semaphore::increase()
{
#ifdef _COMPILER_WIN
	ReleaseSemaphore(sem_, 1, NULL);
#endif
#ifdef _COMPILER_LINUX
	sem_post(&sem_);
#endif
}

//-----------------------------------------------------------------------------
// 描述: 等待旗标(直到旗标值大于0)，然后将旗标值原子地减1
//-----------------------------------------------------------------------------
void Semaphore::wait()
{
#ifdef _COMPILER_WIN
	WaitForSingleObject(sem_, INFINITE);
#endif
#ifdef _COMPILER_LINUX
	int ret;
	do
	{
		ret = sem_wait(&sem_);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));
#endif
}

//-----------------------------------------------------------------------------
// 描述: 将旗标重置，其计数值设为初始状态
//-----------------------------------------------------------------------------
void Semaphore::reset()
{
	doDestroySem();
	doCreateSem();
}

//-----------------------------------------------------------------------------

void Semaphore::doCreateSem()
{
#ifdef _COMPILER_WIN
	sem_ = CreateSemaphore(NULL, initValue_, 0x7FFFFFFF, NULL);
	if (!sem_)
		ThrowException(SEM_SEM_INIT_ERROR);
#endif
#ifdef _COMPILER_LINUX
	if (sem_init(&sem_, 0, initValue_) != 0)
		ThrowException(SEM_SEM_INIT_ERROR);
#endif
}


//-----------------------------------------------------------------------------

void Semaphore::doDestroySem()
{
#ifdef _COMPILER_WIN
	CloseHandle(sem_);
#endif
#ifdef _COMPILER_LINUX
	sem_destroy(&sem_);
#endif
}


///////////////////////////////////////////////////////////////////////////////
// class Condition::Mutex

Condition::Mutex::Mutex()
{
#ifdef _COMPILER_WIN
	mutex_ = ::CreateMutexA(NULL, FALSE, NULL);
	if (mutex_ == NULL)
		ThrowException(SEM_FAIL_TO_CREATE_MUTEX);
#endif
}

Condition::Mutex::~Mutex()
{
#ifdef _COMPILER_WIN
	::CloseHandle(mutex_);
#endif
}

void Condition::Mutex::lock()
{
#ifdef _COMPILER_WIN
	::WaitForSingleObject(mutex_, INFINITE);
#endif
#ifdef _COMPILER_LINUX
	mutex_.lock();
#endif
}


void Condition::Mutex::unlock()
{
#ifdef _COMPILER_WIN
	::ReleaseMutex(mutex_);
#endif
#ifdef _COMPILER_LINUX
	mutex_.unlock();
#endif
}



///////////////////////////////////////////////////////////////////////////////
// class Condition

#ifdef _COMPILER_WIN

Condition::Condition(Condition::Mutex& mutex) :
	mutex_(mutex)
{
	// nothing
}

Condition::~Condition()
{
	// nothing
}

void Condition::wait()
{
	waiters_.increment();
	::SignalObjectAndWait(mutex_.mutex_, sem_.sem_, INFINITE, FALSE);
	mutex_.lock();
	waiters_.decrement();
}

void Condition::notify()
{
	if (waiters_.get() > 0)
		sem_.increase();
}

void Condition::notifyAll()
{
	int count = (int)waiters_.get();
	for (int i = 0; i < count; ++i)
		sem_.increase();
}

#endif

//-----------------------------------------------------------------------------

#ifdef _COMPILER_LINUX

Condition::Condition(Condition::Mutex& mutex) :
	mutex_(mutex)
{
	if (pthread_cond_init(&cond_, NULL) != 0)
		ThrowException(SEM_COND_INIT_ERROR);
}

Condition::~Condition()
{
	pthread_cond_destroy(&cond_);
}

void Condition::wait()
{
	pthread_cond_wait(&cond_, &mutex_.mutex_.mutex_);
}

void Condition::notify()
{
	pthread_cond_signal(&cond_);
}

void Condition::notifyAll()
{
	pthread_cond_broadcast(&cond_);
}

#endif
//-----------------------------------------------------------------------------
LocalCacheData::LocalCacheData(int size, const std::string& filename, UINT32 line) :
	buf(localbuff)
{
	memset(localbuff, 0, sizeof(localbuff));

	if (size > LOCAL_CACHE_DATA_SIZE)
	{
		buf = new char[size];

		if (NULL == buf)
		{
			WARN_LOG("Local cache new data big size err %s:%d !!!", filename.c_str(), line);
			ThrowMemoryException();
		}
	}
}

LocalCacheData::~LocalCacheData()
{
	if (buf != localbuff)
	{
		delete[] buf;
	}
}


///////////////////////////////////////////////////////////////////////////////
// class SeqNumberAlloc

//-----------------------------------------------------------------------------
// 描述: 构造函数
// 参数:
//   startId - 起始序列号
//-----------------------------------------------------------------------------
SeqNumberAlloc::SeqNumberAlloc(UINT64 startId)
{
	currentId_ = startId;
}

//-----------------------------------------------------------------------------
// 描述: 返回一个新分配的ID
//-----------------------------------------------------------------------------
UINT64 SeqNumberAlloc::allocId()
{
	AutoLocker locker(mutex_);
	return currentId_++;
}



///////////////////////////////////////////////////////////////////////////////
// class SignalMasker

#ifdef _COMPILER_LINUX

SignalMasker::SignalMasker(bool isAutoRestore) :
	isBlock_(false),
	isAutoRestore_(isAutoRestore)
{
	sigemptyset(&oldSet_);
	sigemptyset(&newSet_);
}

//-----------------------------------------------------------------------------

SignalMasker::~SignalMasker()
{
	if (isAutoRestore_) restore();
}

//-----------------------------------------------------------------------------
// 描述: 设置 Block/UnBlock 操作所需的信号集合
//-----------------------------------------------------------------------------
void SignalMasker::setSignals(int sigCount, va_list argList)
{
	sigemptyset(&newSet_);
	for (int i = 0; i < sigCount; i++)
		sigaddset(&newSet_, va_arg(argList, int));
}

//-----------------------------------------------------------------------------

void SignalMasker::setSignals(int sigCount, ...)
{
	va_list argList;
	va_start(argList, sigCount);
	setSignals(sigCount, argList);
	va_end(argList);
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中添加 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::block()
{
	sigProcMask(SIG_BLOCK, &newSet_, &oldSet_);
	isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 在进程当前阻塞信号集中解除 setSignals 设置的信号
//-----------------------------------------------------------------------------
void SignalMasker::unBlock()
{
	sigProcMask(SIG_UNBLOCK, &newSet_, &oldSet_);
	isBlock_ = true;
}

//-----------------------------------------------------------------------------
// 描述: 将进程阻塞信号集恢复为 Block/UnBlock 之前的状态
//-----------------------------------------------------------------------------
void SignalMasker::restore()
{
	if (isBlock_)
	{
		sigProcMask(SIG_SETMASK, &oldSet_, NULL);
		isBlock_ = false;
	}
}

//-----------------------------------------------------------------------------

int SignalMasker::sigProcMask(int how, const sigset_t *newSet, sigset_t *oldSet)
{
	int result;
	if ((result = sigprocmask(how, newSet, oldSet)) < 0)
		ThrowException(strerror(errno));

	return result;
}

#endif


///////////////////////////////////////////////////////////////////////////////
// class Url

Url::Url(const std::string& url)
{
	setUrl(url);
}

Url::Url(const Url& src)
{
	(*this) = src;
}

//-----------------------------------------------------------------------------

void Url::clear()
{
	protocol_.clear();
	host_.clear();
	port_.clear();
	path_.clear();
	fileName_.clear();
	bookmark_.clear();
	userName_.clear();
	password_.clear();
	params_.clear();
}

//-----------------------------------------------------------------------------

Url& Url::operator = (const Url& rhs)
{
	if (this == &rhs) return *this;

	protocol_ = rhs.protocol_;
	host_ = rhs.host_;
	port_ = rhs.port_;
	path_ = rhs.path_;
	fileName_ = rhs.fileName_;
	bookmark_ = rhs.bookmark_;
	userName_ = rhs.userName_;
	password_ = rhs.password_;
	params_ = rhs.params_;

	return *this;
}

//-----------------------------------------------------------------------------

std::string Url::getUrl() const
{
	const char SEP_CHAR = '/';
	std::string result;

	if (!protocol_.empty())
		result = protocol_ + "://";

	if (!userName_.empty())
	{
		result += userName_;
		if (!password_.empty())
			result = result + ":" + password_;
		result += "@";
	}

	result += host_;

	if (!port_.empty())
	{
		if (sameText(protocol_, "HTTP"))
		{
			if (port_ != "80")
				result = result + ":" + port_;
		}
		else if (sameText(protocol_, "HTTPS"))
		{
			if (port_ != "443")
				result = result + ":" + port_;
		}
		else if (sameText(protocol_, "FTP"))
		{
			if (port_ != "21")
				result = result + ":" + port_;
		}
	}

	// path and filename
	std::string str = path_;
	if (!str.empty() && str[str.length() - 1] != SEP_CHAR)
		str += SEP_CHAR;
	str += fileName_;

	if (!str.empty())
	{
		if (!result.empty() && str[0] == SEP_CHAR) str.erase(0, 1);
		if (!host_.empty() && result[result.length() - 1] != SEP_CHAR)
			result += SEP_CHAR;
		result += str;
	}

	if (!params_.empty())
		result = result + "?" + params_;

	if (!bookmark_.empty())
		result = result + "#" + bookmark_;

	return result;
}

//-----------------------------------------------------------------------------

std::string Url::getUrl(UINT parts)
{
	Url url(*this);

	if (!(parts & URL_PROTOCOL)) url.setProtocol("");
	if (!(parts & URL_HOST)) url.setHost("");
	if (!(parts & URL_PORT)) url.setPort("");
	if (!(parts & URL_PATH)) url.setPath("");
	if (!(parts & URL_FILENAME)) url.setFileName("");
	if (!(parts & URL_BOOKMARK)) url.setBookmark("");
	if (!(parts & URL_USERNAME)) url.setUserName("");
	if (!(parts & URL_PASSWORD)) url.setPassword("");
	if (!(parts & URL_PARAMS)) url.setParams("");

	return url.getUrl();
}

//-----------------------------------------------------------------------------

void Url::setUrl(const std::string& value)
{
	clear();

	std::string url(value);
	if (url.empty()) return;

	// get the bookmark
	std::string::size_type pos = url.rfind('#');
	if (pos != std::string::npos)
	{
		bookmark_ = url.substr(pos + 1);
		url.erase(pos);
	}

	// get the parameters
	pos = url.find('?');
	if (pos != std::string::npos)
	{
		params_ = url.substr(pos + 1);
		url = url.substr(0, pos);
	}

	std::string buffer;
	pos = url.find("://");
	if (pos != std::string::npos)
	{
		protocol_ = url.substr(0, pos);
		url.erase(0, pos + 3);
		// get the user name, password, host and the port number
		buffer = fetchStr(url, '/', true);
		// get username and password
		pos = buffer.find('@');
		if (pos != std::string::npos)
		{
			password_ = buffer.substr(0, pos);
			buffer.erase(0, pos + 1);
			userName_ = fetchStr(password_, ':');
			if (userName_.empty())
				password_.clear();
		}
		// get the host and the port number
		std::string::size_type p1, p2;
		if ((p1 = buffer.find('[')) != std::string::npos &&
			(p2 = buffer.find(']')) != std::string::npos &&
			p2 > p1)
		{
			// this is for IPv6 Hosts
			host_ = fetchStr(buffer, ']');
			fetchStr(host_, '[');
			fetchStr(buffer, ':');
		}
		else
			host_ = fetchStr(buffer, ':', true);
		port_ = buffer;
		// get the path
		pos = url.rfind('/');
		if (pos != std::string::npos)
		{
			path_ = "/" + url.substr(0, pos + 1);
			url.erase(0, pos + 1);
		}
		else
			path_ = "/";
	}
	else
	{
		// get the path
		pos = url.rfind('/');
		if (pos != std::string::npos)
		{
			path_ = url.substr(0, pos + 1);
			url.erase(0, pos + 1);
		}
	}

	// get the filename
	fileName_ = url;
}

