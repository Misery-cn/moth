#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <deque>
// #include <list>
// #include <map>
// #include <unordered_map>
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
#include "time_utils.h"
#include "cond.h"
#include "thread.h"
#include "buffer.h"
#include "msg_types.h"
#include "messenger.h"
#include "socketconnection.h"

// SYS_NS_BEGIN

static const int SM_IOV_MAX = (IOV_MAX >= 1024 ? IOV_MAX / 4 : IOV_MAX);

class SimpleMessenger;
class DispatchQueue;


class Socket : public RefCountable
{
public:
    Socket(SimpleMessenger* r, int st, SocketConnection* con);
    Socket(const Socket& other);
    const Socket& operator=(const Socket& other);
    virtual ~Socket();
    
    enum SOCKET_STATE
    {
        SOCKET_ACCEPTING,
        SOCKET_CONNECTING,
        SOCKET_OPEN,
        SOCKET_STANDBY,
        SOCKET_CLOSED,
        SOCKET_CLOSING,
        SOCKET_WAIT    
    };

    // socket的读线程
    class Reader : public Thread
    {
    private:
        Socket* _socket;
    public:
        explicit Reader(Socket* s) : _socket(s) {}
        void entry() { _socket->reader(); }
    } _reader_thread;

    // socket的写线程
    class Writer : public Thread
    {
    private:
        Socket* _socket;
    public:
        explicit Writer(Socket* s) : _socket(s) {}
        void entry() { _socket->writer(); }
    } _writer_thread;

    // 延迟发送消息线程
    class DelayedDelivery : public Thread
    {
    private:
        Socket* _socket;
        // 待发送队列
        std::deque< std::pair<utime_t, Message*> > _delay_queue;
        Mutex _delay_lock;
        Cond _delay_cond;
        int _flush_count;
        // 线程已激活
        bool _active_flush;
        // 是否停止发送线程
        bool _stop_delayed_delivery;
        bool _delay_dispatching;
        bool _stop_fast_dispatching_flag;

    public:
        explicit DelayedDelivery(Socket* s) : _socket(s), _delay_lock(), _flush_count(0),
                    _active_flush(false), _stop_delayed_delivery(false), _delay_dispatching(false),
                    _stop_fast_dispatching_flag(false)
        {}
        
        virtual ~DelayedDelivery()
        {
            discard();
        }
        
        void entry();
        void queue(utime_t release, Message* m)
        {
            Mutex::Locker locker(_delay_lock);
            _delay_queue.push_back(std::make_pair(release, m));
            _delay_cond.signal();
        }
        void discard();
        void flush();
        bool is_flushing()
        {
            Mutex::Locker locker(_delay_lock);
            return _flush_count > 0 || _active_flush;
        }
        
        void wait_for_flush()
        {
            Mutex::Locker locker(_delay_lock);
            while (_flush_count > 0 || _active_flush)
            {
                _delay_cond.wait(_delay_lock);
            }
        }
        
        void stop()
        {
            Mutex::Locker locker(_delay_lock);
            _stop_delayed_delivery = true;
            _delay_cond.signal();
        }
        
        void steal_for_socket(Socket* s)
        {
            Mutex::Locker locker(_delay_lock);
            _socket= s;
        }
        
        void stop_fast_dispatching();
    } *_delay_thread;

    /**
     * 获取socket句柄
     *
     */
    Socket* get()
    {
        return static_cast<Socket*>(RefCountable::get());
    }

    /**
     * 判断socket是否已打开
     *
     */
    bool is_connected()
    {
        Mutex::Locker l(_lock);
        return _state == SOCKET_OPEN;
    }

    static const char* get_state_name(int s)
    {
        switch (s)
        {
            case SOCKET_ACCEPTING: return "accepting";
            case SOCKET_CONNECTING: return "connecting";
            case SOCKET_OPEN: return "open";
            case SOCKET_STANDBY: return "standby";
            case SOCKET_CLOSED: return "closed";
            case SOCKET_CLOSING: return "closing";
            case SOCKET_WAIT: return "wait";
            default: return "UNKNOWN";
        }
    }

    const char* get_current_state_name()
    {
        return get_state_name(_state);
    }

    int create_socket();

    // close函数会关闭套接字ID
    // 如果有其他的进程共享着这个套接字,那么它仍然是打开的
    int close_socket()
    {
        recv_reset();
    
        if (0 <= _fd)
        {
            ::close(_fd);
            _fd = -1;
        }
    }

    // SHUT_RD 关闭读功能
    // SHUT_WR 关闭写功能
    // SHUT_RDWR 关闭读写功能
    void shutdown_socket()
    {
        recv_reset();
        
        if (0 <= _fd)
        {
            ::shutdown(_fd, SHUT_RDWR);
            _fd = -1;
        }
    }

