#include <netinet/tcp.h>
#include <poll.h>
#include "socket.h"
#include "msg_types.h"
#include "crc32.h"
#include "simple_messenger.h"
#include "error.h"
// SYS_NS_BEGIN

#define SEQ_MASK  0x7fffffff
#define BANNER "banner"

Socket::Socket(SimpleMessenger* msgr, int st, SocketConnection* con)
        : RefCountable(), _reader_thread(this), _writer_thread(this), _delay_thread(NULL), _msgr(msgr),
        _conn_id(msgr->_dispatch_queue.get_id()), _recv_ofs(0), _recv_len(0), _fd(-1),
        _port(0), _peer_type(-1), _lock(), _state(st), _connection_state(NULL), 
        _reader_running(false), _reader_needs_join(false), _reader_dispatching(false),
        _notify_on_dispatch_done(false), _writer_running(false), _in_q(&(msgr->_dispatch_queue)),
        _send_keepalive(false), _send_keepalive_ack(false), _connect_seq(0), _peer_global_seq(0),
        _out_seq(0), _in_seq(0), _in_seq_acked(0)
{
    if (con)
    {
        _connection_state = con;
        _connection_state->reset_socket(this);
    }
    else
    {
        _connection_state = new SocketConnection(msgr);
        _connection_state->_socket = get();
    } 

    // ms
    _msgr->_timeout = SOCKET_TIMEOUT * 1000;

    // 预取缓冲
    _recv_max_prefetch = IO_BUFFER_MAX;
    _recv_buf = new char[_recv_max_prefetch];
}

Socket::~Socket()
{
    DELETE_P(_delay_thread);
    DELETE_ARRAY(_recv_buf);
}

void Socket::handle_ack(uint64_t seq)
{
    while (!_sent.empty() && _sent.front()->get_seq() <= seq)
    {
        Message* m = _sent.front();
        _sent.pop_front();
        m->dec();
    }
}

void Socket::start_reader()
{
    if (_reader_needs_join)
    {
        _reader_thread.join();
        _reader_needs_join = false;
    }
    
    _reader_running = true;
    _reader_thread.create();
}

void Socket::start_delay_thread()
{
    if (!_delay_thread)
    {
        uint32_t pos = std::string("").find(entity_type_name(_connection_state->_peer_type));
        if (pos != std::string::npos)
        {
            _delay_thread = new DelayedDelivery(this);
            _delay_thread->create();
        }
    }
}

void Socket::start_writer()
{
    _writer_running = true;
    _writer_thread.create();
}

void Socket::join_reader()
{
    if (!_reader_running)
    {
        return;
    }
    
    _cond.signal();

    _lock.unlock();
    _reader_thread.join();
    _lock.lock();
    _reader_needs_join = false;
}

void Socket::set_socket_options()
{
    // 禁用Nagle’s Algorithm
    int flag = 1;
    int r = ::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
    if (0 > r)
    {
        r = -errno;
    }

    // 设置滑动窗口大小
    /*
    int size = 0;
    int r = ::setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size));
    if (0 > r)
    {
        r = -errno;
    }
    */


#if defined(SO_NOSIGPIPE)
    int val = 1;
    int r = ::setsockopt(_fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&val, sizeof(val));
    if (r)
    {
        r = -errno;
    }
#endif

#ifdef SO_PRIORITY
    int prio = _msgr->get_socket_priority();
    if (0 <= prio)
    {
        int r = -1;
    #ifdef IPTOS_CLASS_CS6
        int iptos = IPTOS_CLASS_CS6;
        int addr_family = 0;
        if (!_peer_addr.is_blank_ip())
        {
            addr_family = _peer_addr.get_family();
        }
        else
        {
            addr_family = _msgr->get_entity_addr().get_family();
        }
        
        switch (addr_family)
        {
            case AF_INET:
            {
                r = ::setsockopt(sd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
                break;
            }
            case AF_INET6:
            {
                r = ::setsockopt(sd, IPPROTO_IPV6, IPV6_TCLASS, &iptos, sizeof(iptos));
                break;
            }
            default:
                return;
        }
        
        if (0 > r)
        {
            r = -errno;
        }
    #endif
        r = ::setsockopt(_fd, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio));
        if (0 > r)
        {
            r = -errno;
        }
    }
#endif
}

int Socket::create_socket()
{
    // 已经打开了一个socket
    if (0 <= _fd)
    {
        close_socket();
    }

    _fd = ::socket(_peer_addr.get_family(), SOCK_STREAM, 0);
    if (0 > _fd)
    {
        return -1;
    }

    return 0;
}


