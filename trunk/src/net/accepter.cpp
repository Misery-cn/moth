#include <poll.h>
#include "accepter.h"
#include "socket.h"
#include "simple_messenger.h"

// 设置文件描述符旗标防止异常关闭
static int set_close_on_exec(int fd)
{
    int flags = fcntl(fd, F_GETFD, 0);
    if (flags < 0)
    {
        return errno;
    }
    
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC))
    {
        return errno;
    }
    
    return 0;
}


int Accepter::create_socket(int* rd, int* wr)
{
    int selfpipe[2];
    int ret = ::pipe2(selfpipe, (O_CLOEXEC|O_NONBLOCK));
    if (0 > ret)
    {
        return -errno;
    }
    
    // pipefd[0] 为读端
    // pipefd[1] 为写端
    *rd = selfpipe[0];
    *wr = selfpipe[1];
    
    return 0;
}

int Accepter::bind(const entity_addr_t& bind_addr, const std::set<int>& avoid_ports)
{
    int family;
    switch (bind_addr.get_family())
    {
        case AF_INET:
        case AF_INET6:
        {
            family = bind_addr.get_family();
            break;
        }
        default:
        {
            family = AF_INET;
            // family = AF_INET6;
        }
    }

    // 创建一个socket用于监听
    _listen_fd = ::socket(family, SOCK_STREAM, 0);
    
    if (0 > _listen_fd)
    {
        return -errno;
    }

    if (set_close_on_exec(_listen_fd))
    {
        ERROR_LOG("set close on exec flag failed!");
    }
  
    entity_addr_t listen_addr = bind_addr;
    listen_addr.set_family(family);

    int rc = -1;
    int r = -1;

    // 尝试绑定3次
    for (int i = 0; i < 3; ++i)
    {
        if (0 < i)
        {
            sleep(5);
        }

        if (listen_addr.get_port())
        {
            int on = 1;
            rc = ::setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
            if (0 > rc)
            {
                r = -errno;
                continue;
            }

            rc = ::bind(_listen_fd, listen_addr.get_sockaddr(), listen_addr.get_sockaddr_len());
            if (0 > rc)
            {
                r = -errno;
                continue;
            }
        }
        // 如果没有配置监听端口
        else
        {
            for (int port = 9000; port <= 9999; port++)
            {
                if (avoid_ports.count(port))
                {
                    continue;
                }

                listen_addr.set_port(port);
                rc = ::bind(_listen_fd, listen_addr.get_sockaddr(),
                listen_addr.get_sockaddr_len());
                if (0 == rc)
                {
                    break;
                }
            }
            
            if (0 > rc)
            {
                r = -errno;
                listen_addr.set_port(0); 
                continue;
            }
        }

        if (0 == rc)
        {
            break;
        }
    }

    // 绑定端口失败
    if (0 > rc)
    {
        ::close(_listen_fd);
        _listen_fd = -1;
        return r;
    }
    
    sockaddr_storage ss;
    socklen_t llen = sizeof(ss);
    rc = getsockname(_listen_fd, (sockaddr*)&ss, &llen);
    if (0 > rc)
    {
        rc = -errno;
        ::close(_listen_fd);
        _listen_fd = -1;
        return rc;
    }
    
    listen_addr.set_sockaddr((sockaddr*)&ss);

    // 设置接收缓冲大小
    int size = 4096;
    rc = ::setsockopt(_listen_fd, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size));
    if (0 > rc)
    {
        rc = -errno;
        ::close(_listen_fd);
        _listen_fd = -1;
        return rc;
    }
    
    // 设置发送缓冲大小
    rc = ::setsockopt(_listen_fd, SOL_SOCKET, SO_SNDBUF , (void*)&size, sizeof(size));
    if (0 > rc)
    {
        rc = -errno;
        ::close(_listen_fd);
        _listen_fd = -1;
        return rc;
    }

    // 开始监听
    rc = ::listen(_listen_fd, 128);
    if (0 > rc)
    {
        rc = -errno;
        ::close(_listen_fd);
        _listen_fd = -1;
        return rc;
    }
  
    _msgr->set_entity_addr(listen_addr);
    _msgr->init_local_connection();

    rc = create_socket(&_shutdown_rd_fd, &_shutdown_wr_fd);
    if (0 > rc)
    {
        return rc;
    }

    return 0;
}

