#include <sys/shm.h>
#include "shm_queue.h"
#include "intarith.h"

ShmQueue::ShmQueue(ShareMemory* shm, QueueMode mode) : _shm(shm)
{
    _queue_addr = (char*)_shm->get_shm_address();
    // 共享内存头部存放ShmTrunk信息
    // _shm_trunk = new (_queue_addr)ShmTrunk();
    _shm_trunk = (ShmTrunk*)_queue_addr;
    // 队列首地址
    _queue_addr += sizeof(ShmTrunk);

    struct shmid_ds buf;
    _shm->stat(&buf);
    if (1 == buf.shm_nattch)
    {
        // 队列的头尾的索引
        _shm_trunk->_head = 0;
        _shm_trunk->_tail = 0;
        _shm_trunk->_shm_key = _shm->get_shmkey();
        _shm_trunk->_shm_id = _shm->get_shmid();
        // _shm_trunk->_shm_size = _shm->get_shmsize();
        _shm_trunk->_queue_size = _shm->get_shmsize() - sizeof(ShmTrunk);
        _shm_trunk->_queue_mode = mode;
    }
    else
    {
        dump();
    }
    
    init_lock();
}

ShmQueue::~ShmQueue()
{
    if (_shm_trunk)
    {
        _shm->close();
        // delete _shm_trunk;
        _shm_trunk->~ShmTrunk();
    }
    
    if (_rLock)
    {
        delete _rLock;
        _rLock = NULL;
    }
    
    if (_wLock)
    {
        delete _wLock;
        _wLock = NULL;
    }
}

int ShmQueue::send(char* message, uint32_t length)
{
    if (!message || length <= 0)
    {
        return -1;
    }

    ShmRWLock::ShmWLocker wLocker;
    // 尝试获取写锁
    if (wlock() && _wLock)
    {
        wLocker.init_lock(_wLock);
    }

    // 首先判断是否队列已满
    int freesize = get_free_size();
    // printf("queue size is %d\n", size);
    if (0 >= freesize)
    {
        printf("queue size not enough, %d\n", freesize);
        return -1;
    }

    // 空间不足
    if ((length + sizeof(uint32_t)) > freesize)
    {
        printf("queue size not enough\n");
        return -1;
    }

    uint32_t len = length;
    char* dst = _queue_addr;
    char* src = (char *)(&len);

    // 写入的时候我们在数据头插上数据的长度，方便准确取数据
    // 防止每次写入一个字节可能会分散在队列的头和尾
    uint32_t end = _shm_trunk->_tail;
    for (uint32_t i = 0; i < sizeof(len); i++)
    {
        dst[end] = src[i];
        // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
        end = (end + 1) % _shm_trunk->_queue_size;
        // end = (end + 1) & (_shm_trunk->_queue_size - 1);
    }

    uint32_t minlen = MIN(len, _shm_trunk->_queue_size - end);
    memcpy((void *)(&dst[end]), (const void *) message, (size_t) minlen);
    size_t lastLen = length - minlen;
    if (0 < lastLen)
    {
        memcpy(&dst[0], message + minlen, lastLen);
    }

    __WRITE_BARRIER__;

    // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
    _shm_trunk->_tail = (end  + len) % _shm_trunk->_queue_size;
    // _shm_trunk->_tail = (end + len) & (_shm_trunk->_queue_size - 1);

    printf("write message %s\n", message);
    
    return 0;
}

int ShmQueue::recv(char* message)
{
    if (!message)
    {
        return -1;
    }

    ShmRWLock::ShmRLocker rLocker;
    // 修改共享内存读锁
    if (rlock() && _wLock)
    {
        rLocker.init_lock(_wLock);
    }

    int datalen = get_data_size();
    if (0 >= datalen)
    {
        return -1;
    }

    char* src = _queue_addr;
    if (datalen <= sizeof(uint32_t))
    {
        printf("[%s:%d] ReadHeadMessage data len illegal, datalen %d \n", __FILE__, __LINE__, datalen);
        dump();
        _shm_trunk->_head = _shm_trunk->_tail;
        return -1;
    }

    uint32_t dstlen;
    char* dst = (char *) (&dstlen);
    uint32_t begin = _shm_trunk->_head;
    // 读取数据长度
    for (uint32_t i = 0; i < sizeof(uint32_t); i++)
    {
        dst[i] = src[begin];
        // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
        begin = (begin + 1) % _shm_trunk->_queue_size;
        // begin = (begin + 1) & (_shm_trunk->_queue_size - 1);
    }

    // 校验数据长度
    if (dstlen > (int) (datalen - sizeof(uint32_t)) || dstlen < 0)
    {
        printf("[%s:%d] ReadHeadMessage Length illegal, dstlen: %d, datalen %d \n", __FILE__, __LINE__, dstlen, datalen);
        dump();
        _shm_trunk->_head = _shm_trunk->_tail;
        return -1;
    }

    dst = &message[0];
    uint32_t minlen = MIN(dstlen, _shm_trunk->_queue_size - begin);
    memcpy(&dst[0], &src[begin], minlen);
    uint32_t lastlen = dstlen - minlen;
    if (0 < lastlen)
    {
        memcpy(&dst[minlen], src, lastlen);
    }

    __WRITE_BARRIER__;

    // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
    _shm_trunk->_head = (begin + dstlen) % _shm_trunk->_queue_size;
    // _shm_trunk->_head = (begin + dstlen) & (_shm_trunk->_queue_size - 1);

    printf("read message %s\n", message);
    
    return dstlen;
}

