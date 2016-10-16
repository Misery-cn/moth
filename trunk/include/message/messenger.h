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

	CMessenger(entity_name_t entityname) : entity_(), magic_(0)
	{
		entity_.name_ = entityname;
	}
	
	virtual ~CMessenger() {};

	// 
	static CMessenger* create(std::string type, entity_name_t name, std::string lname, uint64_t nonce = 0, uint64_t features = 0);

	const entity_inst_t& get_entity() { return entity_; }

	void set_entity(entity_inst_t e) { entity_ = e; }

	uint32_t get_magic() { return magic_; }
	
	void set_magic(uint32_t magic) { magic_ = magic; }

	const entity_addr_t& get_entity_addr() { return entity_.addr_; }

	const entity_name_t& get_entity_name() { return entity_.name_; }

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
	virtual void set_entity_addr(const entity_addr_t& a) { entity_.addr_ = a; }

public:
	int crc_flag_;

protected:
	// 当前通信实体
	entity_inst_t entity_;

	// 是否已启动
	bool started_;

	uint32_t magic_;

private:
	// 消息分发
	std::list<CDispatcher*> dispatchers_;
	// 快速消息分发
	std::list<CDispatcher*> fast_dispatchers_;
};

#endif
