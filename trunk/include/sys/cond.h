#ifndef _SYS_COND_H_
#define _SYS_COND_H_

#include "mutex.h"

// SYS_NS_BEGIN

class Cond
{
public:
    Cond() throw (SysCallException);

	// 不会抛出异常
    ~Cond() throw ();

	// 线程挂起,等待被唤醒
    void wait(Mutex& lock) throw (SysCallException);

	// 线程挂起,被唤醒或者超时
    bool timed_wait(Mutex& lock, uint32_t millisecond) throw (SysCallException);

	// 唤醒挂起的线程
    void signal() throw (SysCallException);

	// 唤醒所有的挂起线程
    void broadcast() throw (SysCallException);
    
private:
	pthread_condattr_t _attr;
    pthread_cond_t _cond;
};

// SYS_NS_END

#endif