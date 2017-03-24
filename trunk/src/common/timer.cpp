#include "timer.h"

typedef std::multimap<int64_t, callback> scheduled_map_t;
typedef std::map<callback, scheduled_map_t::iterator> event_lookup_map_t;

class CTimerThread : public CThread
{
public:
	CTimerThread(CTimer* t) : timer(t)
	{
	}
	
	void run()
	{
		timer->timer_thread();
		return;
	}
	
private:
	CTimer* timer;
};

void CTimer::init()
{
	_stopping = false;
	_thread = new CTimerThread(this);
	_thread->start();
}

void CTimer::shutdown()
{
	if (_thread)
	{
		cancel_all_events();
		_stopping = true;
		_cond.signal();
		_lock.unlock();
		_thread->join();
		_lock.lock();
		
		delete _thread;
		_thread = NULL;
	}
}

void CTimer::timer_thread()
{
	_lock.lock();

	while (!_stopping)
	{
		int64_t now = CTimeUtils::get_current_microseconds();
		// 
		while (!_schedule.empty())
		{
			scheduled_map_t::iterator p = _schedule.begin();
			// 时间还没到
			if (p->first > now)
			{
				break;
			}

			callback cb = p->second;
			_events.erase(cb);
			_schedule.erase(p);
			
			// 执行回调函数
			(*cb)();
		}

		if (_stopping)
		{
			break;
		}

		if (_schedule.empty())
		{
			_cond.wait(_lock);
		}
		else
		{
			_cond.timed_wait(_lock, _schedule.begin()->first - now);
		}
	}
	
	_lock.unlock();
}

void CTimer::add_event_after(int64_t seconds, callback cb)
{
	int64_t when = CTimeUtils::get_current_microseconds();
	when += seconds;
	add_event_at(when, cb);
}

void CTimer::add_event_at(int64_t when, callback cb)
{
	scheduled_map_t::value_type s_val(when, cb);
	scheduled_map_t::iterator i = _schedule.insert(s_val);

	event_lookup_map_t::value_type e_val(cb, i);
	std::pair<event_lookup_map_t::iterator, bool> rval(_events.insert(e_val));

	if (i == _schedule.begin())
	{
		_cond.signal();
	}
}

bool CTimer::cancel_event(callback cb)
{
	std::map<callback, std::multimap<int64_t, callback>::iterator>::iterator p = _events.find(cb);
	if (p == _events.end())
	{
		return false;
	}

	// delete p->first;

	_schedule.erase(p->second);
	_events.erase(p);
	
	return true;
}

void CTimer::cancel_all_events()
{
	while (!_events.empty())
	{
		std::map<callback, std::multimap<int64_t, callback>::iterator>::iterator p = _events.begin();
		// delete p->first;
		_schedule.erase(p->second);
		_events.erase(p);
	}
}
