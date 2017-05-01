#ifndef _SINGLETON_H_
#define _SINGLETON_H_

#include <cstdlib>
#include "mutex.h"


template <typename T>
class Singleton
{
public:
        static T& instance();
        virtual void update() {};

protected:
		Singleton();

private:
	
	Singleton(const Singleton&);
	Singleton& operator=(const Singleton&);

	static void destroy_singleton();

private:
	static Mutex _lock;
	static T *_instance;
	static bool _destroyed;
};


template<typename T>
Mutex Singleton<T>::_lock;

template<typename T>
T* Singleton<T>::_instance = NULL;

template<typename T>
bool Singleton<T>::_destroyed = false;

template<typename T>
T& Singleton<T>::instance()
{
    if (!_instance)
    {
        MutexGuard guard(_lock);

        if (!_instance)
        {
			_instance = new T;
			std::atexit(destroy_singleton);
        }
    }

    return *_instance;
}

template<typename T>
void Singleton<T>::destroy_singleton()
{
    if (NULL != _instance)
    {
        delete _instance;
        _instance = NULL;
    }
}

#endif
