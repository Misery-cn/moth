#include "exception.h"

class ShareMemory
{
public:
    ShareMemory() throw () : _shmid(-1), _shmaddr(NULL)
    {
    }

    ~ShareMemory()
    {
    }
    
    void open(const char* path) throw (SysCallException);
    
    bool create(const char* path, mode_t mode = IPC_DEFAULT_PERM) throw (SysCallException);

    void close() throw (SysCallException);

    void* attach(int flag) throw (SysCallException);

    void detach() throw (SysCallException);
    
    void* get_shared_memory_address() const;
    
private:
    int _shmid;
    void* _shmaddr;
};

