#include <map>
#include "mutex.h"
#include "cond.h"
#include "thread.h"
#include "time_utils.h"
#include "callback.h"

class TimerThread;

// 定时器类
class Timer
{
public:
	Timer(){};
	~Timer(){};
	
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
	std::multimap<utime_t, Callback*> _schedule;
	std::map<Callback*, std::multimap<utime_t, Callback*>::iterator> _events;
};