int Socket::accepting()
{
    int rc = 0;
    char banner[strlen(BANNER) + 1] = {0};
    bool is_reset_from_peer = false;
    // 保存本地地址和对端地址信息的buf
    buffer addrs_buf;
    socklen_t len;
    // 对端地址
    entity_addr_t peer_addr;
    // 保存对端地址信息的buf
    buffer peer_addr_buf;
    msg_connect connect;
    msg_connect_reply connect_reply;
    Socket* existing = NULL;
    uint64_t existing_seq = -1;
    uint64_t newly_acked_seq = 0;

    recv_reset();
    
    set_socket_options();

    if (0 > tcp_write(BANNER, strlen(BANNER)))
    {
        accept_fail();
        return -1;
    }

    if (0 > tcp_read(banner, strlen(BANNER)))
    {
        accept_fail();
        return -1;
    }
    
    if (memcmp(banner, BANNER, strlen(BANNER)))
    {
        // _lock.lock();
        _state = SOCKET_CLOSED;
        atomic_set(&_state_closed, 1);
        fault();
        return -1;
    }

    // 先将自己的地址信息保存到buffer中
    ::encode(_msgr->_entity._addr, addrs_buf);
    
    _port = _msgr->_entity._addr.get_port();

    sockaddr_storage ss;
    len = sizeof(ss);
    // 获取对端地址
    rc = ::getpeername(_fd, (sockaddr*)&ss, &len);
    if (0 > rc)
    {    
        // _lock.lock();
        accept_fail();
        return -1;
    }
    
    peer_addr.set_sockaddr((sockaddr*)&ss);
    
    // 将对端地址保存到buffer中
    ::encode(peer_addr, addrs_buf);

    rc = tcp_write(addrs_buf.c_str(), addrs_buf.length());
    if (0 > rc)
    {
        // _lock.lock();
        accept_fail();
        return -1;
    }

    {
        ptr p(sizeof(entity_addr_t));
        peer_addr_buf.push_back(std::move(p));
    }

    if (0 > tcp_read(peer_addr_buf.c_str(), peer_addr_buf.length()))
    {
        // _lock.lock();
        accept_fail();
        return -1;
    }

    {
        buffer::iterator it = peer_addr_buf.begin();
        ::decode(_peer_addr, it);
    }

    if (_peer_addr.is_blank_ip())
    {
        int port = _peer_addr.get_port();
        _peer_addr._addr = peer_addr._addr;
        _peer_addr.set_port(port);
    }
    
    set_peer_addr(_peer_addr);
    
    if (0 > tcp_read((char*)&connect, sizeof(connect)))
    {
        // _lock.lock();
        accept_fail();
        return -1;
    }

    // _msgr->_lock.lock();
    Mutex::Locker locker(_msgr->_lock);
    // _lock.lock();
    if (_msgr->_dispatch_queue._stop)
    {
        // _msgr->_lock.unlock();
        set_state_close();
        fault();
        return -1;
    }
    
    if (SOCKET_ACCEPTING != _state)
    {
        // _msgr->_lock.unlock();
        set_state_close();
        fault();
        return -1;
    }

    set_peer_type(connect.host_type);
    _policy = _msgr->get_policy(connect.host_type);

    memset(&connect_reply, 0, sizeof(connect_reply));
    connect_reply.protocol_version = 0;
    
    // _msgr->_lock.unlock();

    if (connect.protocol_version != connect_reply.protocol_version)
    {
        connect_reply.tag = MSGR_TAG_BADPROTOVER;
        // _lock.unlock();
        rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
        if (0 > rc)
        {
            // _lock.lock();
            accept_fail();
            return -1;
        }
    }
    
    // _lock.unlock();
    
    // 是否已发起过连接
    existing = _msgr->lookup_socket(_peer_addr);

    if (existing)
    {
        // existing->_lock.lock();
        Mutex::Locker locker(existing->_lock);

        // 读线程已经启动了
        if (existing->_reader_dispatching)
        {
            existing->get();
            // _lock.unlock();
            // _msgr->_lock.unlock();
            existing->_notify_on_dispatch_done = true;
            
            while (existing->_reader_dispatching)
            {
                existing->_cond.wait(existing->_lock);
            }
            
            // existing->_lock.unlock();
            existing->dec();
        }

        /*
        if (connect.global_seq < existing->_peer_global_seq)
        {            
            connect_reply.tag = MSGR_TAG_RETRY_GLOBAL;
            connect_reply.global_seq = existing->_peer_global_seq;
            existing->_lock.unlock();
            _msgr->_lock.unlock();
            
            // _lock.unlock();
            rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
            if (0 > rc)
            {
                // _lock.lock();
                accept_fail();
                return -1;
            }
        }
        */

        if (existing->_policy._lossy)
        {        
            existing->was_session_reset();
            // 替换该socket
            replace_socket(existing);
            
            return open_socket(connect);
        }

        if (0 == connect.connect_seq &&  0 < existing->_connect_seq)
        {        
            is_reset_from_peer = true;
            
            if (_policy._resetcheck)
            {
                existing->was_session_reset();
            }
            
            // 替换该socket
            replace_socket(existing);
            
            return open_socket(connect);
        }

        if (connect.connect_seq < existing->_connect_seq)
        {
            DEBUG_LOG("connect.connect_seq < existing->_connect_seq");
            
            connect_reply.tag = MSGR_TAG_RETRY_SESSION;
            connect_reply.connect_seq = existing->_connect_seq + 1;
            // existing->_lock.unlock();
            // _msgr->_lock.unlock();

            // _lock.unlock();
            rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
            if (0 > rc)
            {
                // _lock.lock();
                accept_fail();                
                return -1;
            }
        }

        if (connect.connect_seq == existing->_connect_seq)
        {
            DEBUG_LOG("connect.connect_seq == existing->_connect_seq");
            
            if (SOCKET_OPEN == existing->_state || SOCKET_STANDBY == existing->_state)
            {
                DEBUG_LOG("existing->_state == SOCKET_OPEN || existing->_state == SOCKET_STANDBY");
                
                connect_reply.tag = MSGR_TAG_RETRY_SESSION;
                connect_reply.connect_seq = existing->_connect_seq + 1;
                // existing->_lock.unlock();
                // _msgr->_lock.unlock();

                // _lock.unlock();
                rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
                if (0 > rc)
                {
                    // _lock.lock();
                    accept_fail();
                    return -1;
                }
            }

            if (_peer_addr < _msgr->_entity._addr || existing->_policy._server)
            {
                DEBUG_LOG("_peer_addr < _msgr->_entity._addr || existing->_policy._server");
                
                // 替换该socket
                replace_socket(existing);
            
                return open_socket(connect);
            }
            else
            {
                DEBUG_LOG("_peer_addr > _msgr->_entity._addr && !existing->_policy._server");
                
                existing->send_keepalive();
                connect_reply.tag = MSGR_TAG_WAIT;
                // existing->_lock.unlock();
                // _msgr->_lock.unlock();
                // _lock.unlock();
                rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
                if (0 > rc)
                {
                    // _lock.lock();
                    accept_fail();                    
                    return -1;
                }
            }

            if (_policy._resetcheck && 0 == existing->_connect_seq)
            {
                DEBUG_LOG("_policy._resetcheck && 0 == existing->_connect_seq");
                
                connect_reply.tag = MSGR_TAG_RESETSESSION;
                // _msgr->_lock.unlock();
                // existing->_lock.unlock();

                // _lock.unlock();
                rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
                if (0 > rc)
                {
                    // _lock.lock();
                    accept_fail();                    
                    return -1;
                }
            }
        }

        // 替换该socket
        replace_socket(existing);
    
        return open_socket(connect);
    }
    else if (0 < connect.connect_seq)
    {
        connect_reply.tag = MSGR_TAG_RESETSESSION;
        
        // _lock.unlock();
        rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
        if (0 > rc)
        {
            // _lock.lock();
            accept_fail();        
            return -1;
        }
    }
    else
    {
        return open_socket(connect);
    }
}

