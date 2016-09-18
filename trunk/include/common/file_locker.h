#ifndef _FILE_LOCKER_H_
#define _FILE_LOCKER_H_

#include <errno.h>
#include <stdio.h>
#include <string>
#include <sys/file.h>
#include <unistd.h>


// UTILS_NS_BEGIN

// 普通文件锁，总是锁定整个文件
// 支持独占和共享两种类型，由参数exclusive决定
// 进程退出时，它所持有的锁会被自动释放
// 注意同一个FileLocker对象，不要跨线程使用
// 同一个FileLocker对象总是只会被同一个线程调度
class FileLocker
{
public:
	// 不加锁构造函数，由调用者决定何时加锁
    explicit FileLocker(const char* filepath) throw ()
        : fd_(-1), filepath_(filepath)
    {
    }

	// 自动加锁构造函数
	// 建议使用SharedFileLocker或ExclusiveFileLocker，替代此构造函数调用
    explicit FileLocker(const char* filepath, bool exclusive) throw ()
        : fd_(-1), filepath_(filepath)
    {
        lock(exclusive);
    }

    ~FileLocker() throw ()
    {
        unlock();
    }

	// 是否独占加锁
    bool lock(bool exclusive) throw ()
    {
        // 独占还是共享
        int operation = exclusive ? LOCK_EX : LOCK_SH;

        fd_ = open(filepath_.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
        if (-1 != fd_)
        {
            if (-1 == flock(fd_, operation))
            {
                int errcode = errno;
                close(fd_);

                fd_ = -1;
				// 恢复，目的是让上层调用者可以使用errno
                errno = errcode;
            }
        }

        return (-1 != fd_);
    }

    bool unlock() throw ()
    {
        bool ret = false;
    
        if (is_locked())
        {
            if (0 == flock(fd_, LOCK_UN))
            {
                ret = true;
            }

            close(fd_);
        }
        
        return ret;
    }

    bool is_locked() const throw ()
    {
        return (-1 != fd_);
    }

    operator bool () const
    {
        return (-1 != fd_);
    }

	// 获取锁文件路径
    const std::string& get_filepath() const throw ()
    {
        return filepath_;
    }

private:
    int fd_;
    std::string filepath_;
};


// 独占文件锁
class ExclusiveFileLocker : public FileLocker
{
public:
    explicit ExclusiveFileLocker(const char* filepath)
        : FileLocker(filepath, true)
    {
    }

private:
    bool lock();
    bool unlock();
};


// 共享文件锁
class SharedFileLocker : public FileLocker
{
public:
    explicit SharedFileLocker(const char* filepath)
        : FileLocker(filepath, false)
    {
    }

private:
    bool lock();
    bool unlock();
};


// 增强型文件锁，可只锁文件指定的部分,一个ExFileLocker对象，总是只被一个线程调用
class ExFileLocker
{
public:
	// 构造增强型文件锁对象
	// 调用者需要负责文件的打开和关闭
	// fd必须是已打开的文件描述符
	// 且必须以O_WRONLY或O_RDWR方式打开，否则遇到EBADF错误
	// 但如果调用进程具有PRIV_LOCKRDONLY权限的组的成员，则可使用O_RDONLY打开fd
    explicit ExFileLocker(int fd)
        : fd_(fd)
    {
    }

	// 如果抢到锁，则立即返回true，否则被阻塞
	// 如果加锁失败，则返回false
	// size 如果为0，表示锁定从文件当前偏移开始到文件尾的区域
	// 如果为正，则表示锁定从文件当前偏移开始，往后大小为size的连续区域
	// 如果为负，则表示锁定从文件当前偏移开始，往前大小为size的连续区域
    bool lock(off_t size)
    {
        if (-1 != fd_)
        {
            return 0 == lockf(fd_, F_LOCK, size);
        }

        return false;
    }

	// 尝试加锁，总是立即返回，不会被阻塞
	// 如果加锁成功，则立即返回true，否则立即返回false
	// size 如果为0，表示尝试锁定从文件当前偏移开始到文件尾的区域
	// 如果为正，则表示尝试锁定从文件当前偏移开始，往后大小为size的连续区域
	// 如果为负，则表示尝试锁定从文件当前偏移开始，往前大小为size的连续区域
    bool try_lock(off_t size)
    {
        if (-1 != fd_)
        {
            return 0 == lockf(fd_, F_TLOCK, size);
        }

        return false;
    }

	// 检测在指定的区域中是否存在其他进程的锁定
	// 如果该区域可访问，则返回true，否则返回false
	// size 如果为0，表示检测从文件当前偏移开始到文件尾的区域的锁定状态
	// 如果为正，则表示检测从文件当前偏移开始，往后大小为size的连续区域的锁定状态
	// 如果为负，则表示检测从文件当前偏移开始，往前大小为size的连续区域的锁定状态
    bool test_lock(off_t size)
    {
        if (-1 != fd_)
        {
            return 0 == lockf(fd_, F_TEST, 0);
        }

        return false;
    }

	// 如果解锁成功，则立即返回true，否则立即返回false
	// size 如果为0，表示解锁从文件当前偏移开始到文件尾的区域
	// 如果为正，则表示解锁从文件当前偏移开始，往后大小为size的连续区域
	// 如果为负，则表示解锁从文件当前偏移开始，往前大小为size的连续区域
    bool unlock(off_t size)
    {
        if (-1 != fd_)
        {
            return 0 == lockf(fd_, F_ULOCK, size);
        }

        return false;
    }

private:
    int fd_;
};

// UTILS_NS_END

#endif
