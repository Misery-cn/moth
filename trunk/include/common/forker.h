#ifndef _SYS_FORKER_H_
#define _SYS_FORKER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include "error.h"
#include "safe_io.h"


class Forker
{
public:
	Forker() : _child_pid(0), _forked(false)
	{}

	int fork(std::string& err)
	{
		int r = socketpair(AF_UNIX, SOCK_STREAM, 0, _fd);
		std::ostringstream oss;
		if (0 > r)
		{
			oss << "[" << getpid() << "]: unable to create socketpair: " << Error::to_string(errno);
			err = oss.str();
			return r;
		}

		_forked = true;

		_child_pid = ::fork();
		if (0 > _child_pid)
		{
			r = -errno;
			oss << "[" << getpid() << "]: unable to fork: " << Error::to_string(errno);
			err = oss.str();
			return r;
		}

		// 如果是父进程关闭fd[1]套接字
		// 如果是子进程则关闭fd[0]套接字
		// 父进程通过fd[0]读写数据,子进程通过fd[1]读写数据
		if (0 == _child_pid)
		{
			::close(_fd[0]);
		}
		else
		{
			::close(_fd[1]);
		}
		
		return 0;
	}

	bool is_child()
	{
		return 0 == _child_pid;
	}

	bool is_parent()
	{
		return 0 != _child_pid;
	}

	int parent_wait(std::string& err_msg)
	{
		int r = -1;
		std::ostringstream oss;
		int err = safe_read_exact(_fd[0], &r, sizeof(r));
		if (0 == err && -1 == r)
		{
			// 父进程退出同时关闭标准输入、输出、错误
			::close(0);
			::close(1);
			::close(2);
			r = 0;
		}
		else if (err)
		{
			oss << "[" << getpid() << "]: " << Error::to_string(err);
		}
		else
		{
			int status;
			err = waitpid(_child_pid, &status, 0);
			if (err < 0)
			{
				oss << "[" << getpid() << "]" << " waitpid error: " << Error::to_string(err);
			}
			else if (WIFSIGNALED(status))
			{
				oss << "[" << getpid() << "]" << " exited with a signal";
			}
			else if (!WIFEXITED(status))
			{
				oss << "[" << getpid() << "]" << " did not exit normally";
			}
			else
			{
				err = WEXITSTATUS(status);
				if (err != 0)
				{
					oss << "[" << getpid() << "]" << " returned exit_status " << Error::to_string(err);
				}
			}
		}
		
		err_msg = oss.str();
		return err;
	}

	int signal_exit(int r)
	{
		if (_forked)
		{
			int ret = safe_write(_fd[1], &r, sizeof(r));
			if (0 >= ret)
			{
				return ret;
			}
		}
		
		return r;
	}
	
	void exit(int r)
	{
		signal_exit(r);
		::exit(r);
	}

	void daemonize()
	{
		static int r = -1;
		::write(_fd[1], &r, sizeof(r));
	}

private:
	// 子进程ID
	pid_t _child_pid;
	bool _forked;
	// 父子进程用来通信的文件描述符
	int _fd[2];
};

#endif
