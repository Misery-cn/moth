#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <cstdlib>
#include "mutex.h"


template <typename T>
class CSingleton
{
public:
        static T& instance();
        virtual void update() {};

protected:
		CSingleton();

private:
	
	CSingleton(const CSingleton&);
	CSingleton& operator=(const CSingleton&);

	static void destroy_singleton();

private:
	static CMutex lock_;
	static T *instance_;
	static bool destroyed_;
};


template<typename T>
CMutex CSingleton<T>::lock_;

template<typename T>
T* CSingleton<T>::instance_ = NULL;

template<typename T>
bool CSingleton<T>::destroyed_ = false;

template<typename T>
T& CSingleton<T>::instance()
{
    if (!instance_)
    {
        CMutexGuard guard(lock_);

        if (!instance_)
        {
			instance_ = new T;
			std::atexit(destroy_singleton);
        }
    }

    return *instance_;
}

template<typename T>
void CSingleton<T>::destroy_singleton()
{
    if (NULL != instance_)
    {
        delete instance_;
        instance_ = NULL;
    }
}

#endif
