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
	Connection(Messenger* m) : _lock(), _msgr(m), _priv(NULL), _peer_type(-1),
								_features(0), _failed(false), _rx_buffers_version(0)
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

	const entity_addr_t& get_peer_addr() const { return _peer_addr; }
	
	void set_peer_addr(const entity_addr_t& a) { _peer_addr = a; }

	uint64_t get_features() const { return _features; }
	
	bool has_feature(uint64_t f) const { return _features & f; }
	
	bool has_features(uint64_t f) const
	{
		return (_features & f) == f;
	}
	
	void set_features(uint64_t f) { _features = f; }
	
	void set_feature(uint64_t f) { _features |= f; }

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

	utime_t get_last_keepalive() const
	{
		Mutex::Locker locker(_lock);
		return _last_keepalive;
	}
	
	void set_last_keepalive(utime_t t)
	{
		Mutex::Locker locker(_lock);
		_last_keepalive = t;
	}
	
	utime_t get_last_keepalive_ack() const
	{
		Mutex::Locker locker(_lock);
		return _last_keepalive_ack;
	}
	
	void set_last_keepalive_ack(utime_t t)
	{
		Mutex::Locker locker(_lock);
		_last_keepalive_ack = t;
	}

public:
	mutable Mutex _lock;
	Messenger* _msgr;
	RefCountable* _priv;
	int _peer_type;
	entity_addr_t _peer_addr;
	utime_t _last_keepalive;
	utime_t _last_keepalive_ack;
	bool _failed;

	int _rx_buffers_version;
	std::map<uint64_t, std::pair<buffer, int> > _rx_buffers;

	friend class SocketConnection;

private:
	uint64_t _features;
};

#endif
