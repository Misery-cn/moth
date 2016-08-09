#include "socket.h"

SYS_NS_BEGIN


CSocket::CSocket()
{
	state_ = SOCKET_CLOSE;
    fd_ = -1;
}

CSocket::~CSocket()
{
    if (0 < fd_)
    {
        (void)s_close();
		fd_ = -1;
    }
}

int CSocket::s_open()
{
    // 创建一个套接字
    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (0 >= fd_)
    {
        RUNLOG("create CSocket failed!\n");
        return SOCKET_ERROR;
    }

    return NO_ERROR;
}

int CSocket::s_close()
{
    // 关闭连接
    shutdown(fd_, SHUT_RDWR);
    // 清理套接字
    if (0 != close(fd_))
    {
        return SOCKET_CLOSE_ERROR;
    }
	
	fd_ = -1;
	
    return NO_ERROR;
}

int CSocket::s_close(size_t fd)
{
    // 关闭连接
    shutdown(fd, SHUT_RDWR);
    // 清理套接字
    if (0 != close(fd))
    {
        return SOCKET_CLOSE_ERROR;
    }
	
	fd = -1;
	
    return NO_ERROR;
}

int CSocket::s_bind(char* ip, int port)
{
    int	op = 1;
    struct sockaddr_in addr;

    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ((NULL == ip) || (MIN_IP_LEN > strlen(ip)))
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        addr.sin_addr.s_addr = inet_addr(ip);
    }

    // 允许在bind过程中本地地址可重复使用
    // 成功返回0，失败返回-1
    if (0 > setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)))
    {
        return SOCKET_SET_ERROR;
    }

    // 给套接口分配协议地址,如果前面没有设置地址可重复使用,则要判断errno=EADDRINUSE,即地址已使用
    // 成功返回0，失败返回-1
    if (0 > bind(fd_, (struct sockaddr*)&addr, sizeof(addr)))
    {
        return SOCKET_BIND_ERROR;
    }

    return NO_ERROR;
}

int CSocket::s_listen()
{
    if (0 != listen(fd_, MAX_CONN))
    {
        return SOCKET_LISTEN_ERROR;
    }

    return NO_ERROR;
}

int CSocket::s_accept(int* fd, struct sockaddr_in* addr)
{
    int newfd = -1;
    int size = sizeof(struct sockaddr_in);

    newfd = accept(fd_, (struct sockaddr*)addr, (socklen_t*)&size);
    if (0 > newfd)
    {
        return SOCKET_ACCEPT_ERROR;
    }

    *fd = newfd;

    return NO_ERROR;
}

int CSocket::s_connect(char* ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family    = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    // CSocket连接
    if (0 != connect(fd_, (struct sockaddr*)&addr, sizeof(addr)))
    {
        RUNLOG("connect failed: %s", strerror(errno));
        return SOCKET_CONNECT_ERROR;
    }

    return NO_ERROR;
}

int CSocket::s_read(int fd, char* buff, int len)
{
    /*
	fd_set readfds;
    fd_set errfds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&readfds);
    // 打开sockfd的可读位
    FD_SET(fd, &readfds);

    // 初始化
    FD_ZERO(&errfds);
    // 设置sockfd的异常位
    FD_SET(fd, &errfds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    // 检测套接字状态
    if (select(fd + 1, &readfds, NULL, &errfds, &tv))
    {
        // 判断描述字sockfd的异常位是否打开,如果打开则表示产生错误
        if (FD_ISSET(fd, &errfds))
        {
            return SOCKET_READ_ERROR;
        }

        // 判断描述字sockfd的可读位是否打开,如果打开则读取数据
        if (FD_ISSET(fd, &readfds))
        {
            // 读数据
            // 返回实际所读的字节数
            if (0 > read(fd, buff, len))
            {
                return SOCKET_READ_ERROR;
            }
            else
            {
				return NO_ERROR;
            }
        }
    }
	*/
	
	// 读数据
	// 返回实际所读的字节数
	if (0 > read(fd, buff, len))
	{
		return SOCKET_READ_ERROR;
	}
	else
	{
		return NO_ERROR;
	}
}

int CSocket::s_write(int fd, char* buff)
{
	/*
    fd_set writefds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&writefds);
    // 打开fd的可写位
    FD_SET(fd, &writefds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    int bufLen = strlen(buff);

    // 检测套接字状态
    if (select(fd + 1, NULL, &writefds, 0, &tv))
    {
        // 判断描述字sockfd的可写位是否打开,如果打开则写入数据
        if (FD_ISSET(fd, &writefds))
        {
            // 写入数据
            // 返回所写字节数
            int wl = write(fd, buff, bufLen);
            if (0 > wl)
            {
                return SOCKET_WRITE_ERROR;
            }
            else
            {
                if (wl == bufLen)
                {
                    return NO_ERROR;
                }
                else
                {
                    return SOCKET_WRITE_ERROR;
                }
            }
        }
    }
	*/
	
	// 写入数据
	// 返回所写字节数
	int bufLen = strlen(buff);
	int wl = write(fd, buff, bufLen);
	if (0 > wl)
	{
		return SOCKET_WRITE_ERROR;
	}
	else
	{
		if (wl == bufLen)
		{
			return NO_ERROR;
		}
		else
		{
			return SOCKET_WRITE_ERROR;
		}
	}
}

SYS_NS_END
