#ifndef _ACCEPTER_H_
#define _ACCEPTER_H_

#include <set>
#include "thread.h"
#include "msg_types.h"

class SimpleMessenger;

class Accepter : public Thread
{
private:
    SimpleMessenger* _msgr;
    // 是否停止
    bool _done;
    // 本地监听的文件描述符
    int _listen_fd;

    // 管道的读\写描述符
    int _shutdown_rd_fd;
    int _shutdown_wr_fd;
    
    int create_socket(int* rd, int* wr);

public:
    Accepter(SimpleMessenger* r) : _msgr(r), _done(false), _listen_fd(-1),
                _shutdown_rd_fd(-1), _shutdown_wr_fd(-1)
    {}
    
    /**
     * 线程入口
     *
     */
    void entry();
                
    /**
     * 停止线程
     *
     */
    void stop();

    /**
     * 绑定并开始监听
     *
     */
    int bind(const entity_addr_t& bind_addr, const std::set<int>& avoid_ports);
 
    /**
     * 重新绑定
     *
     */
    int rebind(const std::set<int>& avoid_port);

    /**
     * 启动线程
     *
     */
    int start();
};

#endif
