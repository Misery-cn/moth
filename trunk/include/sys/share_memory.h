#include "dir_utils.h"


#define IPCKEY 0x01


class ShareMemory
{
public:
    ShareMemory() throw () : _shmid(-1), _shmaddr(NULL)
    {
    }

    ~ShareMemory()
    {
    }

    /**
     * 只打开共享内存，获取共享内存标识
     */
    void open(const char* path) throw (SysCallException);

    /**
     * 创建共享内存
     */
    bool create(const char* path, size_t size, mode_t mode = IPC_DEFAULT_PERM) throw (SysCallException);

    void close() throw (SysCallException);

    void* attach(int flag = 0) throw (SysCallException);

    void detach() throw (SysCallException);

    int stat(struct shmid_ds* buf) throw (SysCallException);
    
    void* get_shm_address() const;

    int get_shmid() { return _shmid; }

    key_t get_shmkey() { return _shmkey; }

    uint32_t get_shmsize() { return _shmsize; }
    
private:
    // 共享内存ID
    int _shmid;
    // 共享内存地址
    void* _shmaddr;
    // 共享内存key
    key_t _shmkey;
    // 共享内存大小
    uint32_t _shmsize;
};

