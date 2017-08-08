#ifndef _SIMPLE_MESSENGER_H_
#define _SIMPLE_MESSENGER_H_

#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include "log.h"
#include "spinlock.h"
#include "messenger.h"
#include "socket.h"
#include "accepter.h"
#include "dispatch_queue.h"


class SimpleMessenger : public PolicyMessenger
{
public:
	SimpleMessenger(entity_name_t name, std::string mname);

	virtual ~SimpleMessenger();

	void set_addr_unknowns(entity_addr_t& addr);

	// 获取消息转发队列大小
	int get_dispatch_queue_len()
	{
		return _dispatch_queue.get_queue_len();
	}

	// 获取消息转发队列中最大时间
	double get_dispatch_queue_max_age(utime_t now)
	{
		return _dispatch_queue.get_max_age(now);
	}

	// 绑定端口
	int bind(const entity_addr_t& bind_addr);

	// 重新绑定端口
	int rebind(const std::set<int>& avoid_ports);

	// 启动messenger
	int start();
	// 等待线程退出
	void wait();
	// 停止messenger
	int shutdown();
	// 根据地址信息发送消息
	int send_message(Message* m, const entity_inst_t& dest);
	// 根据已有连接发送消息
	int send_message(Message* m, Connection* con);
	// 根据地址获取连接
	Connection* get_connection(const entity_inst_t& dest);
	// 获取本地连接
	Connection* get_loopback_connection()
	{
		return _local_connection;
	}
	// 发送心跳
	int send_keepalive(Connection* con);
	// 根据地址信息停止连接
	void mark_down(const entity_addr_t& addr);
	// 根据连接信息停止连接
	void mark_down(Connection* con);
	
	void mark_disposable(Connection* con);
	// 关闭所有连接
	void mark_down_all();

protected:
	void ready();

public:
	// 负责绑定端口、监听的accepter线程
	Accepter _accepter;
	// 发送队列
	DispatchQueue _dispatch_queue;

	friend class Accepter;

	Socket* add_accept_socket(int fd);

private:
	// 负责关闭socket的线程
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

	void submit_message(Message* m, SocketConnection* con, const entity_addr_t& addr, int dest_type, bool already_locked);
	
	// 关闭socket
	void reaper();

private:
	
	Mutex _lock;
	
	// 是否已绑定
	bool _did_bind;
	
	// 全局sequence
	uint32_t _global_seq;
	
	// _global_seq锁
	SpinLock _global_seq_lock;

	// std::unordered_map<entity_addr_t, Socket*> _rank_socket;
	std::map<entity_addr_t, Socket*> _rank_socket;

	std::set<Socket*> _accepting_sockets;

	std::set<Socket*> _sockets;
	
	// 待关闭socket队列
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

	void init_local_connection();
	
	void reaper_entry();

	void queue_reap(Socket* s);

	bool is_connected(Connection *con);
};

#endif