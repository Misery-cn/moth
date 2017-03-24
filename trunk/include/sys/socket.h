
#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "utils.h"

// SYS_NS_BEGIN

class CSocket
{
public:
    CSocket();
    virtual ~CSocket();
	
	enum SOCKET_STATE
	{
		SOCKET_CLOSE = 0,
		SOCKET_OPEN = 1
	};

    // 创建套接字
    int s_open() throw (CSysCallException);
    // 关闭套接字
    int s_close() throw (CSysCallException);
	// 关闭调用open返回的套接字
	int s_close(size_t fd) throw (CSysCallException);
    // 监听端口,MaxConn为同时处理的最大连接数
    int s_listen() throw (CSysCallException);
    // 绑定端口
    int s_bind(IN char* ip, IN int port) throw (CSysCallException);
    // 请求建立连接
    int s_connect(IN char* ip, IN int port) throw (CSysCallException);
    // 接收连接
    int s_accept(OUT int* fd, OUT struct sockaddr_in* addr) throw (CSysCallException);
    // 读取数据
    int s_read(IN int fd, OUT char* buff, OUT int len) throw (CSysCallException);
    // 写入数据
    int s_write(IN int fd, IN char* buff) throw (CSysCallException);
	
	inline size_t getfd() {return _fd;}
private:
    size_t _fd;
	SOCKET_STATE _state;
};

// SYS_NS_END

#endif
