#ifndef _TMP_SINGLETON_H
#define  _TMP_SINGLETON_H

#include "Options.h"
#include "UtilClass.h"

///////////////////////////////////////////////////////////////////////////////
// class Singleton - 全局单例类

#ifdef _COMPILER_WIN

template<typename T>
class Singleton : noncopyable
{
public:
	static T& instance()
	{
		// DCL with volatile
		if (instance_ == NULL)
		{
			AutoLocker locker(mutex_);
			if (instance_ == NULL)
				instance_ = new T();
			return *instance_;
		}
		return *instance_;
	}
protected:
	Singleton() {}
	~Singleton() {}
private:
	static T* volatile instance_;
	static Mutex mutex_;
};

template<typename T> T* volatile Singleton<T>::instance_ = NULL;
template<typename T> Mutex Singleton<T>::mutex_;

#endif
#ifdef _COMPILER_LINUX

template<typename T>
class Singleton : noncopyable
{
public:
	static T& instance()
	{
		pthread_once(&once_, &Singleton::init);
		return *instance_;
	}
protected:
	Singleton() {}
	~Singleton() {}
private:
	static void init()
	{
		instance_ = new T();
	}
private:
	static T* instance_;
	static pthread_once_t once_;
};

template<typename T> T* Singleton<T>::instance_ = NULL;
template<typename T> pthread_once_t Singleton<T>::once_ = PTHREAD_ONCE_INIT;

#endif


#endif // 
