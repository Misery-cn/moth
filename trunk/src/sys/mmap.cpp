#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include "define.h"
#include "mmap.h"

mmap_t* MMap::mmap_read_only(int32_t fd, size_t size, size_t offset, size_t size_max) throw (SysCallException)
{
	return do_map(PROT_READ, fd, size, offset, size_max, true);
}

mmap_t* MMap::mmap_read_only(const char * filename, size_t size_max) throw (SysCallException)
{
	int fd = open(filename, O_RDONLY);
	if (0 > fd)
	{
		THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
	}

	return do_map(PROT_READ, fd, 0, 0, size_max, false);
}

mmap_t* MMap::mmap_write_only(int fd, size_t size, size_t offset, size_t size_max) throw (SysCallException)
{
    return do_map(PROT_WRITE, fd, size, offset, size_max, true);
}

mmap_t* MMap::mmap_write_only(const char* filename, size_t size_max) throw (SysCallException)
{
    int fd = open(filename, O_WRONLY);
    if (0 > fd)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
    
    return do_map(PROT_WRITE, fd, 0, 0, size_max, false);
}

mmap_t* MMap::mmap(int fd, size_t size, size_t offset, size_t size_max) throw (SysCallException)
{
    return do_map(PROT_READ|PROT_WRITE, fd, size, offset, size_max, true);
}

mmap_t* MMap::mmap(const char* filename, size_t size_max) throw (SysCallException)
{
    int fd = open(filename, O_RDONLY|O_WRONLY);
    if (0 > fd)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "open");
    }
    
    return do_map(PROT_READ|PROT_WRITE, fd, 0, 0, size_max, false);
}

mmap_t* MMap::do_map(int prot, int fd, size_t size, size_t offset, size_t size_max, bool byfd) throw (SysCallException)
{
	mmap_t* ptr = new mmap_t();

    try
    {        
        struct stat st;
        if (-1 == fstat(fd, &st))
        {
            THROW_SYSCALL_EXCEPTION(NULL, errno, "fstat");
        }

		// 如果该文件描述符由调用者打开的,则应由调用者自己关闭
        ptr->_fd = byfd ? -1 : fd;
		// 如果size为0,则映射整个文件
        ptr->_len = (0 == size) ? ((size_t)st.st_size - offset) : (size + offset > (size_t)st.st_size) ? (size_t)st.st_size : (size + offset);
		ptr->_addr = NULL;

		// 如果超过最大映射值则不予映射
		if ((0 == size_max) || (ptr->_len < size_max))
		{
			void* addr = ::mmap(NULL, ptr->_len, prot, MAP_SHARED, fd, offset);
			if (MAP_FAILED == addr)
			{
			    THROW_SYSCALL_EXCEPTION(NULL, errno, "mmap");
			}

			ptr->_addr = addr;
		}        
        
        return ptr;
    }
    catch (SysCallException& ex)
    {
        // 关闭文件
        if (ptr->_fd > -1)
        {
            close(ptr->_fd);
            ptr->_fd = -1;
        }
        
		DELETE_P(ptr)
        throw;
    }    
}

void MMap::unmap(mmap_t* ptr) throw (SysCallException)
{
	if (ptr->_addr != NULL)
    {
		if (-1 == munmap(ptr->_addr, ptr->_len))
		{
		    THROW_SYSCALL_EXCEPTION(NULL, errno, "munmap");
		}
	}

    // 记得关闭打开的文件
    if (ptr->_fd > -1) 
    {
        close(ptr->_fd);
        ptr->_fd = -1;
    }
    
	DELETE_P(ptr)
}

void MMap::flush_sync(mmap_t* ptr, size_t offset, size_t length, bool invalid) throw (SysCallException)
{
    if (offset >= ptr->_len)
    {
        THROW_SYSCALL_EXCEPTION(NULL, EINVAL, NULL);
    }
    
    int flags = invalid ? MS_SYNC|MS_INVALIDATE : MS_SYNC;
    if (-1 == msync(((char*)(ptr->_addr)) + offset, (offset + length > ptr->_len) ? ptr->_len - offset : length, flags))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "msync");
    }
}

void MMap::flush_async(mmap_t* ptr, size_t offset, size_t length, bool invalid) throw (SysCallException)
{
    if (offset >= ptr->_len)
    {
        THROW_SYSCALL_EXCEPTION(NULL, EINVAL, NULL);
    }
    
    int flags = invalid ? MS_ASYNC|MS_INVALIDATE : MS_ASYNC;
    if (-1 == msync(((char*)(ptr->_addr)) + offset, (offset + length > ptr->_len) ? ptr->_len - offset : length, flags))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "msync");
    }
}
