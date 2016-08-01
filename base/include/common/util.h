#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define.h"
#include "const.h"

UTILS_NS_BEGIN


template<class Point>
class CPointGuard
{
public:
    CPointGuard(Point* p, bool is_arrary = false);
    virtual ~CPointGuard();
private:
    Point* point_;
	bool is_arrary_;
};

template<class Point>
CPointGuard<Point>::CPointGuard(Point * p, bool is_arrary)
{
	point_ = p;
	is_arrary_ = is_arrary;
}

template<class Point>
CPointGuard<Point>::~CPointGuard()
{
	if (!is_arrary_)
	{
		delete point_;
	}
	else
	{
		delete[] point_;
	}
	
	point_ = NULL;
}

template <typename DataType>
class CArrayList
{       
public:
    CArrayList(uint32_t list_max) : head_(0), tail_(0), list_size_(0)
    {
        list_max_ = list_max;
        if (0 == list_max_)
        {
            items_ = NULL;
        }
        else
        {
            items_ = new DataType[list_max_];        
            memset(items_, 0, list_max_);
        }
    }

    ~CArrayList()
    {
        delete []items_;
    }

	
    bool is_full() const 
    {
        return (list_max_ == list_size_);
    }
    
    bool is_empty() const 
    {
        return (0 == list_size_);
    }

    DataType front() const 
    {
        return items_[head_];
    }
    
    DataType pop_front() 
    {
        DataType item = items_[head_];
        head_ = (head_ + 1) % list_max_;
        --list_size_;
        return item;
    }

    void push_back(DataType item) 
    {
        items_[tail_] = item;
        tail_ = (tail_ + 1) % list_max_; 
        ++list_size_;
    }

    uint32_t size() const 
    {
        return list_size_;
    }
    
    uint32_t capacity() const
    {
        return list_max_;
    }

private:
	volatile uint32_t head_;
    volatile uint32_t tail_;
    volatile uint32_t list_size_;
    uint32_t list_max_;
    DataType* items_;
};


bool get_work_dir(char* buf);

UTILS_NS_END

#endif
