#ifndef _SHM_QUEUE_H_
#define _SHM_QUEUE_H_

#include "shm_lock.h"
#include "share_memory.h"

#define CACHELINE_SIZE 64
// 修改字对齐规则，避免false sharing
#define CACHELINE_ALIGN  __attribute__((aligned(CACHELINE_SIZE)))

#define CACHE_LINE_SIZE 64 //cache line 大小

#define EXTRA_BYTE 8

// 内存屏障
#define __MEM_BARRIER \
    __asm__ __volatile__("mfence":::"memory")
// 内存读屏障
#define __READ_BARRIER__ \
    __asm__ __volatile__("lfence":::"memory")
// 内存写屏障
#define __WRITE_BARRIER__ \
    __asm__ __volatile__("sfence":::"memory")


enum QueueMode
{
    ONE_READ_ONE_WRITE, // 一个进程读一个进程
    ONE_READ_MUL_WRITE, // 一个进程读多个进程写
    MUL_READ_ONE_WRITE, // 多个进程一个进程写
    MUL_READ_MUL_WRITE  // 多个进程读多个进程写
};

enum ShmType
{
    SHM_INIT,   // 初始化
    SHM_RESUME  // 共享内存已存在
};

class CACHELINE_ALIGN ShmQueue
{
public:
    
    ShmQueue(ShareMemory* shm, QueueMode mode = MUL_READ_MUL_WRITE);
    
    ~ShmQueue();

    /**
     * 写共享内存
     * @param msg 需要写到共享内存中的数据
     * @param length 消息的长度
     * @return
     */
    int send(char* msg, uint32_t length);

    /**
     * 读共享内存
     * @param msg 从共享内存中读取的数据
     * @return
     */
    int recv(char* msg);

    /**
     * 从queue头部读取一个消息
     * @param msg 从共享内存中读取的数据
     * @return
     */
    int recv_head(char* msg);

    /**
     * 从queue头部删除一个消息
     * @param msg 从共享内存中读取的数据
     * @return
     */
    int delete_head();

    /**
     * 输出queue里的内容
     */
    void dump();
    
private:
    // 获取共享内存queue剩余大小
    uint32_t get_free_size();
    
    // 获取数据大小
    uint32_t get_data_size();
    
    // 获取共享内存queue长度
    uint32_t get_queue_length();
    
    // 初始化锁
    void init_lock();
    
    // 读端上锁
    bool rlock();
    
    // 写端上锁
    bool wlock();

    struct ShmTrunk
    {
        // 这里读写索引用int类型,cpu可以保证对float,double和long除外的基本类型的读写是原子的,保证一个线程不会读到
        // 另外一个线程写到一半的值
        volatile uint32_t _head;
        // 在变量之间插入一个64字节(cache line的长度)的变量(没有实际的计算意义),但是可以保证两个变量不会同时在一个
        // cache line里,防止不同的进程或者线程同时访问在一个cache line里不同的变量产生false sharing
        char __cache_padding1__[CACHE_LINE_SIZE];
        volatile uint32_t _tail;
        char __cache_padding2__[CACHE_LINE_SIZE];
        int _shm_key;
        char __cache_padding3__[CACHE_LINE_SIZE];
        int _shm_id;
        char __cache_padding4__[CACHE_LINE_SIZE];
        uint32_t _queue_size;
        char __cache_padding5__[CACHE_LINE_SIZE];
        QueueMode _queue_mode;
    };

private:
    
    ShareMemory* _shm;
    
    ShmTrunk* _shm_trunk;
    // queue的首地址，注意和共享内存首地址不一样
    // 共享内存头部有一个ShmTrunk信息
    char* _queue_addr;
    ShmRWLock* _rLock;
    ShmRWLock* _wLock;
};

#endif

