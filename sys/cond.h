#include "mutex.h"

class CCond
{
public:
    CCond();
    ~CCond();

	// 线程挂起,等待被唤醒
    void wait(CMutex& lock);

	// 线程挂起,被唤醒或者超时
    bool timed_wait(CMutex& lock, unsigned int millisecond);

	// 唤醒挂起的线程
    void signal();

	// 唤醒所有的挂起线程
    void broadcast();
    
private:
    pthread_cond_t cond_;
};