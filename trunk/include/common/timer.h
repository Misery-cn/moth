#include <map>
#include <set>
#include "mutex.h"
#include "cond.h"
#include "thread.h"
#include "time_utils.h"
#include "callback.h"

struct TimerJob
{
	TimerJob() : _once(false)
	{
		
	}
	
	~TimerJob()
	{
	}
	
	bool operator>(const TimerJob& other)
	{
		return this->_time > other._time;
	}
	
	bool operator<(const TimerJob& other)
	{
		return this->_time < other._time;
	}
	
	bool operator>=(const TimerJob& other)
	{
		return !operator<(other);
	}
	
	bool operator<=(const TimerJob& other)
	{
		return !operator>(other);
	}
	
	bool operator==(const TimerJob& other)
	{
		return this->_time == other._time;
	}
	
	bool operator!=(const TimerJob& other)
	{
		return this->_time != other._time;
	}
	
public:
	// 只执行一次
	bool _once;
	// 执行间隔
	uint64_t _interval;
	// 执行时间
	utime_t _time;
	// 回掉函数
	Callback* _callback;
};

class TimerThread;

// 定时器类
class Timer
{
public:
	Timer() : _stopping(false), _lock(), _cond(), _thread(NULL)
	{
	}
	
	~Timer() 
	{
	}
	
	void init();
	
	void shutdown();
	
	// 延迟执行
	void add_event_after(int64_t seconds, Callback* cb);
	
	void add_event_at(utime_t when, Callback* cb);
	
	bool cancel_event(Callback* cb);
	
	void cancel_all_events();
	
private:
	friend class TimerThread;
	
	void timer_thread();
	
private:
	bool _stopping;
	Mutex _lock;
	Cond _cond;
	TimerThread* _thread;
	
	/*
	std::multiset<TimerJob*> _schedule;
	std::map<Callback*, std::multiset<TimerJob*>::iterator> _events;
	
	typedef std::multiset<TimerJob*>::iterator schedule_iter;
	typedef std::map<Callback*, schedule_iter>::iterator event_iter;
	*/
	
	std::multimap<utime_t, Callback*> _schedule;
	std::map<Callback*, std::multimap<utime_t, Callback*>::iterator> _events;
	
	typedef std::multimap<utime_t, Callback*>::iterator schedule_iter;
	typedef std::map<Callback*, schedule_iter>::iterator event_iter;
};
