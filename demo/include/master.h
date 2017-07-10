#ifndef _MASTER_H_
#define _MASTER_H_

#include "msg_types.h"
#include "dispatcher.h"
#include "messenger.h"
#include "mastermap.h"
#include "timer.h"
#include "callback.h"

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
};

#endif