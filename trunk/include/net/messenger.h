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
		
		Policy(bool l, bool s, bool st, bool r) : _lossy(l), _server(s), _standby(st), 
					_resetcheck(r), _throttler_bytes(NULL), _throttler_messages(NULL)
		{
			
		}

		static Policy stateful_server()
		{
			return Policy(false, true, true, true);
		}
		
		static Policy stateless_server()
		{
			return Policy(true, true, false, false);
		}
		
		static Policy lossless_peer()
		{
			return Policy(false, false, true, false);
		}
		
		static Policy lossless_peer_reuse()
		{
			return Policy(false, false, true, true);
		}
		
		static Policy lossy_client()
		{
			return Policy(true, false, false, false);
		}
		
		static Policy lossless_client()
		{
			return Policy(false, false, false, true);
		}
		
	};

	static Messenger* create(std::string type, entity_name_t name, std::string lname);

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
	// 关闭socket连接
	virtual void mark_down(const entity_addr_t& addr) = 0;
	// 关闭所有socket连接
	virtual void mark_down_all() = 0;
	// 获取消息转发队列大小
	virtual int get_dispatch_queue_len() = 0;
	// 获取消息转发队列中最大时间
	virtual double get_dispatch_queue_max_age(utime_t now) = 0;
	// 设置messenger默认策略
	virtual void set_default_policy(Policy p) = 0;
	// 获取messenger默认策略
	virtual Policy get_default_policy() = 0;
	// 设置messenger策略
	virtual void set_policy(int type, Policy p) = 0;
	// 获取messenger策略
	virtual Policy get_policy(int t) = 0;
	// 设置messenger节流阀值
	virtual void set_policy_throttlers(int type, Throttle* bytes, Throttle* msgs = NULL) = 0;
	// 获取socket连接，如果没有则开始连接
	virtual Connection* get_connection(const entity_inst_t& dest) = 0;
	// 获取本地连接
	virtual Connection* get_loopback_connection() = 0;
	// messenger准备就绪
	virtual void ready() {}
	// 设置messenger发送优先级
	void set_default_send_priority(int p) { _default_send_priority = p; }
	// 获取发送优先级
	int get_default_send_priority() { return _default_send_priority; }
	// 设置socket优先级IP_TOS
	void set_socket_priority(int prio) { _socket_priority = prio; }
	// 获取socket优先级
	int get_socket_priority() { return _socket_priority; }
	// 获取本地通信实体信息
	const entity_inst_t& get_entity() { return _entity; }
	// 设置本地通信实体信息
	void set_entity(entity_inst_t e) { _entity = e; }
	// 魔数字
	uint32_t get_magic() { return _magic; }
	// 设置魔数字
	void set_magic(uint32_t magic) { _magic = magic; }
	// 获取本地通信地址信息
	const entity_addr_t& get_entity_addr() { return _entity._addr; }
	// 获取本地通信实体名
	const entity_name_t& get_entity_name() { return _entity._name; }
	// 设置本地通信实体名
	void set_entity_name(const entity_name_t m) { _entity._name = m; }

	// 添加dispatcher到dispatchers队列头部
	void add_dispatcher_head(Dispatcher* d)
	{ 
		bool first = _dispatchers.empty();
		_dispatchers.push_front(d);
		if (d->ms_can_fast_dispatch_any())
		{
			_fast_dispatchers.push_front(d);
		}
		
		// 不为空则启动messenger
		if (first)
		{
			ready();
		}
	}
	
	// 添加dispatcher到dispatchers队列尾部
	void add_dispatcher_tail(Dispatcher* d)
	{ 
		bool first = _dispatchers.empty();
		_dispatchers.push_back(d);
		if (d->ms_can_fast_dispatch_any())
		{
			_fast_dispatchers.push_back(d);
		}
		
		// 不为空则启动messenger
		if (first)
		{
			ready();
		}
	}
	
	// 检查fast_dispatchers队列是否可以处理该消息
	bool ms_can_fast_dispatch(Message* m)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_can_fast_dispatch(m))
			{
				return true;
			}
		}
			
		return false;
	}
	
	// 处理收到的消息
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
	
	// 消息预处理
	void ms_fast_preprocess(Message* m)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_fast_preprocess(m);
		}
	}
	
	// 下发消息给dispatcher处理
	void ms_deliver_dispatch(Message* m)
	{
		m->set_dispatch_stamp(clock_now());
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin(); 
			iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_dispatch(m))
			{
				return;
			}
		}
		
		m->dec();
	}

	// 下发connect请求给dispatcher处理
	void ms_deliver_handle_connect(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_connect(con);
		}
	}
	
	// 下发connect请求给dispatcher处理
	void ms_deliver_handle_fast_connect(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_fast_connect(con);
		}
	}
	
	// 下发accept请求给dispatcher处理
	void ms_deliver_handle_accept(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_accept(con);
		}
	}
	
	// 下发accept请求给dispatcher处理
	void ms_deliver_handle_fast_accept(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _fast_dispatchers.begin();
			iter != _fast_dispatchers.end(); ++iter)
		{
			(*iter)->ms_handle_fast_accept(con);
		}
	}

	// 下发重置连接请求给dispatcher处理
	void ms_deliver_handle_reset(Connection* con)
	{
		for (std::list<Dispatcher*>::iterator iter = _dispatchers.begin();
			iter != _dispatchers.end(); ++iter)
		{
			if ((*iter)->ms_handle_reset(con))
			{
				return;
			}
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
	
	// 下发connect refused消息给dispatcher处理
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

protected:
	// 设置本地通信实体地址
	virtual void set_entity_addr(const entity_addr_t& a) { _entity._addr = a; }

public:
	int _crc_flag;

protected:
	// 当前通信实体
	entity_inst_t _entity;
	// 是否已启动
	bool _started;
	// 魔数字
	uint32_t _magic;
	// 发送优先级
	int _default_send_priority;
	// socket优先级
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
	// messenger默认策略
	Policy _default_policy;
	std::map<int, Policy> _policy_map;

public:
	PolicyMessenger(entity_name_t name, std::string mname) : Messenger(name)
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
