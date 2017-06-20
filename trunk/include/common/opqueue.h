#ifndef _OP_QUEUE_H_
#define _OP_QUEUE_H_

#include <functional>

template <typename T, typename K>
class OpQueue
{
public:
    virtual unsigned length() const = 0;

    virtual void remove_by_filter(std::function<bool (T)> f) = 0;
	
    virtual void remove_by_class(K k, std::list<T>* out) = 0;

    virtual void enqueue_strict(K cl, unsigned priority, T item) = 0;

    virtual void enqueue_strict_front(K cl, unsigned priority, T item) = 0;

    virtual void enqueue(K cl, unsigned priority, unsigned cost, T item) = 0;

    virtual void enqueue_front(K cl, unsigned priority, unsigned cost, T item) = 0;

    virtual bool empty() const = 0;

    virtual T dequeue() = 0;

    // virtual void dump(Formatter* f) const = 0;

    virtual ~OpQueue() {}; 
};

#endif