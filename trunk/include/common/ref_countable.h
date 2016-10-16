#ifndef _REF_COUNTABLE_H_
#define _REF_COUNTABLE_H_

#include "atomic.h"

// 引用计数基类
class CRefCountable
{
public:
	CRefCountable() { atomic_set(&refcount_, 0); }

	virtual ~CRefCountable() {}

	// 获取当前引用数
	int get_refcount() const { return atomic_read(&refcount_); }

	// +1
	void inc_refcount() { atomic_inc(&refcount_); }

	// -1
	bool dec_refcount()
	{
		volatile bool deleted = false;
		// 减一如果等于0返回true
		if (atomic_dec_and_test(&refcount_))
		{
			deleted = true;
			delete this;
		}

		return deleted;
	}

	// 获取当前引用计数指针
	CRefCountable* get()
	{
		// +1
		inc_refcount();
		
		return this;
	}
	
private:
	atomic_t refcount_;
};

#endif
