#ifndef _THROTTLE_H_
#define _THROTTLE_H_

#include <list>
#include "atomic.h"
#include "cond.h"

class Throttle
{
public:
	Throttle(const std::string& n, int64_t m = 0);
	virtual ~Throttle();

private:
	void _reset_max(int64_t m);
	
	bool _should_wait(int64_t c) const
	{
		int64_t max = atomic_read(&_max);
		int64_t cur = atomic_read(&_count);

		return max && ((c <= max && cur + c > max) || (c >= max && cur > max));
	}

	bool _wait(int64_t c);

public:
	int64_t get_current() const
	{
		return atomic_read(&_count);
	}

	int64_t get_max() const
	{
		return atomic_read(&_max);
	}

	// ÊÇ·ñ¹ýÒ»°ë
	bool past_midpoint() const
	{
    	return atomic_read(&_count) >= atomic_read(&_max) / 2;
	}

	bool wait(int64_t m = 0);

	int64_t take(int64_t c = 1);
	
	bool get(int64_t c = 1, int64_t m = 0);

	bool get_or_fail(int64_t c = 1);

	int64_t put(int64_t c = 1);
	
	void reset();

	bool should_wait(int64_t c) const
	{
		return _should_wait(c);
	}

	
	void reset_max(int64_t m)
	{
		Mutex::Locker locker(_lock);
		_reset_max(m);
	}

private:
	const std::string _name;
	atomic_t _count;
	atomic_t _max;
	Mutex _lock;
	std::list<Cond*> _conds;
};

#endif
