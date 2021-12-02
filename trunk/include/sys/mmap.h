#include "exception.h"

typedef struct
{
    int32_t _fd;    // 打开的文件描述符,如果是调用者打开则应有调用者自己关闭
    void* _addr;    // 文件映射到内存的地址
    size_t _len;    // 映射到内存的大小
} mmap_t;

class MMap
{
public:
        // 只读方式映射
        // size:需要映射到内存的大小.如果为0默认映射整个文件
        // offet:映射偏移量
        // size_max:最大映射字节
        static mmap_t* mmap_read_only(int fd, size_t size = 0, size_t offset = 0, size_t size_max = 0) throw (SysCallException);

        static mmap_t* mmap_read_only(const char* filename, size_t size_max = 0) throw (SysCallException);

        // 只写方式映射
        static mmap_t* mmap_write_only(int fd, size_t size = 0, size_t offset = 0, size_t size_max = 0) throw (SysCallException);

        static mmap_t* mmap_write_only(const char* filename, size_t size_max = 0) throw (SysCallException);

        // 可读写方式映射
        static mmap_t* mmap(int fd, size_t size = 0, size_t offset = 0, size_t size_max = 0) throw (SysCallException);

        static mmap_t* mmap(const char* filename, size_t size_max = 0) throw (SysCallException);
        // 卸载内存映射
        static void unmap(mmap_t* ptr) throw (SysCallException);

        // 同步方式刷回磁盘
        static void flush_sync(mmap_t* ptr, size_t offset = 0, size_t length = 0, bool invalid = false) throw (SysCallException);
        // 异步方式刷回磁盘
        static void flush_async(mmap_t* ptr, size_t offset = 0, size_t length = 0, bool invalid = false) throw (SysCallException);

private:
        static mmap_t* do_map(int prot, int fd, size_t size, size_t offset, size_t size_max, bool byfd) throw (SysCallException);
};
