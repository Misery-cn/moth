#ifndef _REF_COUNTABLE_H_
#define _REF_COUNTABLE_H_

#include "atomic.h"

// 引用计数基类
class CRefCountable
{
public:
	CRefCountable() { atomic_set(&_ref, 0); }

	virtual ~CRefCountable() {}

	// 获取当前引用数
	int get_ref() const { return atomic_read(&_ref); }

	// +1
	void inc() { atomic_inc(&_ref); }

	// -1
	bool dec()
	{
		volatile bool deleted = false;
		// 减一如果等于0返回true
		if (atomic_dec_and_test(&_ref))
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
		inc();
		
		return this;
	}
	
private:
	atomic_t _ref;
};

#endif