int Accepter::rebind(const std::set<int>& avoid_ports)
{
    entity_addr_t addr = _msgr->get_entity_addr();
    std::set<int> new_avoid = avoid_ports;
    // 不继续绑定之前的端口
    new_avoid.insert(addr.get_port());
    addr.set_port(0);

    int r = bind(addr, new_avoid);
    if (0 == r)
    {
        start();
    }
    
    return r;
}

int Accepter::start()
{
    DEBUG_LOG("Accepter start");

    create();

    return 0;
}

void Accepter::entry()
{
    DEBUG_LOG("Accepter entry");
    
    int errors = 0;
    int ch;

    struct pollfd pfd[2];
    memset(pfd, 0, sizeof(pfd));
    
    // POLLIN 普通或优先级带数据可读
    // POLLRDNORM 普通数据可读
    // POLLRDBAND 优先级带数据可读
    // POLLPRI 高优先级数据可读
    // POLLOUT 普通数据可写
    // POLLWRNORM 普通数据可写
    // POLLWRBAND 优先级带数据可写
    // POLLERR 发生错误
    // POLLHUP 对方文件描述符挂起
    // POLLNVAL 文件描述符不是一个打开的文件

    pfd[0].fd = _listen_fd;
    pfd[0].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
    pfd[1].fd = _shutdown_rd_fd;
    pfd[1].events = POLLIN | POLLERR | POLLNVAL | POLLHUP;
    
    while (!_done)
    {
        int r = poll(pfd, 2, -1);
        if (0 > r)
        {
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }
        
        // 发生错误
        if (pfd[0].revents & (POLLERR | POLLNVAL | POLLHUP))
        {
            break;
        }
        
        // 退出accepter
        if (pfd[1].revents & (POLLIN | POLLERR | POLLNVAL | POLLHUP))
        {
            if (-1 == ::read(_shutdown_rd_fd, &ch, 1))
            {
                if (errno != EAGAIN)
                {
                    
                }
            }
            
            break;
        }
        
        // 结束
        if (_done)
        {
            break;
        }

        sockaddr_storage ss;
        socklen_t len = sizeof(ss);
        int fd = ::accept(_listen_fd, (sockaddr*)&ss, &len);
        if (0 <= fd)
        {
            sockaddr_in* addr_in = (sockaddr_in*)&ss;
            INFO_LOG("accept %s:%d connect", inet_ntoa(addr_in->sin_addr), ntohs(addr_in->sin_port));

            // 防止异常关闭
            if (set_close_on_exec(fd))
            {
                ERROR_LOG("set close on exec flag failed while accept");
            }
            
            errors = 0;

            // 添加连接到msgr,此时socket状态为accepting
            _msgr->add_accept_socket(fd);
        }
        else
        {
            if (++errors > 4)
            {
                break;
            }
        }
    }

    if (0 <= _shutdown_rd_fd)
    {
        ::close(_shutdown_rd_fd);
        _shutdown_rd_fd = -1;
    }
}

void Accepter::stop()
{
    _done = true;

    if (0 > _shutdown_wr_fd)
    {
        return;
    }

    char buf[1] = {0};
    int ret = safe_write(_shutdown_wr_fd, buf, 1);
    if (0 > ret)
    {
        // 
    }
    
    VOID_TEMP_FAILURE_RETRY(::close(_shutdown_wr_fd));
    _shutdown_wr_fd = -1;

    if (is_started())
    {
        join();
    }

    if (0 <= _listen_fd)
    {
        if (0 > ::close(_listen_fd))
        {
            // 
        }
        _listen_fd = -1;
    }
    
    if (0 <= _shutdown_rd_fd)
    {
        if (0 > ::close(_shutdown_rd_fd))
        {
            // 
        }
        _shutdown_rd_fd = -1;
    }
    
    _done = false;
}


