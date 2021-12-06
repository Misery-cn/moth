#ifndef _SHM_LOCK_H_
#define _SHM_LOCK_H_

#include <stdio.h>
#include <sys/sem.h>
#include <sys/types.h>

class ShmRWLock
{
public:
    ShmRWLock();
    ShmRWLock(int key);

    int rlock() const;

    int un_rlock() const;

    bool try_rlock() const;

    int wlock() const;

    int un_wlock() const;

    bool try_wlock() const;

    int lock() const;

    int unlock() const;

    bool try_lock() const;

    int getkey() const;
    
    int getid() const;

public:
    class ShmRLocker
    {
    public:
        ShmRLocker() : _rwlock(NULL) { }
        
        ShmRLocker(ShmRWLock* lock) : _rwlock(lock)
        {
            if (NULL != _rwlock)
            {
                _rwlock->rlock();
            }
        }

        void init_lock(ShmRWLock* lock)
        {
            _rwlock = lock;
            _rwlock->rlock();
        }

        ~ShmRLocker()
        {
            _rwlock->un_rlock();
        }
    private:
        ShmRWLock* _rwlock;
    };

    class ShmWLocker
    {
    public:
        ShmWLocker() : _rwlock(NULL) { }
        
        ShmWLocker(ShmRWLock* lock) : _rwlock(lock)
        {
            if (NULL != _rwlock)
            {
                _rwlock->wlock();
            }
        }

        void init_lock(ShmRWLock* lock)
        {
            _rwlock = lock;
            _rwlock->wlock();
        }


        ~ShmWLocker()
        {
            if (NULL != _rwlock)
            {
                _rwlock->un_wlock();
            }
        }
    private:
        ShmRWLock* _rwlock;
    };

private:
    //初始化
    void init(int key);
    
protected:
    int _sem_id;
    int _sem_key;
};

#endif

