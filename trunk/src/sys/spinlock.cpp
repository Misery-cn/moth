#include "spinlock.h"

SpinLock::SpinLock() throw (SysCallException)
{
	int r = 0;
#ifdef HAVE_PTHREAD_SPINLOCK
	r = pthread_spin_init(_lock, PTHREAD_PROCESS_PRIVATE);
else
	r = pthread_mutex_init(_lock, NULL);
#endif

	if (0 != r) 
	{
		// RUNLOG or throw Exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_spin_init");
    }
}

SpinLock::~SpinLock() throw ()
{
#ifdef HAVE_PTHREAD_SPINLOCK
	pthread_spin_destroy(_lock);
else
	pthread_mutex_destroy(_lock);
#endif
}

void SpinLock::lock() const throw (SysCallException)
{
	int r = 0;
#ifdef HAVE_PTHREAD_SPINLOCK
	r = pthread_spin_lock(_lock);
else
	r = pthread_mutex_lock(_lock);
#endif

	if (0 != r) 
	{
		// RUNLOG or throw Exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_spin_lock");
    }
}

void SpinLock::unlock() const throw (SysCallException)
{
	int r = 0;
#ifdef HAVE_PTHREAD_SPINLOCK
	r = pthread_spin_unlock(_lock);
else
	r = pthread_mutex_unlock(_lock);
#endif

	if (0 != r) 
	{
		// RUNLOG or throw Exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_spin_unlock");
	}
}

