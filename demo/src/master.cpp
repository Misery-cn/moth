#include "master.h"

Master::Master(Messenger* msgr, MasterMap* mmap) : _msgr(msgr), _master_map(mmap)
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
	
}

int Master::init()
{
	DEBUG_LOG("master init");

	_timer.init();
	new_tick();
	
	// 给messenger添加dispatcher
	// 同时启动dispatch_queue和accpter线程
	_msgr->add_dispatcher_tail(this);
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
