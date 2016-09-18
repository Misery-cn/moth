#include <map>
#include "mutex.h"
#include "cond.h"
#include "thread.h"
#include "datetime_utils.h"

// 自定义类型
typedef  void (*callback)(void);

class CTimerThread;

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
	bool stopping;
	CMutex lock_;
	CCond cond_;
	CTimerThread* thread_;
	std::multimap<int64_t, callback> schedule_;
	std::map<callback, std::multimap<int64_t, callback>::iterator> events_;
};
