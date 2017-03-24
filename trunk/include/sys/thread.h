
#ifndef _SYS_THREAD_H_
#define _SYS_THREAD_H_

#include <pthread.h>
#include "cond.h"

// SYS_NS_BEGIN

class CThread
{
public:
    // 获取当前线程号
    static uint32_t get_current_thread_id() throw ();

public:
	CThread() throw (CException, CSysCallException);
	virtual ~CThread() throw ();
	
	// 由派生类实现
	// 不能直接调用run,不能直接调用,应该调用start启动线程
	virtual void run() = 0;

    // 将stop_成员设置为true，线程可以根据_stop状态来决定是否退出线程      
    // wait_stop: 是否等待线程结束，只有当线程是可Join时才有效
    virtual void stop(bool wait_stop = true) throw (CException, CSysCallException);

	// 默认启动一个可join的线程
    // detach: 是否以可分离模式启动线程
	void start(bool detach = false) throw (CException, CSysCallException);

    // 设置线程栈大小。应当在start之前调用，否则设置无效，如放在before_start当中
    void set_stack_size(uint32_t stack_size) { _stack_size = stack_size; }
    
    // 得到线程栈大小字节数
    size_t get_stack_size() const throw (CSysCallException);

    // 得到本线程号
    uint32_t get_thread_id() const { return _thread; }
    
    // 等待线程返回
    void join() throw (CSysCallException);

    // 将线程设置为可分离的
    void detach() throw (CSysCallException);

    // 返回线程是否可join
    bool can_join() const throw (CSysCallException);

    // 如果线程正处于等待状态，则唤醒
    void wakeup();

	// 终止
	int kill(int signal);
	
	// 绑定cpu核
	int set_affinity(int cpuid);

	// 是否已启动
	bool is_started() const { return 0 != _thread; }
	
	// 是否是当前线程
	bool am_self() const { return pthread_self() == _thread; }
	
protected:

	// 判断线程是否应当退出，默认返回stop_的值
    virtual bool is_stop() const;

	// 线程可以调用它进入睡眠状态，并且可以通过调用do_wakeup唤醒
    void do_sleep(int milliseconds);

private:
	// 线程执行函数
	static void* thread_proc(void* thread_param);
	
	// 唤醒线程
    void do_wakeup(bool stop);
	
	// 线程启动前的操作
	// 由派生类实现
    virtual void before_start() {}

	// 线程结束前的操作
	// 由派生类实现
    virtual void before_stop() {}

protected:
	// 互斥锁
    CMutex _lock;

private:
	// 条件变量
    CCond _cond;
	// 是否停止线程标识
    volatile bool _stop;
	// 线程状态
    volatile enum 
	{
		state_sleeping, 
		state_wakeuped, 
		state_running
	} _state;
	
	// 线程id
    pthread_t _thread;
    pthread_attr_t _attr;
	// 栈大小
    uint32_t _stack_size;    
};

// SYS_NS_END

#endif