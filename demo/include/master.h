#ifndef _MASTER_H_
#define _MASTER_H_

#include "msg_types.h"
#include "dispatcher.h"
#include "messenger.h"
#include "mastermap.h"
#include "timer.h"
#include "callback.h"
#include "thread_pool.h"

class Operation
{
public:
	Operation() {};
	virtual ~Operation() {};

	void op()
	{
		DEBUG_LOG("Operation::op");
	}
};

class Master : public Dispatcher
{
public:
	Master(Messenger* msgr, MasterMap* mmap, uint32_t rank);
	virtual ~Master();
	
	virtual bool ms_dispatch(Message* m);
	
	virtual bool ms_handle_reset(Connection *con);
	
	virtual void ms_handle_remote_reset(Connection* con);
	
	virtual bool ms_handle_refused(Connection* con);
	
	int init();

	void tick();
	
	void boot();

public:

	class OPWorkQueue : public ThreadPool::WorkQueue<Operation>
	{
	public:
		OPWorkQueue(ThreadPool* p) : ThreadPool::WorkQueue<Operation>(p)
		{}
		
		virtual ~OPWorkQueue() {}

		virtual void _clear()
		{
			_ops.clear();
		}

		virtual bool _empty()
		{
			return _ops.empty();
		}

		virtual void _process(Operation* op);
		// 入列
	    virtual bool _enqueue(Operation* op);
		// 出列
	    virtual void _dequeue(Operation* op);
		// 出列
	    virtual Operation* _dequeue();

	private:
		std::list<Operation*> _ops;
	};

private:
	
	enum
	{
		STATE_PROBING = 1,
		STATE_SYNCHRONIZING,
		STATE_ELECTING,
		STATE_LEADER,
		STATE_PEON,
		STATE_SHUTDOWN
	};
	
	int _state;

	void new_tick();
	// tick
	class MasterTick : public Callback
	{
	public:
		explicit MasterTick(Master* m) : _master(m)
		{
		
		}

		virtual void finish(int r)
		{
			_master->tick();
		}
		
		virtual ~MasterTick() {}

	private:
		Master* _master;
	};
	
private:
	
	Messenger* _msgr;

	// 保存配置的master信息
	MasterMap* _master_map;

	// 定时器
	Timer _timer;

	uint32_t _rank;

	bool _has_ever_joined;

	ThreadPool _op_pool;

	OPWorkQueue _op_wq;
};

#endif