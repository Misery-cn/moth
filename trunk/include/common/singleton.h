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
	static CMutex _lock;
	static T *_instance;
	static bool _destroyed;
};


template<typename T>
CMutex CSingleton<T>::_lock;

template<typename T>
T* CSingleton<T>::_instance = NULL;

template<typename T>
bool CSingleton<T>::_destroyed = false;

template<typename T>
T& CSingleton<T>::instance()
{
    if (!_instance)
    {
        CMutexGuard guard(_lock);

        if (!_instance)
        {
			_instance = new T;
			std::atexit(destroy_singleton);
        }
    }

    return *_instance;
}

template<typename T>
void CSingleton<T>::destroy_singleton()
{
    if (NULL != _instance)
    {
        delete _instance;
        _instance = NULL;
    }
}

#endif