void Socket::replace_socket(Socket* other)
{
    DEBUG_LOG("replace socket");
    
    _replaced = true;
    other->stop();
    other->unregister_socket();

    if (other->_policy._lossy)
    {
        if (other->_connection_state->clear_socket(other))
        {
            _msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(other->_connection_state->get()));
        }
    }
    else
    {
        _msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
        _connection_state = other->_connection_state;

        _connection_state->reset_socket(this);

        if (other->_delay_thread)
        {
            other->_delay_thread->steal_for_socket(this);
            _delay_thread = other->_delay_thread;
            other->_delay_thread = NULL;
            _delay_thread->flush();
        }

        uint64_t replaced_conn_id = _conn_id;
        _conn_id = other->_conn_id;
        other->_conn_id = replaced_conn_id;

        _in_seq = _is_reset_from_peer ? 0 : other->_in_seq;
        _in_seq_acked = _in_seq;

        other->requeue_sent();
        _out_seq = other->_out_seq;

        for (std::map<int, std::list<Message*> >::iterator iter = other->_out_q.begin(); iter != other->_out_q.end(); ++iter)
        {
            _out_q[iter->first].splice(_out_q[iter->first].begin(), iter->second);
        }
    }
    
    other->stop_and_wait();
}

int Socket::open_socket(const msg_connect& connect)
{
    DEBUG_LOG("open socket");
    
    int rc = 0;
    msg_connect_reply connect_reply;

    _connect_seq = connect.connect_seq + 1;
    _peer_global_seq = connect.global_seq;
    _state = SOCKET_OPEN;

    connect_reply.tag = MSGR_TAG_READY;
    connect_reply.global_seq = _msgr->get_global_seq();
    connect_reply.connect_seq = _connect_seq;
    connect_reply.flags = 0;
    if (_policy._lossy)
    {
        connect_reply.flags = connect_reply.flags | MSG_CONNECT_LOSSY;
    }

    _msgr->_dispatch_queue.queue_accept(static_cast<Connection*>(_connection_state->get()));
    _msgr->ms_deliver_handle_fast_accept(static_cast<Connection*>(_connection_state->get()));

    if (_msgr->_dispatch_queue._stop)
    {
        set_state_close();
        fault();
        return -1;
    }
    
    _msgr->_accepting_sockets.erase(this);
    register_socket();

    rc = tcp_write((char*)&connect_reply, sizeof(connect_reply));
    if (0 > rc)
    {
        accept_fail();
        return -1;
    }

    // _lock.lock();
    discard_requeued_up_to(0);
    
    if (SOCKET_CLOSED != _state)
    {
        start_writer();
    }

    start_delay_thread();

    return 0;
}


int Socket::connecting()
{
    DEBUG_LOG("socket connecting");

    uint32_t global_seq = _msgr->get_global_seq();
    DEBUG_LOG("connect seq is %d, global seq is %d", _connect_seq, global_seq);

    join_reader();

    // _lock.unlock();

    int rc = 0;
    char banner[strlen(BANNER) + 1] = {0};
    buffer addrs_buf;
    // 对端地址信息
    entity_addr_t peer_addr;
    // 对端发送给的我的地址信息
    entity_addr_t peer_addr_for_me;
    buffer my_addr_buf;

    // 打开一个socket
    if (create_socket())
    {
        connect_fail();
        return -1;
    }

    recv_reset();

    set_socket_options();

    rc = ::connect(_fd, (sockaddr*)&_peer_addr._addr, _peer_addr.addr_size());
    if (0 > rc)
    {
        ERROR_LOG("connect %s faild", inet_ntoa(((sockaddr_in*)&_peer_addr._addr)->sin_addr));
        
        if (ECONNREFUSED == Error::code())
        {
            _msgr->_dispatch_queue.queue_refused(static_cast<Connection*>(_connection_state->get()));
        }
        
        // _lock.lock();

        connect_fail();

        return -1;
    }

    rc = tcp_read((char*)&banner, strlen(BANNER));
    if (0 > rc)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }
    
    if (memcmp(banner, BANNER, strlen(BANNER)))
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    DEBUG_LOG("connect read banner successful");

    rc = tcp_write(BANNER, strlen(BANNER));
    if (0 > rc)
    {
        ERROR_LOG("socket connect send banner failed");
        // _lock.lock();
        connect_fail();
        return -1;
    }

    {
    #if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
        ptr p(sizeof(entity_addr_t) * 2);
    #else
        // addr(sockaddr_storage) + type(uint32_t)
        int len = sizeof(uint32_t) + sizeof(sock_addr_storage);
        ptr p(len * 2);
    #endif
        addrs_buf.push_back(std::move(p));
    }

    rc = tcp_read(addrs_buf.c_str(), addrs_buf.length());
    if (0 > rc)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    try
    {
        buffer::iterator p = addrs_buf.begin();
        // accept的时候先编的是对端的地址信息
        // 先解出对端地址信息
        ::decode(peer_addr, p);
        // 解出自己的地址信息
        ::decode(peer_addr_for_me, p);
    }
    catch (...)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    // 自己的端口号
    _port = peer_addr_for_me.get_port();

    // 判断是否和发起connetct的地址一样
    if (_peer_addr != peer_addr)
    {    
        if (peer_addr.is_blank_ip() && _peer_addr.get_port() == peer_addr.get_port())
        {
            ERROR_LOG("same node");
        }
        else
        {
            ERROR_LOG("not same node");
            // _lock.lock();
            connect_fail();
            return -1;
        }
      }

    // 发送自己的地址信息给对端
    ::encode(_msgr->_entity._addr, my_addr_buf);

    rc = tcp_write(my_addr_buf.c_str(), my_addr_buf.length());
    if (0 > rc)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    msg_connect connect;
    connect.host_type = _msgr->get_entity()._name.type();
    connect.global_seq = global_seq;
    connect.connect_seq = _connect_seq;
    connect.protocol_version = 0;
    connect.flags = 0;

    if (_policy._lossy)
    {
        connect.flags |= MSG_CONNECT_LOSSY;
    }

    rc = tcp_write((char*)&connect, sizeof(connect));
    if (0 > rc)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    msg_connect_reply connect_reply;
    rc = tcp_read((char*)&connect_reply, sizeof(connect_reply));
    if (0 > rc)
    {
        // _lock.lock();
        connect_fail();
        return -1;
    }

    if (MSGR_TAG_BADPROTOVER == connect_reply.tag)
    {
        connect_fail();
        return -1;
    }

    if (MSGR_TAG_RESETSESSION == connect_reply.tag)
    {
        was_session_reset();
        _connect_seq = 0;
        // _lock.unlock();
        connect_fail();
        return -1;
    }

    if (MSGR_TAG_RETRY_GLOBAL == connect_reply.tag)
    {
        global_seq = _msgr->get_global_seq(connect_reply.global_seq);
        // _lock.unlock();
        connect_fail();
        return -1;
    }

    if (MSGR_TAG_RETRY_SESSION == connect_reply.tag)
    {
        _connect_seq = connect_reply.connect_seq;
        // _lock.unlock();
        connect_fail();
        return -1;
    }

    if (MSGR_TAG_WAIT == connect_reply.tag)
    {
        _state = SOCKET_WAIT;
        return -1;
    }

    if (MSGR_TAG_READY == connect_reply.tag)
    {
        _peer_global_seq = connect_reply.global_seq;
        _policy._lossy = connect_reply.flags & MSG_CONNECT_LOSSY;
        _state = SOCKET_OPEN;
        _connect_seq = _connect_seq + 1;
        
        _backoff = utime_t();
        _msgr->_dispatch_queue.queue_connect(static_cast<Connection*>(_connection_state->get()));
        _msgr->ms_deliver_handle_fast_connect(static_cast<Connection*>(_connection_state->get()));
  
        if (!_reader_running)
        {
            start_reader();
        }
        
        start_delay_thread();
        
        return 0;
    }

    connect_fail();

    return -1;
}