int ShmQueue::recv_head(char* head)
{
    if (!head)
    {
        return -1;
    }

    ShmRWLock::ShmRLocker rLocker;
    // 共享内存读锁
    if (rlock() && _rLock)
    {
        rLocker.init_lock(_rLock);
    }

    int datalen = get_data_size();
    if (0 >= datalen)
    {
        return -1;
    }

    char* src = _queue_addr;
    if (datalen <= (int) sizeof(uint32_t))
    {
        printf("[%s:%d] ReadHeadMessage data len illegal, datalen %d \n", __FILE__, __LINE__, datalen);
        dump();
        return -1;
    }

    uint32_t dstlen;
    char* dst = (char *) (&dstlen);
    uint32_t begin = _shm_trunk->_head;
    // 读取数据长度
    for (uint32_t i = 0; i < sizeof(uint32_t); i++)
    {
        dst[i] = src[begin];
        // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
        begin = (begin + 1) % _shm_trunk->_queue_size;
        // begin = (begin + 1) & (_shm_trunk->_queue_size - 1);
    }

    // 校验数据长度
    if (dstlen > (int) (datalen - sizeof(uint32_t)) || dstlen < 0)
    {
        printf("[%s:%d] ReadHeadMessage Length illegal, dstlen: %d, datalen %d \n", __FILE__, __LINE__, dstlen, datalen);
        dump();
        return -1;
    }

    dst = &head[0];

    // uint32_t index = begin & (_shm_trunk->_queue_size - 1);
    uint32_t index = begin % _shm_trunk->_queue_size;
    uint32_t minlen = MIN(dstlen, _shm_trunk->_queue_size - index);
    memcpy(dst, src + begin, minlen);
    uint32_t lastlen = dstlen - minlen;
    if (0 < lastlen)
    {
        memcpy(dst + minlen, src, lastlen);
    }
    
    return dstlen;
}

int ShmQueue::delete_head()
{
    ShmRWLock::ShmWLocker wLocker;
    // 修改共享内存写锁
    if (wlock() && _wLock) {
        wLocker.init_lock(_wLock);
    }

    int datalen = get_data_size();
    if (0 >= datalen)
    {
        return -1;
    }

    char* src = _queue_addr;
    if (datalen <= sizeof(uint32_t))
    {
        printf("[%s:%d] ReadHeadMessage data len illegal, datalen %d \n", __FILE__, __LINE__, datalen);
        dump();
        _shm_trunk->_head = _shm_trunk->_tail;
        return -1;
    }

    uint32_t dstlen;
    char* dst = (char*) (&dstlen);
    uint32_t begin = _shm_trunk->_head;
    // 取出数据的长度
    for (uint32_t i = 0; i < sizeof(uint32_t); i++)
    {
        dst[i] = src[begin];
        // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
        begin = (begin + 1) % _shm_trunk->_queue_size;
        // begin = (begin + 1) & (_shm_trunk->_queue_size -1);
    }

    // 校验数据长度
    if (dstlen > (int) (datalen - sizeof(uint32_t)) || dstlen < 0)
    {
        printf("[%s:%d] ReadHeadMessage Length illegal, dstlen: %d, datalen %d \n", __FILE__, __LINE__, dstlen, datalen);
        dump();
        _shm_trunk->_head = _shm_trunk->_tail;
        return -1;
    }

    // 需要保证共享内存size是2的倍数，不然不能使用下列方法取余
    _shm_trunk->_head = (begin + dstlen) % _shm_trunk->_queue_size;
    // _shm_trunk->_head = (begin + dstlen) & (_shm_trunk->_queue_size - 1);
    
    return dstlen;
}

void ShmQueue::dump()
{
    printf("Mem trunk address 0x%p, shmkey %d, shmid %d, size %d, begin %d, end %d, queue mode %d\n",
           _shm_trunk, _shm_trunk->_shm_key, _shm_trunk->_shm_id, _shm_trunk->_queue_size,
           _shm_trunk->_head, _shm_trunk->_tail, _shm_trunk->_queue_mode);
}

// 获取空闲区大小
uint32_t ShmQueue::get_free_size()
{
    // 长度应该减去预留部分长度8，保证首尾不会相接
    return get_queue_length() - get_data_size();
}

// 获取数据长度
uint32_t ShmQueue::get_data_size()
{
    // 第一次写数据前
    if (_shm_trunk->_head == _shm_trunk->_tail)
    {
        return 0;
    }
    // 数据在两头
    else if (_shm_trunk->_head > _shm_trunk->_tail)
    {
        return _shm_trunk->_tail + _shm_trunk->_queue_size - _shm_trunk->_head;
    }
    // 数据在中间
    else
    {
        return _shm_trunk->_tail - _shm_trunk->_head;
    }
}

uint32_t ShmQueue::get_queue_length()
{
    return _shm_trunk->_queue_size;
}

void ShmQueue::init_lock()
{
    if (rlock())
    {
        _rLock = new ShmRWLock(_shm_trunk->_shm_key + 1);
    }

    if (wlock())
    {
        _wLock = new ShmRWLock(_shm_trunk->_shm_key + 2);
    }
}

bool ShmQueue::rlock()
{
    return (_shm_trunk->_queue_mode == QueueMode::MUL_READ_MUL_WRITE ||
        _shm_trunk->_queue_mode == QueueMode::MUL_READ_ONE_WRITE);
}

bool ShmQueue::wlock()
{
    return (_shm_trunk->_queue_mode == QueueMode::MUL_READ_MUL_WRITE ||
        _shm_trunk->_queue_mode == QueueMode::ONE_READ_MUL_WRITE);
}

