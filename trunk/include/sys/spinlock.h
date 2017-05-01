#ifndef _SYS_SPINLOCK_H_
#define _SYS_SPINLOCK_H_

#include "exception.h"

class SpinLock
{
public:
	SpinLock() throw (SysCallException);

	~SpinLock() throw ();

	void lock() const throw (SysCallException);

	void unlock() const throw (SysCallException);

	void operator =(SpinLock& other);
	SpinLock(const SpinLock& other);

	// ÍÐ¹ÜCSpinLock
	class Locker
	{
	private:
		const SpinLock& _spinlock;
	public:
		Locker(const SpinLock& s) : _spinlock(s)
		{
			_spinlock.lock();
		}
		
		~Locker()
		{
			_spinlock.unlock();
		}
	};
	
private:
	
#ifdef HAVE_PTHREAD_SPINLOCK
	pthread_spinlock_t _lock;
#else
	pthread_mutex_t _lock;
#endif
};

#endif
