#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include "safe_io.h"

ssize_t safe_read(int fd, void* buf, size_t count)
{
	size_t cnt = 0;

	while (cnt < count)
	{
		ssize_t r = read(fd, buf, count - cnt);
		if (0 >= r)
		{
			// 读完了
			if (0 == r)
			{
				// EOF
				return cnt;
			}
			// 中断
			if (EINTR == errno)
			{
				continue;
			}
			
			return -errno;
		}
		cnt += r;
		buf = (char *)buf + r;
	}
	
	return cnt;
}

ssize_t safe_read_exact(int fd, void* buf, size_t count)
{
	ssize_t ret = safe_read(fd, buf, count);
	if (0 > ret)
	{
		return ret;
	}
	
	if ((size_t)ret != count)
	{
		return -EDOM;
	}
	
	return 0;
}
 
ssize_t safe_write(int fd, const void* buf, size_t count)
{
	while (0 < count)
	{
		ssize_t r = write(fd, buf, count);
		if (0 > r)
		{
			// 中断,继续尝试
			if (errno == EINTR)
			{
				continue;
			}
			
			return -errno;
		}
		
		count -= r;
		buf = (char *)buf + r;
	}
	return 0;
}

ssize_t safe_pread(int fd, void* buf, size_t count, off_t offset)
{
	size_t cnt = 0;
	char *b = (char*)buf;

	while (cnt < count)
	{
		ssize_t r = pread(fd, b + cnt, count - cnt, offset + cnt);
		if (0 >= r)
		{
			if (0 == r)
			{
				// EOF
				return cnt;
			}
			if (EINTR == errno)
			{
				continue;
			}
			
			return -errno;
		}

		cnt += r;
	}
	return cnt;
}

ssize_t safe_pread_exact(int fd, void* buf, size_t count, off_t offset)
{
	ssize_t ret = safe_pread(fd, buf, count, offset);
	if (0 > ret)
	{
		return ret;
	}
	
	if ((size_t)ret != count)
	{
		return -EDOM;
	}
	
	return 0;
}

ssize_t safe_pwrite(int fd, const void* buf, size_t count, off_t offset)
{
	while (0 < count)
	{
		ssize_t r = pwrite(fd, buf, count, offset);
		if (0 > r)
		{
			if (EINTR == errno)
			{
				continue;
			}
			
			return -errno;
		}
		count -= r;
		buf = (char *)buf + r;
		offset += r;
	}
	return 0;
}

#ifdef HAVE_SPLICE
ssize_t safe_splice(int fd_in, off_t* off_in, int fd_out, off_t* off_out, size_t len, unsigned int flags)
{
	size_t cnt = 0;

	while (cnt < len)
	{
	    ssize_t r = splice(fd_in, off_in, fd_out, off_out, len - cnt, flags);
	    if (0 >= r)
		{
			if (0 == r)
			{
				// EOF
				return cnt;
			}
			
			if (EINTR == errno)
			{
				continue;
			}
			
			if (EAGAIN == errno)
			{
				break;
			}
			
			return -errno;
	    }
	    cnt += r;
	}
	
	return cnt;
}

ssize_t safe_splice_exact(int fd_in, off_t* off_in, int fd_out, off_t* off_out, size_t len, unsigned int flags)
{
	ssize_t ret = safe_splice(fd_in, off_in, fd_out, off_out, len, flags);
	if (0 > ret)
	{
		return ret;
	}
	
	if ((size_t)ret != len)
	{
		return -EDOM;
	}
	
	return 0;
}
#endif

int safe_write_file(const char* base, const char* file, const char* val, size_t vallen)
{
	int ret;
	char fn[PATH_MAX];
	char tmp[PATH_MAX];
	int fd;

	char oldval[80];
	ret = safe_read_file(base, file, oldval, sizeof(oldval));
	if (ret == (int)vallen && 0 == memcmp(oldval, val, vallen))
	{
		return 0;
	}

	snprintf(fn, sizeof(fn), "%s/%s", base, file);
	snprintf(tmp, sizeof(tmp), "%s/%s.tmp", base, file);
	fd = open(tmp, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (0 > fd)
	{
		ret = errno;
		return -ret;
	}
	
	ret = safe_write(fd, val, vallen);
	if (ret)
	{
		VOID_TEMP_FAILURE_RETRY(close(fd));
		return ret;
	}

	ret = fsync(fd);
	if (0 > ret)
	{
		ret = -errno;
	}
	
	VOID_TEMP_FAILURE_RETRY(close(fd));
		
	if (0 > ret)
	{
		unlink(tmp);
		return ret;
	}
	
	ret = rename(tmp, fn);
	if (0 > ret)
	{
		ret = -errno;
		unlink(tmp);
		return ret;
	}

	fd = open(base, O_RDONLY);
	if (0 > fd)
	{
		ret = -errno;
		return ret;
	}
	
	ret = fsync(fd);
	if (ret < 0)
	{
		ret = -errno;
	}
	
	VOID_TEMP_FAILURE_RETRY(close(fd));

	return ret;
}

int safe_read_file(const char* base, const char* file, char* val, size_t vallen)
{
	char fn[PATH_MAX];
	int fd, len;

	snprintf(fn, sizeof(fn), "%s/%s", base, file);
	fd = open(fn, O_RDONLY);
	if (0 > fd)
	{
		return -errno;
	}
	
	len = safe_read(fd, val, vallen);
	if (0 > len)
	{
		VOID_TEMP_FAILURE_RETRY(close(fd));
		return len;
	}

	VOID_TEMP_FAILURE_RETRY(close(fd));

	return len;
}

