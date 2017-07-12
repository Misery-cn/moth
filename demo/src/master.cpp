#include "master.h"
#include "mprobe.h"

Master::Master(Messenger* msgr, MasterMap* mmap, uint32_t rank) : _msgr(msgr), _master_map(mmap), _rank(rank)
{

}

Master::~Master()
{
	
}

bool Master::ms_dispatch(Message* m)
{
	
}

bool Master::ms_handle_reset(Connection *con)
{
	
}

void Master::ms_handle_remote_reset(Connection* con)
{
	
}

bool Master::ms_handle_refused(Connection* con)
{
	DEBUG_LOG("connect refused");

	return false;
}

int Master::init()
{
	DEBUG_LOG("master init");

	_timer.init();
	// new_tick();
	
	// 给messenger添加dispatcher
	// 同时启动dispatch_queue和accpter线程
	_msgr->add_dispatcher_tail(this);
	
	boot();
}

void Master::new_tick()
{
	Master::MasterTick* mt = new Master::MasterTick(this);
	_timer.add_event_after(3, mt);
}

void Master::tick()
{
	DEBUG_LOG("master tick");

	// 再次添加定时回调函数
	new_tick();
}

void Master::boot()
{
	_state = STATE_PROBING;
	
	if (1 == _master_map->size())
	{
		return;
	}
	
	DEBUG_LOG("probing other master");
	
	for (uint32_t i = 0; i < _master_map->size(); i++)
	{
		if ((int)i != _rank)
		{
			_msgr->send_message(new MProbe(MProbe::OP_PROBE, _rank, _has_ever_joined), _master_map->get_entity(i));
		}
	}
}