void Socket::register_socket()
{
    Socket* existing = _msgr->lookup_socket(_peer_addr);
    _msgr->_rank_socket[_peer_addr] = this;
}

void Socket::unregister_socket()
{
    // std::unordered_map<entity_addr_t, Socket*>::iterator iter = _msgr->_rank_socket.find(_peer_addr);
    std::map<entity_addr_t, Socket*>::iterator iter = _msgr->_rank_socket.find(_peer_addr);
    if (iter != _msgr->_rank_socket.end() && iter->second == this)
    {
        _msgr->_rank_socket.erase(iter);
    }
    else
    {
        _msgr->_accepting_sockets.erase(this);
    }
}

void Socket::join()
{
    if (_writer_thread.is_started())
    {
        _writer_thread.join();
    }
    
    if (_reader_thread.is_started())
    {
        _reader_thread.join();
    }
    
    if (_delay_thread)
    {
        _delay_thread->stop();
        _delay_thread->join();
    }
}

void Socket::requeue_sent()
{
    if (_sent.empty())
    {
        return;
    }

    std::list<Message*>& rq = _out_q[MSG_PRIO_HIGHEST];
    while (!_sent.empty())
    {
        Message* m = _sent.back();
        _sent.pop_back();
        rq.push_front(m);
        _out_seq--;
    }
}

void Socket::discard_requeued_up_to(uint64_t seq)
{
    if (0 == _out_q.count(MSG_PRIO_HIGHEST))
    {
        return;
    }
    
    std::list<Message*>& rq = _out_q[MSG_PRIO_HIGHEST];
    while (!rq.empty())
    {
        Message* m = rq.front();
        if (0 == m->get_seq() || m->get_seq() > seq)
        {
            break;
        }
        m->dec();
        rq.pop_front();
        _out_seq++;
    }
    
    if (rq.empty())
    {
        _out_q.erase(MSG_PRIO_HIGHEST);
    }
}

void Socket::discard_out_queue()
{
    for (std::list<Message*>::iterator iter = _sent.begin(); iter != _sent.end(); ++iter)
    {
        (*iter)->dec();
    }
    
    _sent.clear();
    
    for (std::map<int, std::list<Message*> >::iterator iter = _out_q.begin(); iter != _out_q.end(); ++iter)
    {
        for (std::list<Message*>::iterator r = iter->second.begin(); r != iter->second.end(); ++r)
        {
            (*r)->dec();
        }
    }
    
    _out_q.clear();
}

void Socket::fault(bool onread)
{
    DEBUG_LOG("socket fault, current state is %s", get_state_name(_state));
    
    _cond.signal();

    if (onread && _state == SOCKET_CONNECTING)
    {
        return;
    }

    if (_state == SOCKET_CLOSED || _state == SOCKET_CLOSING)
    {
        if (_connection_state->clear_socket(this))
        {
            _msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
        }
        
        return;
    }

    shutdown_socket();

    // socket策略为lossy时,出错即被删除
    if (_policy._lossy && _state != SOCKET_CONNECTING)
    {
        stop();
        bool cleared = _connection_state->clear_socket(this);
        
        _lock.unlock();
        _msgr->_lock.lock();
        _lock.lock();        
        unregister_socket();
        _msgr->_lock.unlock();

        if (_delay_thread)
        {
            _delay_thread->discard();
        }
        
        _in_q->discard_queue(_conn_id);
        discard_out_queue();
        if (cleared)
        {
            _msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
        }
        
        return;
    }

    if (_delay_thread)
    {
        _delay_thread->flush();
    }

    requeue_sent();

    if (_policy._standby && !is_queued())
    {
        _state = SOCKET_STANDBY;
        
        return;
    }

    if (_state != SOCKET_CONNECTING)
    {
        if (_policy._server)
        {
            _state = SOCKET_STANDBY;
        }
        else
        {
            _connect_seq++;
            _state = SOCKET_CONNECTING;
        }
        
        _backoff = utime_t();
    }
    else if (_backoff == utime_t())
    {
        _backoff.set_from_double(.2);
    }
    else
    {
        _cond.timed_wait(_lock, _backoff.to_msec());
        _backoff += _backoff;
        if (_backoff > 15.0)
        {
            _backoff.set_from_double(15.0);
        }
    }
}

void Socket::accept_fail()
{
    Mutex::Locker(_lock);
    
    if (SOCKET_CLOSED != _state)
    {    
        bool queued = is_queued();

        if (queued)
        {
            _state = _policy._server ? SOCKET_STANDBY : SOCKET_CONNECTING;
        }
        else if (_replaced)
        {
            _state = SOCKET_STANDBY;
        }
        else
        {
            set_state_close();
        }
        
        fault();

        // 有消息需要发送
        if (queued || _replaced)
        {
            start_writer();
        }
    }
}

