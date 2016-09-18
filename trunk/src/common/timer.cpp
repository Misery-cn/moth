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
	stopping = false;
	thread_ = new CTimerThread(this);
	thread_->start();
}

void CTimer::shutdown()
{
	if (thread_)
	{
		cancel_all_events();
		stopping = true;
		cond_.signal();
		lock_.unlock();
		thread_->join();
		lock_.lock();
		
		delete thread_;
		thread_ = NULL;
	}
}

void CTimer::timer_thread()
{
	lock_.lock();

	while (!stopping)
	{
		int64_t now = CDatetimeUtils::get_current_microseconds();
		// 
		while (!schedule_.empty())
		{
			scheduled_map_t::iterator p = schedule_.begin();
			// 时间还没到
			if (p->first > now)
			{
				break;
			}

			callback cb = p->second;
			events_.erase(cb);
			schedule_.erase(p);
			
			// 执行
			(*cb)();
		}

		if (stopping)
		{
			break;
		}

		if (schedule_.empty())
		{
			cond_.wait(lock_);
		}
		else
		{
			cond_.timed_wait(lock_, schedule_.begin()->first - now);
		}
	}
	
	lock_.unlock();
}

void CTimer::add_event_after(int64_t seconds, callback cb)
{
	int64_t when = CDatetimeUtils::get_current_microseconds();
	when += seconds;
	add_event_at(when, cb);
}

void CTimer::add_event_at(int64_t when, callback cb)
{
	scheduled_map_t::value_type s_val(when, cb);
	scheduled_map_t::iterator i = schedule_.insert(s_val);

	event_lookup_map_t::value_type e_val(cb, i);
	std::pair<event_lookup_map_t::iterator, bool> rval(events_.insert(e_val));

	if (i == schedule_.begin())
	{
		cond_.signal();
	}
}

bool CTimer::cancel_event(callback cb)
{
	std::map<callback, std::multimap<int64_t, callback>::iterator>::iterator p = events_.find(cb);
	if (p == events_.end())
	{
		return false;
	}

	// delete p->first;

	schedule_.erase(p->second);
	events_.erase(p);
	
	return true;
}

void CTimer::cancel_all_events()
{
	while (!events_.empty())
	{
		std::map<callback, std::multimap<int64_t, callback>::iterator>::iterator p = events_.begin();
		// delete p->first;
		schedule_.erase(p->second);
		events_.erase(p);
	}
}
