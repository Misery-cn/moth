#include "messenger.h"
#include "simple_messenger.h"

Messenger* Messenger::create(const std::string type, entity_name_t name, std::string lname, uint64_t nonce, uint64_t cflags)
{
	if (type == "simple")
	{
		return new SimpleMessenger(name, std::move(lname), nonce);
	}
	else if (type.find("async") != std::string::npos)
	{
		// return new AsyncMessenger(cct, name, type, std::move(lname), nonce);
		return NULL;
	}
	
	return NULL;
}

int Messenger::get_default_crc_flags()
{

}