    void set_state_close()
    {
        _state = SOCKET_CLOSED;
        atomic_set(&_state_closed, 1);
    }
    
    // 设置选项
    void set_socket_options();
    // 启动读线程
    void start_reader();
    // 启动写线程
    void start_writer();
    
    void start_delay_thread();
    
    void join_reader();

    uint64_t get_out_seq() { return _out_seq; }

    bool is_queued() { return !_out_q.empty() || _send_keepalive || _send_keepalive_ack; }

    entity_addr_t& get_peer_addr() { return _peer_addr; }

    void set_peer_addr(const entity_addr_t& e)
    {
        if (&_peer_addr != &e)
        {
            _peer_addr = e;
        }
        
        _connection_state->set_peer_addr(e);
    }

    void set_peer_type(int t)
    {
        _peer_type = t;
        _connection_state->set_peer_type(t);
    }

    void register_socket();
    
    void unregister_socket();

    void join();

    void stop();

    void stop_and_wait();

    void send(Message* m)
    {
        _out_q[m->get_priority()].push_back(m);
        _cond.signal();
    }

    void send_keepalive()
    {
        _send_keepalive = true;
        _cond.signal();
    }

    Message* get_next_outgoing()
    {
        Message* m = NULL;
        while (!m && !_out_q.empty())
        {
            std::map<int, std::list<Message*> >::reverse_iterator iter = _out_q.rbegin();
            if (!iter->second.empty())
            {
                m = iter->second.front();
                iter->second.pop_front();
            }
            
            if (iter->second.empty())
            {
                _out_q.erase(iter->first);
            }
        }
        
        return m;
    }

    void requeue_sent();

    void discard_requeued_up_to(uint64_t seq);

    void discard_out_queue();

    void recv_reset()
    {
        _recv_len = 0;
        _recv_ofs = 0;
    }

    ssize_t do_recv(char* buf, size_t len, int flags);

    ssize_t buffered_recv(char* buf, size_t len, int flags);

    bool has_pending_data() { return _recv_len > _recv_ofs; }

    int tcp_read(char* buf, uint32_t len);

    int tcp_read_wait();

    ssize_t tcp_read_nonblocking(char* buf, uint32_t len);

    int tcp_write(const char* buf, uint32_t len);
    
    inline size_t getfd() {return _fd;}

public:
    
    static const Socket& Server(int s);
    static const Socket& Client(const entity_addr_t& e);

public:
    SimpleMessenger* _msgr;
    uint64_t _conn_id;

    int _port;
    int _peer_type;
    entity_addr_t _peer_addr;
    Messenger::Policy _policy;
    
    Mutex _lock;
    int _state;
    atomic_t _state_closed;

    char* _recv_buf;
    size_t _recv_max_prefetch;
    size_t _recv_ofs;
    size_t _recv_len;

protected:
    friend class SimpleMessenger;
    SocketConnection* _connection_state;
    utime_t _backoff;
    // 读线程是否启动
    bool _reader_running;
    bool _reader_needs_join;
    // 读线程是否在发送消息
    bool _reader_dispatching;
    // 消息是否发送完成
    bool _notify_on_dispatch_done;
    bool _writer_running;
    // 是否替换socket
    bool _replaced;
    bool _is_reset_from_peer;

    std::map<int, std::list<Message*> > _out_q;

    DispatchQueue* _in_q;

    // 发送队列
    std::list<Message*> _sent;

    Cond _cond;
    bool _send_keepalive;
    bool _send_keepalive_ack;
    utime_t _keepalive_ack_stamp;
    bool _halt_delivery;
    
    uint32_t _connect_seq;
    uint32_t _peer_global_seq;
    uint64_t _out_seq;
    uint64_t _in_seq;
    uint64_t _in_seq_acked;

    int accept();    
    
    int connect();

    int accepting();

    int connecting();

    void replace_socket(Socket* other);

    int open_socket(const msg_connect& connect);
    
    void reader();
    
    void writer();
    
    void unlock_maybe_reap();
    
    int read_message(Message** pm);
    
    int write_message(const msg_header& h, const msg_footer& f, buffer& body);
    
    int do_sendmsg(struct msghdr* msg, unsigned len, bool more = false);
    
    int write_ack(uint64_t s);
    
    int write_keepalive();
    
    int write_keepalive(char tag, const utime_t& t);
    
    void suppress_signal();
    
    void restore_signal();
    
    void fault(bool reader = false);

    void accept_fail();

    void connect_fail();
    
    void was_session_reset();

    void handle_ack(uint64_t seq);

private:

    int accept_failed();

    int connect_failed();

    int read_failed();

    int write_failed();


private:
    size_t _fd;
    struct iovec _msgvec[SM_IOV_MAX];
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
    sigset_t _sigpipe_mask;
    bool _sigpipe_pending;
    bool _sigpipe_unblock;
#endif
};

// SYS_NS_END

#endif
