#ifndef _SOCKET_CONNECTION_H_
#define _SOCKET_CONNECTION_H_

#include "connection.h"

class Socket;

class SocketConnection : public Connection
{
private:
    Socket* _socket;
    friend class Socket;

public:
    SocketConnection(Messenger* m) : Connection(m), _socket(NULL)
    {}

    virtual ~SocketConnection();

    /**
     * 获取socket
     *
     */
    Socket* get_socket();

    /**
     * 尝试获取socket，有可能获取失败
     *
     */
    bool try_get_socket(Socket** s);

    /**
     * 关闭socket连接
     *
     */
    bool clear_socket(Socket* s);

    /**
     * 重置当前连接
     *
     */
    void reset_socket(Socket* s);

    /**
     * 判断连接是否正常
     *
     */
    bool is_connected();

    /**
     * 发送消息
     *
     */
    int send_message(Message* m);

    /**
     * 发送心跳
     *
     */
    void send_keepalive();

    /**
     * 关闭连接
     *
     */
    void mark_down();
    
    void mark_disposable();

};

#endif