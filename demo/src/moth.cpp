#include <iostream>
#include "argparse.h"
#include "messenger.h"
#include "forker.h"
#include "master.h"
#include "mastermap.h"
#include "global.h"

Master* master = NULL;

void usage(bool et = true)
{
    std::cerr << "usage: moth -i id" << std::endl;

    // 是否直接退出
    if (et)
    {
        exit(1);
    }
}

int main(int argc, const char* argv[])
{
    std::vector<const char*> args;
    argv_to_vec(argc, argv, args);

    int whoami = -1;
    for (unsigned i = 0; i < args.size(); i++)
    {
        if (0 == strcmp(args[i], "-i"))
        {
            whoami = atoi(args[++i]);
        }
    }
    
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

    MasterMap master_map;
    master_map.build_initial();

    entity_addr_t addr;

    if (!master_map.contains(whoami))
    {
        std::cerr << "master id no exits" << std::endl;
        forker.exit(1);
    }
    else
    {
        addr = master_map.get_addr_by_rank(whoami);
    }
    
    Messenger* msgr = Messenger::create("simple", entity_name_t::MASTER(whoami), "master");

    // 绑定端口开始监听
    err = msgr->bind(addr);
    if (err < 0)
    {
        sockaddr_in* addr_in = (struct sockaddr_in*)addr.get_sockaddr();
        std::cerr << "unable to bind to " << inet_ntoa(addr_in->sin_addr) << std::endl;
        forker.exit(1);
    }
    
    master = new Master(msgr, &master_map, whoami);
    
    forker.daemonize();

    msgr->set_policy(entity_name_t::TYPE_CLIENT, Messenger::Policy::stateless_server());
    msgr->set_policy(entity_name_t::TYPE_SERVER, Messenger::Policy::lossless_peer_reuse());
    msgr->set_policy(entity_name_t::TYPE_MASTER, Messenger::Policy::lossless_peer_reuse());
    msgr->set_policy(entity_name_t::TYPE_SLAVE, Messenger::Policy::stateless_server());
    
    msgr->start();
    
    master->init();
    
    msgr->wait();
    
    return 0;
}
