#include "cond.h"

CCond::CCond()
{
	pthread_condattr_t attr;
    pthread_condattr_init(&attr);
	
	int r = pthread_cond_init(&cond_, &attr);
    if (0 != r)
    {
		// RUNLOG or throw exception
	}
}

CCond::~CCond()
{
	pthread_cond_destroy(&cond_);
}

void CCond::wait(CMutex& lock)
{
	int r = pthread_cond_wait(&cond_, &lock.mutex_);
    if (0 != r)
    {
		// RUNLOG or throw exception
	}
}

bool CCond::timed_wait(CMutex& lock, unsigned int millisecond)
{
	int r;

	if (0 == millisecond)
	{
		r = pthread_cond_wait(&cond_, &lock.mutex_);
	}
	else
	{
		// struct timeval tv;
		struct timespec ts;

#if _POSIX_C_SOURCE >= 199309L
        if (-1 == clock_gettime(CLOCK_REALTIME, &ts))
        {
			// RUNLOG or throw exception
		}

        ts.tv_sec += millisecond / 1000;
        ts.tv_nsec += (millisecond % 1000) * 1000000;
#else
        struct timeval tv;
        if (-1 == gettimeofday(&tv, NULL))
        {
			// RUNLOG or throw exception
		}

        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000;
        ts.tv_sec += millisecond / 1000;
        ts.tv_nsec += (millisecond % 1000) * 1000000;
#endif // _POSIX_C_SOURCE
        
        // ´¦Àítv_nsecÒç³ö
        if (1000000000L <= ts.tv_nsec)
        {
            ++ts.tv_sec;
            ts.tv_nsec %= 1000000000L;
        }

		r = pthread_cond_timedwait(&cond_, &lock.mutex_, &ts);
	}

    if (0 == r)
	{
		return true;
	}
	
    if (ETIMEDOUT == r)
	{
		return false;
	}

    // RUNLOG or throw exception
}

void CCond::signal()
{
	int r = pthread_cond_signal(&cond_);
    if (0 != r)
	{
		// RUNLOG or throw exception
	}
}

void CCond::broadcast()
{
	int r = pthread_cond_broadcast(&cond_);
    if (0 != r)
	{
		// RUNLOG or throw exception
	}
}