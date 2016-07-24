#include <pthread.h>
#include "cond.h"

class CThread
{
public:
	// 线程执行函数
	static void* thread_proc(void* thread_param);
    // 获取当前线程号
    static unsigned int get_current_thread_id();

public:
	CThread();
	virtual ~CThread();
	
	// 由派生类实现
	// 不能直接调用run,不能直接调用,应该调用start启动线程
	virtual void run() = 0;
    
	// 线程启动前的操作
	// 由派生类实现
    virtual bool before_start() { return true; }

	// 线程结束前的操作
	// 由派生类实现
    virtual void before_stop() {}

    // 将stop_成员设置为true，线程可以根据_stop状态来决定是否退出线程      
    // wait_stop: 是否等待线程结束，只有当线程是可Join时才有效
    virtual void stop(bool wait_stop = true);

	// 默认启动一个可join的线程
    // detach: 是否以可分离模式启动线程
	void start(bool detach = false);

    // 设置线程栈大小。应当在start之前调用，否则设置无效，如放在before_start当中
    void set_stack_size(size_t stack_size) { stack_size_ = stack_size; }
    
    // 得到线程栈大小字节数
    size_t get_stack_size() const;

    // 得到本线程号
    unsigned int get_thread_id() const { return thread_; }
    
    // 等待线程返回
    void join();

    // 将线程设置为可分离的
    void detach();

    // 返回线程是否可join
    bool can_join() const;

    // 如果线程正处于等待状态，则唤醒
    void wakeup();

protected:

	// 判断线程是否应当退出，默认返回stop_的值
    virtual bool is_stop() const;

	// 线程可以调用它进入睡眠状态，并且可以通过调用do_wakeup唤醒
    void do_sleep(int milliseconds);

private:
	// 唤醒线程
    void do_wakeup(bool stop);

protected:
	// 互斥锁
    CMutex lock_;

private:
	// 条件变量
    CCond cond_;
	// 是否停止线程标识
    volatile bool stop_;
	// 线程状态
    volatile enum 
	{
		state_sleeping, 
		state_wakeuped, 
		state_running
	} state_;
	
	// 线程id
    pthread_t thread_;
    pthread_attr_t attr_;
	// 栈大小
    size_t stack_size_;    
};