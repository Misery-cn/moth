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
	entity_addr_t addr(straddr.c_str());
	
	Messenger* msgr = Messenger::create("simple", entity_name_t::MASTER(0), "master", 0);
	INFO_LOG("starting bind %s", straddr.c_str());
	err = msgr->bind(addr);
	if (err < 0)
	{
		std::cerr << "unable to bind to " << straddr << std::endl;
		forker.exit(1);
	}
	
	forker.daemonize();
	
	while (true) {}
	
	return 0;
}
