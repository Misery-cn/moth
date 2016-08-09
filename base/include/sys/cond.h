#ifndef _SYS_COND_H_
#define _SYS_COND_H_

#include "mutex.h"

SYS_NS_BEGIN

class CCond
{
public:
    CCond() throw (CSysCallException);

	// 不会抛出异常
    ~CCond() throw ();

	// 线程挂起,等待被唤醒
    void wait(CMutex& lock) throw (CSysCallException);

	// 线程挂起,被唤醒或者超时
    bool timed_wait(CMutex& lock, uint32_t millisecond) throw (CSysCallException);

	// 唤醒挂起的线程
    void signal() throw (CSysCallException);

	// 唤醒所有的挂起线程
    void broadcast() throw (CSysCallException);
    
private:
	pthread_condattr_t attr_;
    pthread_cond_t cond_;
};

SYS_NS_END

#endif