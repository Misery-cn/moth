#include "message.h"
#include "dispatch_queue.h"
#include "simple_messenger.h"

double DispatchQueue::get_max_age(utime_t now) const
{
    Mutex::Locker locker(_lock);
    if (_marrival.empty())
    {
        return 0;
    }
    else
    {
        return (now - _marrival.begin()->first);
    }
}

uint64_t DispatchQueue::pre_dispatch(Message* m)
{
    uint64_t msize = m->get_dispatch_throttle_size();
    m->set_dispatch_throttle_size(0);
    return msize;
}

void DispatchQueue::post_dispatch(Message* m, uint64_t msize)
{
    dispatch_throttle_release(msize);
}

bool DispatchQueue::can_fast_dispatch(Message* m) const
{
    return _msgr->ms_can_fast_dispatch(m);
}

void DispatchQueue::fast_dispatch(Message* m)
{
    uint64_t msize = pre_dispatch(m);
    _msgr->ms_fast_dispatch(m);
    post_dispatch(m, msize);
}

void DispatchQueue::fast_preprocess(Message* m)
{
    _msgr->ms_fast_preprocess(m);
}

void DispatchQueue::enqueue(Message* m, int priority, uint64_t id)
{
    Mutex::Locker locker(_lock);
    add_arrival(m);
    if (priority >= MSG_PRIO_LOW)
    {
        _mqueue.enqueue_strict(id, priority, QueueItem(m));
    }
    else
    {
        _mqueue.enqueue(id, priority, m->get_cost(), QueueItem(m));
    }
    
    _cond.signal();
}

void DispatchQueue::local_delivery(Message* m, int priority)
{
    m->set_recv_stamp(clock_now());
    Mutex::Locker locker(_local_delivery_lock);
    if (_local_messages.empty())
    {
        _local_delivery_cond.signal();
    }
    
    _local_messages.push_back(std::make_pair(m, priority));
    
    return;
}

void DispatchQueue::run_local_delivery()
{
    _local_delivery_lock.lock();
    while (true)
    {
        if (_stop_local_delivery)
        {
            break;
        }
        
        if (_local_messages.empty())
        {
            _local_delivery_cond.wait(_local_delivery_lock);
            continue;
        }
        
        std::pair<Message*, int> mp = _local_messages.front();
        _local_messages.pop_front();
        _local_delivery_lock.unlock();
        Message* m = mp.first;
        int priority = mp.second;
        fast_preprocess(m);
        if (can_fast_dispatch(m))
        {
            fast_dispatch(m);
        }
        else
        {
            enqueue(m, priority, 0);
        }
        _local_delivery_lock.lock();
    }
    
    _local_delivery_lock.unlock();
}

void DispatchQueue::dispatch_throttle_release(uint64_t msize)
{
    if (msize)
    {
        _dispatch_throttler.put(msize);
    }
}

void DispatchQueue::entry()
{
    _lock.lock();
    while (true)
    {
        while (!_mqueue.empty())
        {
            QueueItem item = _mqueue.dequeue();
            if (!item.is_code())
            {
                remove_arrival(item.get_message());
            }
            
            _lock.unlock();

            if (item.is_code())
            {            
                switch (item.get_code())
                {
                    case D_BAD_REMOTE_RESET:
                        _msgr->ms_deliver_handle_remote_reset(item.get_connection());
                        break;
                    case D_CONNECT:
                        _msgr->ms_deliver_handle_connect(item.get_connection());
                        break;
                    case D_ACCEPT:
                        _msgr->ms_deliver_handle_accept(item.get_connection());
                        break;
                    case D_BAD_RESET:
                        _msgr->ms_deliver_handle_reset(item.get_connection());
                        break;
                    case D_CONN_REFUSED:
                        _msgr->ms_deliver_handle_refused(item.get_connection());
                        break;
                    default:
                        abort();
                }
            }
            else
            {
                Message* m = item.get_message();
                if (_stop)
                {
                    m->dec();
                }
                else
                {
                    uint64_t msize = pre_dispatch(m);
                    _msgr->ms_deliver_dispatch(m);
                    post_dispatch(m, msize);
                }
            }

            _lock.lock();
        }
        
        if (_stop)
        {
            break;
        }

        _cond.wait(_lock);
    }
    
    _lock.unlock();
}

void DispatchQueue::discard_queue(uint64_t id)
{
    Mutex::Locker locker(_lock);
    std::list<QueueItem> removed;
    _mqueue.remove_by_class(id, &removed);
    for (std::list<QueueItem>::iterator i = removed.begin(); i != removed.end(); ++i)
    {
        Message* m = i->get_message();
        remove_arrival(m);
        dispatch_throttle_release(m->get_dispatch_throttle_size());
        m->dec();
    }
}

void DispatchQueue::start()
{
    _dispatch_thread.create();
    _local_delivery_thread.create();
}

void DispatchQueue::wait()
{
    _local_delivery_thread.join();
    _dispatch_thread.join();
}

void DispatchQueue::discard_local()
{
    for (std::list<std::pair<Message*, int> >::iterator i = _local_messages.begin(); i != _local_messages.end(); ++i)
    {
        i->first->dec();
    }
    
    _local_messages.clear();
}

void DispatchQueue::shutdown()
{
    _local_delivery_lock.lock();
    _stop_local_delivery = true;
    _local_delivery_cond.signal();
    _local_delivery_lock.unlock();

    _lock.lock();
    _stop = true;
    _cond.signal();
    _lock.unlock();
}

