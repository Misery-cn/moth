#include "timer.h"
#include "log.h"

typedef std::multimap<utime_t, Callback*> scheduled_map_t;
typedef std::map<Callback*, scheduled_map_t::iterator> event_lookup_map_t;

class TimerThread : public Thread
{
public:
	TimerThread(Timer* t) : _timer(t)
	{
	}
	
	void entry()
	{
		_timer->timer_thread();
		return;
	}
	
private:
	Timer* _timer;
};

void Timer::init()
{
	_stopping = false;
	_thread = new TimerThread(this);
	_thread->create();
}

void Timer::shutdown()
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

void Timer::timer_thread()
{
	_lock.lock();

	DEBUG_LOG("timer thread starting");

	while (!_stopping)
	{
		utime_t now = clock_now();

		while (!_schedule.empty())
		{
			scheduled_map_t::iterator p = _schedule.begin();

			if (p->first > now)
			{
				break;
			}

			Callback* cb = p->second;
			_events.erase(cb);
			_schedule.erase(p);
			
			// 执行回调函数
			cb->complete(0);
		}

		// 停止
		if (_stopping)
		{
			break;
		}

		// 挂起
		if (_schedule.empty())
		{
			_cond.wait(_lock);
		}
		else
		{
			// 注意需要转成毫秒
			_cond.timed_wait(_lock, (_schedule.begin()->first - now).to_msec());
		}

		DEBUG_LOG("timer thread awake");
	}
	
	_lock.unlock();
}

void Timer::add_event_after(int64_t seconds, Callback* cb)
{
	utime_t when = clock_now();
	when += seconds;
	add_event_at(when, cb);
}

void Timer::add_event_at(utime_t when, Callback* cb)
{
	scheduled_map_t::value_type s_val(when, cb);
	scheduled_map_t::iterator i = _schedule.insert(s_val);

	event_lookup_map_t::value_type e_val(cb, i);
	std::pair<event_lookup_map_t::iterator, bool> rval(_events.insert(e_val));

	// 如果只有一个任务待执行,则需要唤醒执行线程
	// 如果有多个任务,执行线程只会time_wait
	if (i == _schedule.begin())
	{
		_cond.signal();
	}
}

bool Timer::cancel_event(Callback* cb)
{
	std::map<Callback*, std::multimap<utime_t, Callback*>::iterator>::iterator p = _events.find(cb);
	if (p == _events.end())
	{
		return false;
	}

	// delete p->first;

	_schedule.erase(p->second);
	_events.erase(p);
	
	return true;
}

void Timer::cancel_all_events()
{
	while (!_events.empty())
	{
		std::map<Callback*, std::multimap<utime_t, Callback*>::iterator>::iterator p = _events.begin();
		// delete p->first;
		_schedule.erase(p->second);
		_events.erase(p);
	}
}
