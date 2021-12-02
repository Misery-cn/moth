#include "async_messenger.h"

AsyncMessenger::AsyncMessenger(entity_name_t name, std :: string mname)
    : PolicyMessenger(name, mname),
    _dispatch_queue(this, mname),
{
    std::string transport_type = "posix";
    if (type.find("rdma") != std::string::npos)
    {
        transport_type = "rdma";
    }
    else if (type.find("dpdk") != std::string::npos)
    {
        transport_type = "dpdk";
    }

    
}