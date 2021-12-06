#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "dir_utils.h"
#include "share_memory.h"

void ShareMemory::open(const char* path) throw (SysCallException)
{
    if (NULL == path)
    {
        THROW_SYSCALL_EXCEPTION(NULL, EINVAL, NULL);
    }

    key_t key = ftok(path, IPCKEY);
    if (-1 == key)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "ftok");
    }

    _shmid = shmget(key, 0, 0);
    if (-1 == _shmid)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "shmget");
    }

    _shmkey = key;
}

void ShareMemory::close() throw (SysCallException)
{
    if (-1 != _shmid)
    {
        if (-1 == shmctl(_shmid, IPC_RMID, NULL))
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "shmctl");
        }

        _shmid = -1;
    }
}


bool ShareMemory::create(const char* path, size_t size, mode_t mode) throw (SysCallException)
{
    key_t key = IPC_PRIVATE;
    
    if (path != NULL)
    {
        // 如果要确保key_t值不变，要么确保ftok的文件不被删除，要么不用ftok，指定一个固定的key_t值
        // #define IPCKEY 0x01
        // key = ftok(path, getpid());
        key = ftok(path, IPCKEY);
        if (-1 == key)
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "ftok");
        }
    }

    _shmid = shmget(key, size, IPC_CREAT | IPC_EXCL | mode);
    if (-1 == _shmid)
    {
        // 共享内存段已经存在
        if (EEXIST == errno) 
        {
            return false;
        }
        
        THROW_SYSCALL_EXCEPTION(NULL, errno, "shmget");
    }

    _shmkey = key;
    _shmsize = size;
        
    return true;
}

void ShareMemory::detach() throw (SysCallException)
{
    if (NULL != _shmaddr)
    {
        shmdt(_shmaddr);
        _shmaddr = NULL;
    }
}

void* ShareMemory::attach(int flag) throw (SysCallException)
{
    if (NULL == _shmaddr)
    {
        _shmaddr = shmat(_shmid, NULL, flag);
        if ((void*)-1 == _shmaddr)
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "shmat");
        }
    }

    return _shmaddr;
}

void* ShareMemory::get_shm_address() const
{
    return _shmaddr;
}

/**
    struct shmid_ds {
    struct ipc_perm shm_perm;
    int shm_segsz;
    __kernel_time_t     shm_atime;
    __kernel_time_t     shm_dtime;
    __kernel_time_t     shm_ctime;
    __kernel_ipc_pid_t  shm_cpid;
    __kernel_ipc_pid_t  shm_lpid;
    unsigned short      shm_nattch;
    unsigned short      shm_unused;
    void                *shm_unused2;
    void                *shm_unused3;
    };
 */
int ShareMemory::stat(struct shmid_ds* buf) throw (SysCallException)
{
    if (-1 != _shmid)
    {
        int ret = shmctl(_shmid, IPC_STAT, buf);
        if (-1 == ret)
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "shmctl");
        }
        return 0;
    }
    return -1;
}

