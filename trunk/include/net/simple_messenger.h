#ifndef _SIMPLE_MESSENGER_H_
#define _SIMPLE_MESSENGER_H_

#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include "spinlock.h"
#include "msg_features.h"
#include "messenger.h"
#include "socket.h"
#include "accepter.h"
#include "dispatch_queue.h"


class SimpleMessenger : public PolicyMessenger
{
public:
	SimpleMessenger(entity_name_t name, std::string mname, uint64_t _nonce);

	virtual ~SimpleMessenger();

	void set_addr_unknowns(entity_addr_t& addr);

	int get_dispatch_queue_len()
	{
		return _dispatch_queue.get_queue_len();
	}

	double get_dispatch_queue_max_age(utime_t now)
	{
		return _dispatch_queue.get_max_age(now);
	}

	int bind(const entity_addr_t& bind_addr);
	
	int rebind(const std::set<int>& avoid_ports);
	
	int start();
	
	void wait();
	
	int shutdown();
	
	int send_message(Message* m, const entity_inst_t& dest)
	{
		return _send_message(m, dest);
	}

	int send_message(Message* m, Connection* con)
	{
		return _send_message(m, con);
	}
	
	Connection* get_connection(const entity_inst_t& dest);

	Connection* get_loopback_connection()
	{
		return _local_connection;
	}
	
	int send_keepalive(Connection* con);
	
	void mark_down(const entity_addr_t& addr);
	
	void mark_down(Connection* con);
	
	void mark_disposable(Connection* con);
	
	void mark_down_all();

protected:
	void ready();

public:
	Accepter _accepter;
	DispatchQueue _dispatch_queue;

	friend class Accepter;

	Socket* add_accept_socket(int fd);

private:
	class ReaperThread : public Thread
	{
		SimpleMessenger* _msgr;
	public:
		explicit ReaperThread(SimpleMessenger* m) : _msgr(m) {}
		void entry()
		{
			_msgr->reaper_entry();
		}
	} _reaper_thread;

	Socket* connect_rank(const entity_addr_t& addr, int type, SocketConnection* con, Message* first);

	int _send_message(Message* m, const entity_inst_t& dest);

	int _send_message(Message* m, Connection* con);

	void submit_message(Message* m, SocketConnection* con, const entity_addr_t& addr, int dest_type, bool already_locked);

	void reaper();

	uint64_t _nonce;
	Mutex _lock;
	bool _need_addr;
	
public:
  bool get_need_addr() const { return _need_addr; }

private:

	bool _did_bind;

	uint32_t _global_seq;

	SpinLock _global_seq_lock;

	// std::unordered_map<entity_addr_t, Socket*> _rank_socket;
	std::map<entity_addr_t, Socket*> _rank_socket;

	std::set<Socket*> _accepting_sockets;

	std::set<Socket*> _sockets;

	std::list<Socket*> _socket_reap_queue;

	Cond  _stop_cond;
	bool _stopped = true;
	Throttle _dispatch_throttler;
	bool _reaper_started;
	bool _reaper_stop;
	Cond _reaper_cond;

	Cond  _wait_cond;

	friend class Socket;

	Socket* lookup_socket(const entity_addr_t& k)
	{
		// std::unordered_map<entity_addr_t, Socket*>::iterator iter = _rank_socket.find(k);
		std::map<entity_addr_t, Socket*>::iterator iter = _rank_socket.find(k);
		if (iter == _rank_socket.end())
		{
			return NULL;
		}

		if (atomic_read(&(iter->second->_state_closed)))
		{
			return NULL;
		}
		
		return iter->second;
	}

public:

	int _timeout;

	Connection* _local_connection;

	// AuthAuthorizer* get_authorizer(int peer_type, bool force_new);

	// bool verify_authorizer(Connection* con, int peer_type, int protocol, buffer& auth, buffer& auth_reply,
	//								bool& isvalid, CryptoKey& session_key);
	
	uint32_t get_global_seq(uint32_t old = 0)
	{
    	SpinLock::Locker locker(_global_seq_lock);
		
		if (old > _global_seq)
		{
			_global_seq = old;
		}
		
		uint32_t ret = ++_global_seq;
			
		return ret;
	}

	// int get_proto_version(int peer_type, bool connect);

	void init_local_connection();

	void learned_addr(const entity_addr_t& peer_addr_for_me);
	
	void reaper_entry();

	void queue_reap(Socket* s);

	bool is_connected(Connection *con);
};

#endif