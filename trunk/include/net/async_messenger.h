#ifndef _ASYNC_MESSENGER_H_
#define _ASYNC_MESSENGER_H_

class AsyncMessenger : public PolicyMessenger
{
public:
    AsyncMessenger(entity_name_t name, std::string mname);
    
    virtual ~AsyncMessenger();
    
private:
    DispatchQueue _dispatch_queue;
};

#endif