void Socket::connect_fail()
{
    Mutex::Locker(_lock);

    if (SOCKET_CONNECTING == _state)
    {
        fault();
    }
}

void Socket::was_session_reset()
{
    _in_q->discard_queue(_conn_id);
    
    if (_delay_thread)
    {
        _delay_thread->discard();
    }
    
    discard_out_queue();

    _msgr->_dispatch_queue.queue_remote_reset(static_cast<Connection*>(_connection_state->get()));

    _in_seq = 0;
    _connect_seq = 0;
}

void Socket::stop()
{
    _state = SOCKET_CLOSED;
    atomic_set(&_state_closed, 1);
    _cond.signal();
    shutdown_socket();
}

void Socket::stop_and_wait()
{
    if (_state != SOCKET_CLOSED)
    {
        stop();
    }
  
    if (_delay_thread)
    {
        _lock.unlock();
        _delay_thread->stop_fast_dispatching();
        _lock.lock();
    }
    
    while (_reader_running && _reader_dispatching)
    {
        _cond.wait(_lock);
    }
}

void Socket::reader()
{
    DEBUG_LOG("socket reader start");
    
    _lock.lock();

    if (_state == SOCKET_ACCEPTING)
    {
        accepting();
    }

    while (_state != SOCKET_CLOSED && _state != SOCKET_CONNECTING)
    {
        if (_state == SOCKET_STANDBY)
        {
            _cond.wait(_lock);
            continue;
        }

        _lock.unlock();

        char tag = -1;
        if (0 > tcp_read((char*)&tag, 1))
        {
            _lock.lock();
            fault(true);
            continue;
        }

        if (tag == MSGR_TAG_KEEPALIVE)
        {
            _lock.lock();
            _connection_state->set_last_keepalive(clock_now());
            continue;
        }
        
        if (tag == MSGR_TAG_KEEPALIVE2)
        {
            struct timespec t;
            int rc = tcp_read((char*)&t, sizeof(t));
            _lock.lock();
            if (0 > rc)
            {
                fault(true);
            }
            else
            {
                _send_keepalive_ack = true;
                _keepalive_ack_stamp = utime_t(t);
                _connection_state->set_last_keepalive(clock_now());
                _cond.signal();
            }
            
            continue;
        }
        
        if (tag == MSGR_TAG_KEEPALIVE2_ACK)
        {
            struct timespec t;
            int rc = tcp_read((char*)&t, sizeof(t));
            _lock.lock();
            if (0 > rc)
            {
                fault(true);
            }
            else
            {
                _connection_state->set_last_keepalive_ack(utime_t(t));
            }
            
            continue;
        }

        if (tag == MSGR_TAG_ACK)
        {
            le64 seq;
            int rc = tcp_read((char*)&seq, sizeof(seq));
            _lock.lock();
            if (0 > rc)
            {
                fault(true);
            }
            else if (_state != SOCKET_CLOSED)
            {
                handle_ack(seq);
            }
            
            continue;
        }
        else if (tag == MSGR_TAG_MSG)
        {
            Message* m = NULL;
            int r = read_message(&m);

            _lock.lock();
      
            if (!m)
            {
                if (0 > r)
                {
                    fault(true);
                }
                
                continue;
            }

            if (_state == SOCKET_CLOSED || _state == SOCKET_CONNECTING)
            {
                _in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
                m->dec();
                continue;
            }
            
            if (m->get_seq() <= _in_seq)
            {
                _in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
                m->dec();
            }
            
            if (m->get_seq() > _in_seq + 1)
            {
                ERROR_LOG("reader missed message? skipped from seq %d to %d", _in_seq, m->get_seq());
            }

            m->set_connection(static_cast<Connection*>(_connection_state->get()));

            _in_seq = m->get_seq();

            _cond.signal();
      
            _in_q->fast_preprocess(m);

            if (_delay_thread)
            {
                utime_t release;                
                _delay_thread->queue(release, m);
            }
            else
            {
                if (_in_q->can_fast_dispatch(m))
                {
                    _reader_dispatching = true;
                    _lock.unlock();
                    _in_q->fast_dispatch(m);
                    _lock.lock();
                    _reader_dispatching = false;
                    if (_state == SOCKET_CLOSED || _notify_on_dispatch_done)
                    {
                        _notify_on_dispatch_done = false;
                        _cond.signal();
                    }
                }
                else
                {
                    _in_q->enqueue(m, m->get_priority(), _conn_id);
                }
            }
        }
        else if (tag == MSGR_TAG_CLOSE)
        {
            _lock.lock();
            if (_state == SOCKET_CLOSING)
            {
                _state = SOCKET_CLOSED;
                atomic_set(&_state_closed, 1);
            }
            else
            {
                _state = SOCKET_CLOSING;
            }
            
            _cond.signal();
            break;
        }
        else
        {
            _lock.lock();
            fault(true);
        }
    }
    
    _reader_running = false;
    _reader_needs_join = true;
    unlock_maybe_reap();
}

