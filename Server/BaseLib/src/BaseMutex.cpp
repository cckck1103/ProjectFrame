#include "BaseMutex.h"

///////////////////////////////////////////////////////////////////////////////
// class Mutex

Mutex::Mutex()
{
#ifdef _COMPILER_WIN
	InitializeCriticalSection(&critiSect_);
#endif
#ifdef _COMPILER_LINUX
	pthread_mutexattr_t attr;

	// ������˵��:
	// PTHREAD_MUTEX_TIMED_NP:
	//   ��ͨ����ͬһ�߳��ڱ���ɶԵ��� Lock �� Unlock�������������ö�� Lock�������������
	// PTHREAD_MUTEX_RECURSIVE_NP:
	//   Ƕ�������߳��ڿ���Ƕ�׵��� Lock����һ����Ч��֮����������ͬ������ Unlock ���ɽ�����
	// PTHREAD_MUTEX_ERRORCHECK_NP:
	//   ����������ͬһ�߳�Ƕ�׵��� Lock ���������
	// PTHREAD_MUTEX_ADAPTIVE_NP:
	//   ��Ӧ����
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
	// �����δ����������� destroy���˺����᷵�ش��� EBUSY��
	// �� linux �£���ʹ�˺������ش���Ҳ��������Դй©��
	pthread_mutex_destroy(&mutex_);
#endif
}


//-----------------------------------------------------------------------------
// ����: ����
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
// ����: ����
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
// ����: �����ֵԭ�ӵؼ�1����ʾ����һ���ɷ��ʵ���Դ
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
// ����: �ȴ����(ֱ�����ֵ����0)��Ȼ�����ֵԭ�ӵؼ�1
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
// ����: ��������ã������ֵ��Ϊ��ʼ״̬
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
