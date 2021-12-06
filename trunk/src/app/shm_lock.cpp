#include "shm_lock.h"
#include "exception.h"

ShmRWLock::ShmRWLock() : _sem_id(-1), _sem_key(-1)
{
}

ShmRWLock::ShmRWLock(int key)
{
    init(key);
}

void ShmRWLock::init(int key)
{
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
    /* union semun is defined by including <sys/sem.h> */
#else
    /* according to X/OPEN we have to define it ourselves */
    union semun
    {
        int val;                  /* value for SETVAL */
        struct semid_ds* buf;     /* buffer for IPC_STAT, IPC_SET */
        unsigned short* array;    /* array for GETALL, SETALL */
        /* Linux specific part: */
        struct seminfo *__buf;    /* buffer for IPC_INFO */
    };
#endif
    int semid;
    union semun arg;
    // 创建两个信号量,第一个为写信号量，第二个为读信号量
    u_short array[2] = {0, 0};
    // 生成信号量集
    if (-1 != (semid = semget(key, 2, IPC_CREAT | IPC_EXCL | 0666)))
    {
        arg.array = &array[0];

        // 将所有信号量的值设置为0
        if (-1 == semctl(semid, 0, SETALL, arg))
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "semctl"); 
        }
    }
    else
    {
        // 如果失败，判断信号量是否已经存在
        if (errno != EEXIST)
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "semget"); 
        }

        // 连接信号量
        if (-1 == (semid = semget(key, 2, 0666)))
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "semget"); 
        }
    }

    _sem_id = semid;
    _sem_key = key;
}

int ShmRWLock::rlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 获取读锁
     * 等待写信号量变为0：{0, 0, SEM_UNDO},并且把读信号加一：{1, 1, SEM_UNDO}
     */

    // 信号量操作集
    struct sembuf sops[2] = {{0, 0, SEM_UNDO}, {1, 1, SEM_UNDO}};
    size_t nsops = 2;
    int ret = -1;

    do
    {
        ret = semop(_sem_id, &sops[0], nsops);
    } while ((-1 == ret) && (EINTR == errno));

    return ret;
}

int ShmRWLock::un_rlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 解除读锁
     * 把读信号量（第二个）减一：{1, -1, SEM_UNDO}
     */
    struct sembuf sops[1] = {{1, -1, SEM_UNDO}};
    size_t nsops = 1;

    int ret = -1;

    do
    {
        ret = semop(_sem_id, &sops[0], nsops);
    } while ((-1 == ret) && (EINTR == errno));

    return ret;
}

bool ShmRWLock::try_rlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 获取读锁
     * 尝试等待写信号量（第一个）变为0：{0, 0,SEM_UNDO | IPC_NOWAIT},
     * 把读信号量（第一个）加一：{1, 1,SEM_UNDO | IPC_NOWAIT}
     */
    struct sembuf sops[2] = {{0, 0, SEM_UNDO | IPC_NOWAIT}, {1, 1, SEM_UNDO | IPC_NOWAIT}};
    size_t nsops = 2;

    int ret = semop(_sem_id, &sops[0], nsops);
    if (-1 == ret)
    {
        if (EAGAIN == errno)
        {
            // 无法获得锁
            return false;
        }
        else
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "semop"); 
        }
    }
    return true;
}

int ShmRWLock::wlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 获取写锁
     * 尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO},并且等待读信号量（第二个）变为0：{0, 0, SEM_UNDO}
     * 把写信号量（第一个）加一：{0, 1, SEM_UNDO}
     */
    struct sembuf sops[3] = {{0, 0, SEM_UNDO}, {1, 0, SEM_UNDO}, {0, 1, SEM_UNDO}};
    size_t nsops = 3;

    int ret = -1;

    do
    {
        ret = semop(_sem_id, &sops[0], nsops);
    } while ((-1 == ret) && (EINTR == errno));

    return ret;
}

int ShmRWLock::un_wlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 解除写锁
     * 把写信号量（第一个）减一：{0, -1, SEM_UNDO}
     */
    struct sembuf sops[1] = {{0, -1, SEM_UNDO}};
    size_t nsops = 1;

    int ret = -1;

    do
    {
        ret = semop(_sem_id, &sops[0], nsops);
    } while ((-1 == ret) && (EINTR == errno));

    return ret;
}

bool ShmRWLock::try_wlock() const
{
    /**包含两个信号量,第一个为写信号量，第二个为读信号量
     * 尝试获取写锁
     * 尝试等待写信号量（第一个）变为0：{0, 0, SEM_UNDO | IPC_NOWAIT},并且尝试等待读信号量（第二个）变为0：{0, 0, SEM_UNDO | IPC_NOWAIT}
     * 把写信号量（第一个）加一：{0, 1, SEM_UNDO | IPC_NOWAIT}
     */
    struct sembuf sops[3] = {{0, 0, SEM_UNDO | IPC_NOWAIT},
                             {1, 0, SEM_UNDO | IPC_NOWAIT},
                             {0, 1, SEM_UNDO | IPC_NOWAIT}};
    size_t nsops = 3;

    int ret = semop(_sem_id, &sops[0], nsops);
    if (-1 == ret)
    {
        if (EAGAIN == errno)
        {
            // 无法获得锁
            return false;
        }
        else
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "semop"); 
        }
    }

    return true;
}

int ShmRWLock::lock() const
{
    return wlock();
}

int ShmRWLock::unlock() const
{
    return un_wlock();
}

bool ShmRWLock::try_lock() const
{
    return try_wlock();
}

int ShmRWLock::getkey() const
{
    return _sem_key;
}

int ShmRWLock::getid() const
{
    return _sem_id;
}

