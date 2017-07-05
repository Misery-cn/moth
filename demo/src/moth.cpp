#include <iostream>
#include "messenger.h"
#include "forker.h"
#include "master.h"
#include "global.h"

Master* master = NULL;

int main()
{
	int err = 0;

	global_init();
	
	Forker forker;
	
	std::string err_msg;
	err = forker.fork(err_msg);
	if (0 > err)
	{
		std::cerr << err_msg << std::endl;
        forker.exit(err);
	}
	
	if (forker.is_parent())
	{
		err = forker.parent_wait(err_msg);
		if (0 > err)
		{
			std::cerr << err_msg << std::endl;
		}
		
		forker.exit(err);
	}

	std::string straddr;
	sconfig.get_val("global", "addr", straddr);
	INFO_LOG("addr is %s", straddr.c_str());
	entity_addr_t addr;
	
	
	Messenger* msgr = Messenger::create("simple", entity_name_t::MASTER(0), "master", 0);
	RUNLOG(Log_Info, "create messenger succssful");
	
	
	forker.daemonize();
	
	while (true) {}
	
	return 0;
}
