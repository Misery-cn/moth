#include "thread.h"


// SYS_NS_BEGIN


CThread::CThread() throw (CException, CSysCallException)
	// 创建一个递归锁
    : lock_(true), stop_(false), state_(state_sleeping), stack_size_(0)
{
    int r = pthread_attr_init(&attr_);
    if (0 != r)
    {
        // RUNLOG or export exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_init");
    }
}

CThread::~CThread() throw ()
{
	pthread_attr_destroy(&attr_);
}

void* CThread::thread_proc(void* thread_param)
{
    CThread* thread = (CThread *)thread_param;
	// go!
    thread->run();
	
    return NULL;
}

uint32_t CThread::get_current_thread_id() throw ()
{
    return pthread_self();
}

void CThread::start(bool detach) throw (CException, CSysCallException)
{
    before_start();

    int r = 0;

    // 设置线程栈大小
    if (0 < stack_size_)
	{
		r = pthread_attr_setstacksize(&attr_, stack_size_);
		if (0 != r)
		{
			THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_setstacksize");
		}
	}
        
	// 设置线程运行方式
	r = pthread_attr_setdetachstate(&attr_, detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
	if (0 != r)
	{
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_setdetachstate");
	}
       
	// 创建线程
    r = pthread_create(&thread_, &attr_, thread_proc, this);
	if (0 != r)
	{
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_create");
	}
}

size_t CThread::get_stack_size() const throw (CSysCallException)
{
    size_t stack_size = 0;
    int r = pthread_attr_getstacksize(&attr_, &stack_size);
    if (0 != r)
	{
		// RUNLOG or export exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_getstacksize");
	}

    return stack_size;
}

void CThread::join() throw (CSysCallException)
{
    // 线程自己不能调用join
    if (CThread::get_current_thread_id() != this->get_thread_id())
    {
    	if (0 < thread_)
		{
			int r = pthread_join(thread_, NULL);
	        if (0 != r)
			{
				// RUNLOG or export exception
				THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_join");
			}
		}
    }
}

void CThread::detach() throw (CSysCallException)
{
    int r = pthread_detach(thread_);
    if (0 != r)
	{
		// RUNLOG or export exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_detach");
	}
}

bool CThread::can_join() const throw (CSysCallException)
{
    int state;
    int r = pthread_attr_getdetachstate(&attr_, &state);
    if (0 != r)
	{
		// RUNLOG or export exception
		THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_getdetachstate");
	}

    return (PTHREAD_CREATE_JOINABLE == state);
}

bool CThread::is_stop() const
{
    return stop_;
}

void CThread::do_wakeup(bool stop)
{   
    // 线程终止标识
    if (stop)
	{
		stop_ = stop;
	}
    
    // 保证在唤醒线程之前，已经将它的状态修改为state_wakeup
    if (state_sleeping == state_)
    {
        state_ = state_wakeuped;
        cond_.signal();  
    }
    else
    {
        state_ = state_wakeuped;
    }
}

void CThread::wakeup()
{
    CMutexGuard g(lock_);
    do_wakeup(false);
}

void CThread::stop(bool wait_stop) throw (CException, CSysCallException)
{
    if (!stop_)
    {
        stop_ = true;
        before_stop();
        CMutexGuard g(lock_);
        do_wakeup(true);
    }
	
    if (wait_stop && can_join())
    {
        join();
    }
}

void CThread::do_sleep(int milliseconds)
{
    // 非本线程调用无效
    if (this->get_thread_id() == CThread::get_current_thread_id())
    {    
        CMutexGuard g(lock_);
        if (!is_stop())
        {
            if (state_ != state_wakeuped)
            {        
                state_ = state_sleeping;
                if (0 > milliseconds)
				{
                    cond_.wait(lock_);
				}
                else
				{
					cond_.timed_wait(lock_, milliseconds); 
				}               
            }

            // 不设置为state_wakeup，保证再次都可以调用do_millisleep
            state_ = state_running;
        }
    }
}

// SYS_NS_END
