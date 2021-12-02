#include "thread.h"


// SYS_NS_BEGIN


Thread::Thread() throw (Exception, SysCallException)
    // 创建一个递归锁
    : _lock(true), _stop(false), _state(state_sleeping), _stack_size(0)
{
    int r = pthread_attr_init(&_attr);
    if (0 != r)
    {
        // RUNLOG or export exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_init");
    }
}

Thread::~Thread() throw ()
{
    pthread_attr_destroy(&_attr);
}

void* Thread::thread_proc(void* thread_param)
{
    Thread* thread = (Thread *)thread_param;
    // go!
    thread->entry();
    
    return NULL;
}

uint32_t Thread::get_current_thread_id() throw ()
{
    return pthread_self();
}

void Thread::create(bool detach) throw (Exception, SysCallException)
{
    before_start();

    int r = 0;

    // 设置线程栈大小
    if (0 < _stack_size)
    {
        r = pthread_attr_setstacksize(&_attr, _stack_size);
        if (0 != r)
        {
            THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_setstacksize");
        }
    }
        
    // 设置线程运行方式
    r = pthread_attr_setdetachstate(&_attr, detach ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);
    if (0 != r)
    {
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_setdetachstate");
    }
       
    // 创建线程
    r = pthread_create(&_thread, &_attr, thread_proc, this);
    if (0 != r)
    {
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_create");
    }
}

size_t Thread::get_stack_size() const throw (SysCallException)
{
    size_t stack_size = 0;
    int r = pthread_attr_getstacksize(&_attr, &stack_size);
    if (0 != r)
    {
        // RUNLOG or export exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_getstacksize");
    }

    return stack_size;
}

void Thread::join() throw (SysCallException)
{
    // 线程自己不能调用join
    if (Thread::get_current_thread_id() != this->get_thread_id())
    {
        if (0 < _thread)
        {
            int r = pthread_join(_thread, NULL);
            if (0 != r)
            {
                // RUNLOG or export exception
                THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_join");
            }
        }
    }
}

void Thread::detach() throw (SysCallException)
{
    int r = pthread_detach(_thread);
    if (0 != r)
    {
        // RUNLOG or export exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_detach");
    }
}

bool Thread::can_join() const throw (SysCallException)
{
    int state;
    int r = pthread_attr_getdetachstate(&_attr, &state);
    if (0 != r)
    {
        // RUNLOG or export exception
        THROW_SYSCALL_EXCEPTION(NULL, r, "pthread_attr_getdetachstate");
    }

    return (PTHREAD_CREATE_JOINABLE == state);
}

bool Thread::is_stop() const
{
    return _stop;
}

void Thread::do_wakeup(bool stop)
{   
    // 线程终止标识
    if (stop)
    {
        _stop = stop;
    }
    
    // 保证在唤醒线程之前，已经将它的状态修改为state_wakeup
    if (state_sleeping == _state)
    {
        _state = state_wakeuped;
        _cond.signal();  
    }
    else
    {
        _state = state_wakeuped;
    }
}

void Thread::wakeup()
{
    Mutex::Locker locker(_lock);
    do_wakeup(false);
}

void Thread::stop(bool wait_stop) throw (Exception, SysCallException)
{
    if (!_stop)
    {
        _stop = true;
        before_stop();
        Mutex::Locker locker(_lock);
        do_wakeup(true);
    }
    
    if (wait_stop && can_join())
    {
        join();
    }
}

void Thread::do_sleep(int milliseconds)
{
    // 非本线程调用无效
    if (this->get_thread_id() == Thread::get_current_thread_id())
    {    
        Mutex::Locker locker(_lock);
        if (!is_stop())
        {
            if (_state != state_wakeuped)
            {        
                _state = state_sleeping;
                if (0 > milliseconds)
                {
                    _cond.wait(_lock);
                }
                else
                {
                    _cond.timed_wait(_lock, milliseconds); 
                }               
            }

            // 不设置为state_wakeup，保证再次都可以调用do_millisleep
            _state = state_running;
        }
    }
}

// SYS_NS_END
