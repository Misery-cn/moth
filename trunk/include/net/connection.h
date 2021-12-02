#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "time_utils.h"
#include "ref_countable.h"
#include "mutex.h"

class Messenger;
class Message;


class Connection : public RefCountable
{
public:
    Connection(Messenger* m) : _lock(), _msgr(m), _priv(NULL), _peer_type(-1), _failed(false), _rx_buffers_version(0)
    {
    }

    virtual ~Connection()
    {
        if (_priv)
        {
            _priv->dec();
        }
    }

    Messenger* get_messenger() { return _msgr; }

    /**
     * 设置引用
     *
     */
    void set_priv(RefCountable* ref)
    {
        // MutexGuard(_lock);
        Mutex::Locker locker(_lock);
        // 之前的引用-1
        if (_priv)
        {
            _priv->dec();
        }

        _priv = ref;
    }

    /**
     * 获取引用
     *
     */
    RefCountable* get_priv()
    {
        // MutexGuard(_lock);
        Mutex::Locker locker(_lock);

        if (_priv)
        {
            return _priv->get();
        }

        return NULL;
    }

    virtual bool is_connected() = 0;

    virtual int send_message(Message* m) = 0;

    virtual void send_keepalive() = 0;

    virtual void mark_disposable() = 0;

    virtual void mark_down() = 0;

    int get_peer_type() const { return _peer_type; }
    
    void set_peer_type(int t) { _peer_type = t; }

    /**
     * 获取连接的地址
     *
     */
    const entity_addr_t& get_peer_addr() const { return _peer_addr; }

    /**
     * 设置连接地址
     *
     */
    void set_peer_addr(const entity_addr_t& a) { _peer_addr = a; }

    void post_rx_buffer(uint64_t tid, buffer& buf)
    {
        Mutex::Locker locker(_lock);
        ++_rx_buffers_version;
        _rx_buffers[tid] = std::pair<buffer, int>(buf, _rx_buffers_version);
    }

    void revoke_rx_buffer(uint64_t tid)
    {
        Mutex::Locker l(_lock);
        _rx_buffers.erase(tid);
    }

    /**
     * 获取上一次的心跳时间
     *
     */
    utime_t get_last_keepalive() const
    {
        Mutex::Locker locker(_lock);
        return _last_keepalive;
    }

    /**
     * 设置最新的心跳时间
     *
     */
    void set_last_keepalive(utime_t t)
    {
        Mutex::Locker locker(_lock);
        _last_keepalive = t;
    }

    /**
     * 获取上一次的心跳ack时间
     *
     */
    utime_t get_last_keepalive_ack() const
    {
        Mutex::Locker locker(_lock);
        return _last_keepalive_ack;
    }

    /**
     * 设置最新的心跳ack时间
     *
     */
    void set_last_keepalive_ack(utime_t t)
    {
        Mutex::Locker locker(_lock);
        _last_keepalive_ack = t;
    }

public:
    mutable Mutex _lock;
    Messenger* _msgr;
    RefCountable* _priv;

    // entity_name_t中的通信实体类型
    int _peer_type;
    // 连接的地址(对端)
    entity_addr_t _peer_addr;
    // 最新的心跳时间
    utime_t _last_keepalive;
    // 最新的心跳ack时间
    utime_t _last_keepalive_ack;

    // 连接状态
    bool _failed;

    int _rx_buffers_version;
    std::map<uint64_t, std::pair<buffer, int> > _rx_buffers;

    friend class SocketConnection;
};

#endif
