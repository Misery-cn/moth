#include "throttle.h"
#include "time_utils.h"

Throttle::Throttle(const std::string& n, int64_t m) : _name(n), _max(m)
{
}

Throttle::~Throttle()
{
	while (!_conds.empty())
	{
		Cond* c = _conds.front();
		delete c;
		_conds.pop_front();
	}
}

void Throttle::_reset_max(int64_t m)
{
	if (atomic_read(&_max) == m)
	{
		return;
	}
	
	if (!_conds.empty())
	{
		_conds.front()->signal();
	}
	
	atomic_set(&_max, m);
}

bool Throttle::_wait(int64_t c)
{
	utime_t start;
	bool waited = false;
	
	if (_should_wait(c) || !_conds.empty())
	{
		Cond* cond = new Cond;
		_conds.push_back(cond);
		waited = true;

		do
		{
			cond->wait(_lock);
		} while (_should_wait(c) || cond != _conds.front());

		delete cond;
		_conds.pop_front();

		if (!_conds.empty())
		{
			_conds.front()->signal();
		}
	}
	
	return waited;
}

bool Throttle::wait(int64_t m)
{
	if (0 == atomic_read(&_max) && 0 == m)
	{
		return false;
	}

	Mutex::Locker locker(_lock);
	if (m)
	{
		_reset_max(m);
	}
	
	return _wait(0);
}

int64_t Throttle::take(int64_t c)
{
	if (0 == atomic_read(&_max))
	{
		return 0;
	}
	
	Mutex::Locker locker(_lock);
	atomic_add(c, &_count);
	
	return atomic_read(&_count);
}

bool Throttle::get(int64_t c, int64_t m)
{
	if (0 == atomic_read(&_max) && 0 == m)
	{
		return false;
	}

	bool waited = false;
	{
		Mutex::Locker locker(_lock);
		if (m)
		{
			_reset_max(m);
		}
		
		waited = _wait(c);
		
		atomic_add(c, &_count);
	}

	return waited;
}


bool Throttle::get_or_fail(int64_t c)
{
	if (0 == atomic_read(&_max))
	{
		return true;
	}

	Mutex::Locker locker(_lock);
	if (_should_wait(c) || !_conds.empty())
	{
		return false;
	}
	else
	{
		atomic_add(c, &_count);
		return true;
	}
}

int64_t Throttle::put(int64_t c)
{
	if (0 == atomic_read(&_max))
	{
		return 0;
	}

	Mutex::Locker locker(_lock);
	if (c)
	{
		if (!_conds.empty())
		{
			_conds.front()->signal();
		}
		
		atomic_sub(c, &_count);
	}
	
	return atomic_read(&_count);
}

void Throttle::reset()
{
	Mutex::Locker locker(_lock);
	
	if (!_conds.empty())
	{
		_conds.front()->signal();
	}
	
	atomic_set(&_count, 0);
}