void Socket::writer()
{
    DEBUG_LOG("socket writer start");
    
    _lock.lock();
    
    while (_state != SOCKET_CLOSED)
    {
        if (is_queued() && _state == SOCKET_STANDBY && !_policy._server)
        {
            _state = SOCKET_CONNECTING;
        }

        if (_state == SOCKET_CONNECTING)
        {
            DEBUG_LOG("socket state is SOCKET_CONNECTING, will try connect");
            connecting();
            continue;
        }
    
        if (_state == SOCKET_CLOSING)
        {
            char tag = MSGR_TAG_CLOSE;
            _state = SOCKET_CLOSED;
            atomic_set(&_state_closed, 1);
            _lock.unlock();
            if (0 <= _fd)
            {
                int r = ::write(_fd, &tag, 1);
                (void)r;
            }
            _lock.lock();
            continue;
        }

        if (_state != SOCKET_CONNECTING && _state != SOCKET_WAIT && _state != SOCKET_STANDBY &&
            (is_queued() || _in_seq > _in_seq_acked))
        {
            if (_send_keepalive)
            {
                int rc;

                _lock.unlock();
                rc = write_keepalive();
                
                _lock.lock();
                if (0 > rc)
                {
                    fault();
                    continue;
                }
                _send_keepalive = false;
            }
            
            if (_send_keepalive_ack)
            {
                utime_t t = _keepalive_ack_stamp;
                _lock.unlock();
                int rc = write_keepalive();
                _lock.lock();
                if (0 > rc)
                {
                    fault();
                    continue;
                }
                _send_keepalive_ack = false;
            }

            if (_in_seq > _in_seq_acked)
            {
                uint64_t send_seq = _in_seq;
                _lock.unlock();
                int rc = write_ack(send_seq);
                _lock.lock();
                if (0 > rc)
                {
                    fault();
                    continue;
                }
                _in_seq_acked = send_seq;
            }

            Message* m = get_next_outgoing();
            if (m)
            {
                m->set_seq(++_out_seq);
                if (!_policy._lossy)
                {
                    _sent.push_back(m); 
                    m->get();
                }

                m->set_connection(static_cast<Connection*>(_connection_state->get()));

                if (m->empty_payload())
                {
                }
                else
                {
                }

                m->encode(_msgr->_crc_flag);

                const msg_header& header = m->get_header();
                const msg_footer& footer = m->get_footer();

                buffer buf = m->get_payload();
                buf.append(m->get_middle());
                buf.append(m->get_data());

                _lock.unlock();

                int rc = write_message(header, footer, buf);

                _lock.lock();
                if (0 > rc)
                {
                    fault();
                }
                m->dec();
            }
            
            continue;
        }
    
        _cond.wait(_lock);
    }
    
    _writer_running = false;
    unlock_maybe_reap();
}

void Socket::unlock_maybe_reap()
{
    if (!_reader_running && !_writer_running)
    {
        shutdown_socket();
        _lock.unlock();
        if (_delay_thread && _delay_thread->is_flushing())
        {
            _delay_thread->wait_for_flush();
        }
        _msgr->queue_reap(this);
    }
    else
    {
        _lock.unlock();
    }
}

static void alloc_aligned_buffer(buffer& data, uint32_t len, uint32_t off)
{
    uint32_t left = len;
    if (off & ~PAGE_MASK)
    {
        uint32_t head = 0;
        head = MIN(PAGE_SIZE - (off & ~PAGE_MASK), left);
        data.push_back(create(head));
        left -= head;
    }
    
    uint32_t middle = left & PAGE_MASK;
    if (0 < middle)
    {
        data.push_back(create_page_aligned(middle));
        left -= middle;
    }
    
    if (left)
    {
        data.push_back(create(left));
    }
}

int Socket::read_message(Message** pm)
{
    int ret = -1;
  
    msg_header header; 
    msg_footer footer;
    uint32_t header_crc = 0;

    if (tcp_read((char*)&header, sizeof(header)) < 0)
    {
        return -1;
    }
    
    if (_msgr->_crc_flag & MSG_CRC_HEADER)
    {
        header_crc = crc32c(0, (unsigned char *)&header, sizeof(header) - sizeof(header.crc));
    }

    if ((_msgr->_crc_flag & MSG_CRC_HEADER) && header_crc != header.crc)
    {
        return -1;
    }

    buffer front, middle, data;
    int front_len, middle_len;
    uint32_t data_len, data_off;
    int aborted;
    Message* message;
    utime_t recv_stamp = clock_now();

    if (_policy._throttler_messages)
    {
        _policy._throttler_messages->get();
    }

    uint64_t message_size = header.front_len + header.middle_len + header.data_len;
    if (message_size)
    {
        if (_policy._throttler_bytes)
        {
            _policy._throttler_bytes->get(message_size);
        }
        
        _msgr->_dispatch_throttler.get(message_size);
    }

    utime_t throttle_stamp = clock_now();

    front_len = header.front_len;
    if (front_len)
    {
        ptr bp = create(front_len);
        if (0 > tcp_read(bp.c_str(), front_len))
        {
            goto out_dethrottle;
        }
        
        front.push_back(std::move(bp));
    }

    middle_len = header.middle_len;
    if (middle_len)
    {
        ptr bp = create(middle_len);
        if (0 > tcp_read(bp.c_str(), middle_len))
        {
            goto out_dethrottle;
        }
        
        middle.push_back(std::move(bp));
    }


    data_len = le32_to_cpu(header.data_len);
    data_off = le32_to_cpu(header.data_off);
    if (data_len)
    {
        uint32_t offset = 0;
        uint32_t left = data_len;

        buffer newbuf, rxbuf;
        buffer::iterator blp;
        int rxbuf_version = 0;
    
        while (0 < left)
        {
            if (0 > tcp_read_wait())
            {
                goto out_dethrottle;
            }

            _connection_state->_lock.lock();
            std::map<uint64_t, std::pair<buffer, int> >::iterator p = _connection_state->_rx_buffers.find(header.tid);
            if (p != _connection_state->_rx_buffers.end())
            {
                if (rxbuf.length() == 0 || p->second.second != rxbuf_version)
                {
                    rxbuf = p->second.first;
                    rxbuf_version = p->second.second;
                    
                    if (rxbuf.length() < data_len)
                    {
                        rxbuf.push_back(create(data_len - rxbuf.length()));
                    }
                    blp = p->second.first.begin();
                    blp.advance(offset);
                }
            }
            else
            {
                if (!newbuf.length())
                {
                    alloc_aligned_buffer(newbuf, data_len, data_off);
                    blp = newbuf.begin();
                    blp.advance(offset);
                }
            }
            
            ptr bp = blp.get_current_ptr();
            int read = MIN(bp.length(), left);
            ssize_t got = tcp_read_nonblocking(bp.c_str(), read);
            _connection_state->_lock.unlock();
            
            if (0 > got)
            {
                goto out_dethrottle;
            }
            
            if (0 < got)
            {
                blp.advance(got);
                data.append(bp, 0, got);
                offset += got;
                left -= got;
            }
        }
    }

    if (0 > tcp_read((char*)&footer, sizeof(footer)))
    {
        goto out_dethrottle;
    }
  
    aborted = (footer.flags & MSG_FOOTER_COMPLETE) == 0;

    if (aborted)
    {
        ret = 0;
        goto out_dethrottle;
    }

    message = decode_message(_msgr->_crc_flag, header, footer, front, middle, data);
    if (!message)
    {
        ret = -EINVAL;
        goto out_dethrottle;
    }

    message->set_byte_throttler(_policy._throttler_bytes);
    message->set_message_throttler(_policy._throttler_messages);

    message->set_dispatch_throttle_size(message_size);

    message->set_recv_stamp(recv_stamp);
    message->set_throttle_stamp(throttle_stamp);
    message->set_recv_complete_stamp(clock_now());

    *pm = message;
    return 0;

out_dethrottle:
    
    if (_policy._throttler_messages)
    {
        _policy._throttler_messages->put();
    }
    
    if (message_size)
    {
        if (_policy._throttler_bytes)
        {
            _policy._throttler_bytes->put(message_size);
        }

        _in_q->dispatch_throttle_release(message_size);
    }
    
    return ret;
}

