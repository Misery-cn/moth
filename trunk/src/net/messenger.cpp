#include "messenger.h"
#include "simple_messenger.h"

Messenger* Messenger::create(const std::string type, entity_name_t name, std::string lname)
{
    if (type == "simple")
    {
        return new SimpleMessenger(name, std::move(lname));
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

