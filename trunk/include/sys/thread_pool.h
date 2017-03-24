#ifndef _SYS_THREAD_POOL_H_
#define _SYS_THREAD_POOL_H_

#include "thread.h"

// SYS_NS_BEGIN


// 模版实现,由用户特化自己的线程
template <class ThreadClass>
class CThreadPool
{
public:
    // 构造函数
    CThreadPool() : _next_thread(0), _thread_count(0), _thread_array(NULL)
    {
		
    }

    // 创建线程池,并启动线程池中的所有线程，
	// 池线程创建成功后,并不会立即进行运行状态,而是处于等待状态,需要唤醒它们
	// thread_count:线程池中的线程个数
	// parameter:传递给池线程的参数
    void create(unsigned int thread_count, void* parameter = NULL)
    {
        _thread_array = new ThreadClass*[thread_count];
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            _thread_array[i] = new ThreadClass;
            _thread_array[i]->set_index(i);
            _thread_array[i]->set_parameter(parameter);
        }
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            try
            {                
                _thread_array[i]->start();
                ++_thread_count;
            }
            catch (...)
            {
                destroy();
                throw;
            }
        }
    }

    // 销毁线程池,这里会等待所有线程退,然后删除线程
    void destroy()
    {
        if (NULL != _thread_array)
        {
            unsigned int thread_count = _thread_count;
            for (unsigned int i = thread_count; 0 < i; --i)
            {
                _thread_array[i-1]->stop();
                --_thread_count;
            }

            delete []_thread_array;
            _thread_array = NULL;
        }
    }

	// 激活线程池，将池中的所有线程唤醒，
	// 也可以单独调用各池线程的wakeup将它们唤醒
    void activate()
    {
        for (unsigned int i = 0; i < _thread_count; ++i)
		{
			_thread_array[i]->wakeup();
		}
    }

    // 获取线程个数
    unsigned int get_thread_count() const { return _thread_count; }

    // 得到线程池中的线程数组
    ThreadClass** get_thread_array() { return _thread_array; }
    ThreadClass** get_thread_array() const { return _thread_array; }

    // 根据线程编号,得到对应的线程
    ThreadClass* get_thread(unsigned int thread_index)
    {
        if (0 == _thread_count)
		{
			return NULL;
		}
		
        if (thread_index > _thread_count)
		{
			return NULL;
		}
		
        return _thread_array[thread_index];
    }
    
    // 根据线程编号,得到对应的线程
    ThreadClass* get_thread(unsigned int thread_index) const
    {
        if (0 == _thread_count)
		{
			return NULL;
		}
		
        if (thread_index > _thread_count)
		{
			return NULL;
		}
		
        return _thread_array[thread_index];
    }
    
	// 得到指向下个线程的指针,从第一个开始循环遍历,无终结点,即达到最后一个时,又指向第一个
	// 主要应用于采用轮询方式将一堆任务分配均衡分配给池线程
    ThreadClass* get_next_thread()
    {
        if (0 == _thread_count)
		{
			return NULL;
		}
		// 最后一个线程
        if (_next_thread >= _thread_count)
		{
			_next_thread = 0;
		}

        return _thread_array[_next_thread++];
    }

private:
	// 下一个线程索引号
    unsigned int _next_thread;
	// 线程数
    unsigned int _thread_count;
	// 线程数组
    ThreadClass** _thread_array;
};

// SYS_NS_END

#endif