void Socket::suppress_signal()
{
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
    sigset_t pending;
    sigemptyset(&pending);
    sigpending(&pending);
    sigpipe_pending = sigismember(&pending, SIGPIPE);
    if (!sigpipe_pending)
    {
        sigset_t blocked;
        sigemptyset(&blocked);
        pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &blocked);

        sigpipe_unblock = ! sigismember(&blocked, SIGPIPE);
    }
#endif
}

void Socket::restore_signal()
{
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
    if (!sigpipe_pending)
    {
        sigset_t pending;
        sigemptyset(&pending);
        sigpending(&pending);
        if (sigismember(&pending, SIGPIPE))
        {
            static const struct timespec nowait = { 0, 0 };
            TEMP_FAILURE_RETRY(sigtimedwait(&sigpipe_mask, NULL, &nowait));
        }

        if (sigpipe_unblock)
        {
            pthread_sigmask(SIG_UNBLOCK, &sigpipe_mask, NULL);
        }
    }
#endif
}

int Socket::do_sendmsg(struct msghdr* msg, uint32_t len, bool more)
{
    suppress_signal();
    while (0 < len)
    {
        int r;
#if defined(MSG_NOSIGNAL)
        r = ::sendmsg(_fd, msg, MSG_NOSIGNAL | (more ? MSG_MORE : 0));
#else
        r = ::sendmsg(_fd, msg, (more ? MSG_MORE : 0));
#endif
        if (0 == r)
        {
        }
        
        if (0 > r)
        {
            r = -errno; 
            restore_signal();
            return r;
        }
        
        if (_state == SOCKET_CLOSED)
        {
            restore_signal();
            return -EINTR;
        }

        len -= r;
        if (0 == len)
        {
            break;
        }
    
        while (0 < r)
        {
            if (msg->msg_iov[0].iov_len <= (size_t)r)
            {
                r -= msg->msg_iov[0].iov_len;
                msg->msg_iov++;
                msg->msg_iovlen--;
            }
            else
            {
                msg->msg_iov[0].iov_base = (char *)msg->msg_iov[0].iov_base + r;
                msg->msg_iov[0].iov_len -= r;
                break;
            }
        }
    }
    
    restore_signal();
    return 0;
}

int Socket::write_ack(uint64_t seq)
{
    char c = MSGR_TAG_ACK;
    le64 s;
    s = seq;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    struct iovec msgvec[2];
    msgvec[0].iov_base = &c;
    msgvec[0].iov_len = 1;
    msgvec[1].iov_base = &s;
    msgvec[1].iov_len = sizeof(s);
    msg.msg_iov = msgvec;
    msg.msg_iovlen = 2;
  
    if (0 > do_sendmsg(&msg, 1 + sizeof(s), true))
    {
        return -1;
    }
    
    return 0;
}

int Socket::write_keepalive()
{
    char c = MSGR_TAG_KEEPALIVE;

    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    struct iovec msgvec[2];
    msgvec[0].iov_base = &c;
    msgvec[0].iov_len = 1;
    msg.msg_iov = msgvec;
    msg.msg_iovlen = 1;
  
    if (0 > do_sendmsg(&msg, 1))
    {
        return -1;
    }
    
    return 0;
}

int Socket::write_message(const msg_header& header, const msg_footer& footer, buffer& buf)
{
    int ret = 0;
    
    struct msghdr msg;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = _msgvec;
    int msglen = 0;
  
    char tag = MSGR_TAG_MSG;
    _msgvec[msg.msg_iovlen].iov_base = &tag;
    _msgvec[msg.msg_iovlen].iov_len = 1;
    msglen++;
    msg.msg_iovlen++;

    _msgvec[msg.msg_iovlen].iov_base = (char*)&header;
    _msgvec[msg.msg_iovlen].iov_len = sizeof(header);
    msglen += sizeof(header);
    msg.msg_iovlen++;

    std::list<ptr>::const_iterator it = buf.ptrs().begin();
    uint32_t b_off = 0;
    uint32_t bl_pos = 0;
    uint32_t left = buf.length();

    while (0 < left)
    {
        uint32_t donow = MIN(left, it->length() - b_off);
        if (0 == donow)
        {
            //
        }
    
        if (msg.msg_iovlen >= SM_IOV_MAX - 2)
        {
            if (do_sendmsg(&msg, msglen, true))
            {
                goto fail;
            }
      
            msg.msg_iov = _msgvec;
            msg.msg_iovlen = 0;
            msglen = 0;
        }
    
        _msgvec[msg.msg_iovlen].iov_base = (void*)(it->c_str()+b_off);
        _msgvec[msg.msg_iovlen].iov_len = donow;
        msglen += donow;
        msg.msg_iovlen++;
    
        left -= donow;
        b_off += donow;
        bl_pos += donow;
        
        if (0 == left)
        {
            break;
        }
        
        while (b_off == it->length())
        {
            ++it;
            b_off = 0;
        }
    }

    _msgvec[msg.msg_iovlen].iov_base = (void*)&footer;
    _msgvec[msg.msg_iovlen].iov_len = sizeof(footer);
    msglen += sizeof(footer);
    msg.msg_iovlen++;

    if (do_sendmsg(&msg, msglen))
    {
        goto fail;
    }

    ret = 0;

out:
    return ret;

fail:
    ret = -1;
    goto out;
}

