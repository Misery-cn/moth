#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#include <list>
#include <set>
#include <map>
#include "message.h"
#include "dispatcher.h"

// 通信基类
class Messenger
{
public:
	Messenger();

	Messenger(entity_name_t entityname) : _entity(), _started(false), _magic(0)
	{
		_entity._name = entityname;
	}
	
	virtual ~Messenger() {};

	struct Policy
	{
		// 当该连接出现错误时就删除
		bool _lossy;
		// 服务器模式,被动连接
		bool _server;
		// 该连接处于等待状态
		bool _standby;
		// 连接出错后重连
		bool _resetcheck;


		// 按字节节流
		Throttle* _throttler_bytes;
		// 按消息数节流
		Throttle* _throttler_messages;

		Policy() : _lossy(false), _server(false), _standby(false), _resetcheck(true),
				   _throttler_bytes(NULL), _throttler_messages(NULL)
		{
		}
	};

	static Messenger* create(std::string type, entity_name_t name, std::string lname, uint64_t nonce = 0, uint64_t flags = 0);

	static int get_default_crc_flags();

	// 绑定地址,由派生类实现
	virtual int bind(const entity_addr_t& bind_addr) = 0;

	// 重新绑定端口,并且不能与之前绑定的端口冲突
	virtual int rebind(const std::set<int>& avoid_ports) { return -EOPNOTSUPP; }
	// 启动
	virtual int start() { _started = true; return 0; }
	// 停止
	virtual int shutdown() { _started = false; return 0; }
	// 阻塞等待关闭
	virtual void wait() = 0;
	// 发送消息
	virtual int send_message(Message* msg, const entity_inst_t& dest) = 0;
	// 标记连接断开
	virtual void mark_down(const entity_addr_t& addr) = 0;

	virtual void mark_down_all() = 0;

	virtual int get_dispatch_queue_len() = 0;

	virtual double get_dispatch_queue_max_age(utime_t now) = 0;

	// virtual void set_protocol(int p) = 0;

	virtual void set_default_policy(Policy p) = 0;

	virtual Policy get_default_policy() = 0;

	virtual void set_policy(int type, Policy p) = 0;

	virtual Policy get_policy(int t) = 0;

	virtual void set_policy_throttlers(int type, Throttle* bytes, Throttle* msgs = NULL) = 0;

	// 开始连接
	virtual Connection* get_connection(const entity_inst_t& dest) = 0;

	virtual Connection* get_loopback_connection() = 0;


	
	virtual void ready() {}

	void set_default_send_priority(int p) { _default_send_priority = p; }

	void set_socket_priority(int prio) { _socket_priority = prio; }

	int get_socket_priority() { return _socket_priority; }

	const entity_inst_t& get_entity() { return _entity; }

	void set_entity(entity_inst_t e) { _entity = e; }

	uint32_t get_magic() { return _magic; }
	
	void set_magic(uint32_t magic) { _magic = magic; }

	const entity_addr_t& get_entity_addr() { return _entity._addr; }

	const entity_name_t& get_entity_name() { return _entity._name; }

	void set_entity_name(const entity_name_t m) { _entity._name = m; }

	int get_default_send_priority() { return _default_send_priority; }


	void add_dispatcher_head(Dispatcher* d)
	{ 
		bool first = _dispatchers.empty();
		_dispatchers.push_front(d);
		if (d->ms_can_fast_dispatch_any())
			_fast_dispatchers.push_front(d);
		if (first)
			ready();
	}

	void add_dispatcher_tail(Dispatcher* d)
	{ 
		bool first = _dispatchers.empty();
		_dispatchers.push_back(d);
		if (d->ms_can_fast_dispatch_any())
			_fast_dispatchers.push_back(d);
		if (first)
			ready();
	}

	bool ms_can_fast_dispatch(Message* m)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_can_fast_dispatch(m))
				return true;
		}
			
		return false;
	}

	void ms_fast_dispatch(Message* m)
	{
		m->set_dispatch_stamp(clock_now());
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_can_fast_dispatch(m))
			{
				(*iter)->ms_fast_dispatch(m);
				return;
			}
		}
	}

	void ms_fast_preprocess(Message* m)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_fast_preprocess(m);
		}
	}

	void ms_deliver_dispatch(Message* m)
	{
		m->set_dispatch_stamp(clock_now());
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin(); 
			iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_dispatch(m))
				return;
		}
		m->dec();
	}


	void ms_deliver_handle_connect(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_connect(con);
		}
	}

	void ms_deliver_handle_fast_connect(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_fast_connect(con);
		}
	}

	void ms_deliver_handle_accept(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_accept(con);
		}
	}

	void ms_deliver_handle_fast_accept(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_fast_accept(con);
		}
	}

	void ms_deliver_handle_reset(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_handle_reset(con))
				return;
		}
	}

	void ms_deliver_handle_remote_reset(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_remote_reset(con);
		}
	}

	void ms_deliver_handle_refused(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_handle_refused(con))
			{
				return;
			}
		}
	}

	/*
	AuthAuthorizer* ms_deliver_get_authorizer(int peer_type, bool force_new)
	{
		AuthAuthorizer* auth = 0;
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin(); iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_get_authorizer(peer_type, &auth, force_new))
			{
				return auth;
			}
		}
		
		return NULL;
	}

	bool ms_deliver_verify_authorizer(Connection* con, int peer_type, int protocol, buffer& authorizer, buffer& authorizer_reply, bool& isvalid, CryptoKey& session_key)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin(); iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_verify_authorizer(con, peer_type, protocol, authorizer, authorizer_reply, isvalid, session_key))
			{
				return true;
			}
		}
		
		return false;
	}
	*/

protected:
	virtual void set_entity_addr(const entity_addr_t& a) { _entity._addr = a; }

public:
	int _crc_flag;

protected:
	// 当前通信实体
	entity_inst_t _entity;

	// 是否已启动
	bool _started;

	uint32_t _magic;

	int _default_send_priority;
	int _socket_priority;

private:
	// 消息分发器
	std::list<Dispatcher*> _dispatchers;
	// 快速消息分发器
	std::list<Dispatcher*> _fast_dispatchers;
};



class PolicyMessenger : public Messenger
{
private:
	Mutex _lock;
	Policy _default_policy;
	std::map<int, Policy> _policy_map;

public:
	PolicyMessenger(entity_name_t name, std::string mname, uint64_t nonce) : Messenger(name)
	{
	}

	~PolicyMessenger()
	{
	}

	Policy get_policy(int t)
	{
		Mutex::Locker locker(_lock);
		std::map<int, Policy>::iterator iter = _policy_map.find(t);
		
		if (iter != _policy_map.end())
		{
			return iter->second;
		}
		else
		{
			return _default_policy;
		}
	}

	Policy get_default_policy()
	{
		Mutex::Locker locker(_lock);
		return _default_policy;
	}
	
	void set_default_policy(Policy p)
	{
		Mutex::Locker locker(_lock);
		_default_policy = p;
	}

	void set_policy(int type, Policy p)
	{
		Mutex::Locker locker(_lock);
		_policy_map[type] = p;
	}

	void set_policy_throttlers(int type, Throttle* byte_throttle, Throttle* msg_throttle)
	{
    	Mutex::Locker locker(_lock);
		std::map<int, Policy>::iterator iter = _policy_map.find(type);
		
		if (iter != _policy_map.end())
		{
			iter->second._throttler_bytes = byte_throttle;
			iter->second._throttler_messages = msg_throttle;
		}
		else
		{
			_default_policy._throttler_bytes = byte_throttle;
			_default_policy._throttler_messages = msg_throttle;
		}
	}

};


#endif
