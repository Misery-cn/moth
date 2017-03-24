#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#include <list>
#include <set>
#include "msg_types.h"
#include "message.h"
#include "dispatcher.h"

// 通信基类
class CMessenger
{
public:
	CMessenger();

	CMessenger(entity_name_t entityname) : _entity(), _magic(0)
	{
		_entity.name_ = entityname;
	}
	
	virtual ~CMessenger() {};

	static CMessenger* create(std::string type, entity_name_t name, std::string lname, uint64_t nonce = 0, uint64_t features = 0);

	const entity_inst_t& get_entity() { return _entity; }

	void set_entity(entity_inst_t e) { _entity = e; }

	uint32_t get_magic() { return _magic; }
	
	void set_magic(uint32_t magic) { _magic = magic; }

	const entity_addr_t& get_entity_addr() { return _entity.addr_; }

	const entity_name_t& get_entity_name() { return _entity.name_; }

	struct Policy
	{
		bool lossy;
		bool server;
		bool standby;
		bool resetcheck;
	};

	// 绑定地址,由派生类实现
	virtual int bind(const entity_addr_t& bind_addr) = 0;

	// 重新绑定端口,并且不能与之前绑定的端口冲突
	virtual int rebind(const std::set<int>& avoid_ports) { return -EOPNOTSUPP; }
	// 启动
	virtual int start() { started_ = true; return 0; }
	// 停止
	virtual int shutdown() { started_ = false; return 0; }
	// 阻塞等待关闭
	virtual void wait() = 0;
	// 发送消息
	virtual int send_message(Message* msg, const entity_inst_t& dest) = 0;
	// 标记连接断开
	virtual void mark_down(const entity_addr_t& addr) = 0;

protected:
	virtual void set_entity_addr(const entity_addr_t& a) { _entity.addr_ = a; }

public:
	int _crc_flag;

protected:
	// 当前通信实体
	entity_inst_t _entity;

	// 是否已启动
	bool _started;

	uint32_t _magic;

private:
	// 消息分发器
	std::list<CDispatcher*> _dispatchers;
	// 快速消息分发器
	std::list<CDispatcher*> _fast_dispatchers;
};

#endif
