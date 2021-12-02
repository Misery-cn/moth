#ifndef _NET_STACK_H_
#define _NET_STACK_H_

class Worker
{
};

class NetworkStack
{
public:
    NetworkStack();
    virtual ~NetworkStack();

protected:
    std::vector
private:
    unsigned _num_workers = 0;
    bool _started = false;
    SpinLock _spinlock;
};

#endif