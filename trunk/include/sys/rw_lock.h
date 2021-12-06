#ifndef _SYS_RW_LOCK_H_
#define _SYS_RW_LOCK_H_

#include <pthread.h>
#include "exception.h"

// SYS_NS_BEGIN

class RWLock
{
public:
    RWLock() throw (SysCallException);
    virtual ~RWLock() throw ();

    // 解锁
    void unlock() throw (SysCallException);
    // 读锁
    void rlock() throw (SysCallException);
    // 写锁
    void wlock() throw (SysCallException);
    // 尝试读锁
    bool try_rlock() throw (SysCallException);
    // 尝试写锁
    bool try_wlock() throw (SysCallException);

    bool timed_rlock(uint32_t millisecond) throw (SysCallException);

    bool timed_wlock(uint32_t millisecond) throw (SysCallException);
    
private:
    pthread_rwlockattr_t _attr;
    pthread_rwlock_t _rwlock;
};

class RWLockGuard
{
public:
    // 默认加读锁
    RWLockGuard(RWLock& lock, bool is_read = true) : _lock(lock)
    {
        if (is_read)
        {
            _lock.rlock();
        }
        else
        {
            _lock.wlock();
        }
    }    
    
    ~RWLockGuard()
    {
        _lock.unlock();
    }
    
private:
    RWLock& _lock;
};

// SYS_NS_END

#endif