int Socket::tcp_read(char* buf, uint32_t len)
{
    if (0 > _fd)
    {
        return -EINVAL;
    }

    while (0 < len)
    {
        if (0 > tcp_read_wait())
        {
            return -1;
        }

        ssize_t got = tcp_read_nonblocking(buf, len);

        if (0 > got)
        {
            return -1;
        }

        len -= got;
        buf += got;
    }
    
    return 0;
}

int Socket::tcp_read_wait()
{
    if (0 > _fd)
    {
        return -EINVAL;
    }
    
    struct pollfd pfd;
    short evmask;
    pfd.fd = _fd;
    pfd.events = POLLIN;
#if defined(__linux__)
    pfd.events |= POLLRDHUP;
#endif

    if (has_pending_data())
    {
        return 0;
    }

    int r = poll(&pfd, 1, _msgr->_timeout);
    if (0 > r)
    {
        return -errno;
    }
    
    if (0 == r)
    {
        return -EAGAIN;
    }

    evmask = POLLERR | POLLHUP | POLLNVAL;
#if defined(__linux__)
    evmask |= POLLRDHUP;
#endif
    if (pfd.revents & evmask)
    {
        return -1;
    }

    if (!(pfd.revents & POLLIN))
    {
        return -1;
    }

    return 0;
}

ssize_t Socket::do_recv(char* buf, size_t len, int flags)
{
again:
    ssize_t got = ::recv(_fd, buf, len, flags);
    if (0 > got)
    {
        if (errno == EINTR)
        {
            goto again;
        }

        return -1;
    }
    
    if (0 == got)
    {
        return -1;
    }
    
    return got;
}

ssize_t Socket::buffered_recv(char* buf, size_t len, int flags)
{
    size_t left = len;
    ssize_t total_recv = 0;
    if (_recv_len > _recv_ofs)
    {
        int to_read = MIN(_recv_len - _recv_ofs, left);
        memcpy(buf, &_recv_buf[_recv_ofs], to_read);
        _recv_ofs += to_read;
        left -= to_read;
        if (0 == left)
        {
            return to_read;
        }
        buf += to_read;
        total_recv += to_read;
    }

    if (left > _recv_max_prefetch)
    {
        ssize_t ret = do_recv(buf, left, flags );
        if (0 > ret)
        {
            if (0 < total_recv)
            {
                return total_recv;
            }
            return ret;
        }
        
        total_recv += ret;
        return total_recv;
    }


    ssize_t got = do_recv(_recv_buf, _recv_max_prefetch, flags);
    if (0 > got)
    {
        if (0 < total_recv)
        {
            return total_recv;
        }

        return got;
    }

    _recv_len = (size_t)got;
    got = MIN(left, (size_t)got);
    memcpy(buf, _recv_buf, got);
    _recv_ofs = got;
    total_recv += got;
    return total_recv;
}

ssize_t Socket::tcp_read_nonblocking(char* buf, uint32_t len)
{
    ssize_t got = buffered_recv(buf, len, MSG_DONTWAIT);
    if (0 > got)
    {
        return -1;
    }
    
    if (0 == got)
    {
        return -1;
    }
    
    return got;
}

int Socket::tcp_write(const char* buf, uint32_t len)
{
    if (0 > _fd)
    {
        return -1;
    }
    
    struct pollfd pfd;
    pfd.fd = _fd;
    pfd.events = POLLOUT | POLLHUP | POLLNVAL | POLLERR;
#if defined(__linux__)
    pfd.events |= POLLRDHUP;
#endif

    if (0 > poll(&pfd, 1, -1))
    {
        return -1;
    }

    if (!(pfd.revents & POLLOUT))
    {
        return -1;
    }
    
    suppress_signal();

    while (0 < len)
    {
        int did;
#if defined(MSG_NOSIGNAL)
        did = ::send(_fd, buf, len, MSG_NOSIGNAL);
#else
        did = ::send(_fd, buf, len, 0);
#endif
        if (0 > did)
        {
            return did;
        }
        
        len -= did;
        buf += did;
    }
    
    restore_signal();

    return 0;
}

void Socket::DelayedDelivery::discard()
{
    Mutex::Locker locker(_delay_lock);
    while (!_delay_queue.empty())
    {
        Message* m = _delay_queue.front().second;
        _socket->_in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
        m->dec();
        _delay_queue.pop_front();
    }
}

void Socket::DelayedDelivery::flush()
{
    Mutex::Locker locker(_delay_lock);
    _flush_count = _delay_queue.size();
    _delay_cond.signal();
}

void Socket::DelayedDelivery::entry()
{
    Mutex::Locker locker(_delay_lock);

    while (!_stop_delayed_delivery)
    {
        if (_delay_queue.empty())
        {
            _delay_cond.wait(_delay_lock);
            continue;
        }
        
        utime_t release = _delay_queue.front().first;
        Message* m = _delay_queue.front().second;
        // config
        // 配置延迟发送的消息类型
        // 如果还没有到发送时间则线程挂起一段时间
        std::string delay_msg_type = "";
        if (!_flush_count && (release > clock_now() && (delay_msg_type.empty() || m->get_type_name() == delay_msg_type)))
        {
            _delay_cond.timed_wait(_delay_lock, release.to_msec());
            continue;
        }
        
        _delay_queue.pop_front();
        
        if (0 < _flush_count)
        {
            --_flush_count;
            _active_flush = true;
        }
        
        if (_socket->_in_q->can_fast_dispatch(m))
        {
            if (!_stop_fast_dispatching_flag)
            {
                _delay_dispatching = true;
                _delay_lock.unlock();
                _socket->_in_q->fast_dispatch(m);
                _delay_lock.lock();
                _delay_dispatching = false;
                if (_stop_fast_dispatching_flag)
                {
                    // 唤醒其他挂起的线程
                    _delay_cond.signal();
                    _delay_lock.unlock();
                    _delay_lock.lock();
                }
            }
        }
        else
        {
            _socket->_in_q->enqueue(m, m->get_priority(), _socket->_conn_id);
        }
        
        _active_flush = false;
    }
}

void Socket::DelayedDelivery::stop_fast_dispatching()
{
    Mutex::Locker locker(_delay_lock);
    _stop_fast_dispatching_flag = true;
    while (_delay_dispatching)
        _delay_cond.wait(_delay_lock);
}

// SYS_NS_END
