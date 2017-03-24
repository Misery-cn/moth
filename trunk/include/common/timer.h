#include <map>
#include "mutex.h"
#include "cond.h"
#include "thread.h"
#include "datetime_utils.h"

// 回调函数类型
typedef  void (*callback)(void);

class CTimerThread;

// 定时器类
class CTimer
{
public:
	CTimer(){};
	~CTimer(){};
	
	void init();
	
	void shutdown();
	
	// 延迟执行
	void add_event_after(int64_t seconds, callback cb);
	
	void add_event_at(int64_t when, callback cb);
	
	bool cancel_event(callback cb);
	
	void cancel_all_events();
	
private:
	friend class CTimerThread;
	
	void timer_thread();
	
private:
	bool _stopping;
	CMutex _lock;
	CCond _cond;
	CTimerThread* _thread;
	std::multimap<int64_t, callback> _schedule;
	std::map<callback, std::multimap<int64_t, callback>::iterator> _events;
};
