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
    CThreadPool() : next_thread_(0), thread_count_(0), thread_array_(NULL)
    {
		
    }

    // 创建线程池,并启动线程池中的所有线程，
	// 池线程创建成功后,并不会立即进行运行状态,而是处于等待状态,需要唤醒它们
	// thread_count:线程池中的线程个数
	// parameter:传递给池线程的参数
    void create(unsigned int thread_count, void* parameter = NULL)
    {
        thread_array_ = new ThreadClass*[thread_count];
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            thread_array_[i] = new ThreadClass;
            thread_array_[i]->set_index(i);
            thread_array_[i]->set_parameter(parameter);
        }
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            try
            {                
                thread_array_[i]->start();
                ++thread_count_;
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
        if (NULL != thread_array_)
        {
            unsigned int thread_count = thread_count_;
            for (unsigned int i = thread_count; 0 < i; --i)
            {
                thread_array_[i-1]->stop();
                --thread_count_;
            }

            delete []thread_array_;
            thread_array_ = NULL;
        }
    }

	// 激活线程池，将池中的所有线程唤醒，
	// 也可以单独调用各池线程的wakeup将它们唤醒
    void activate()
    {
        for (unsigned int i = 0; i < thread_count_; ++i)
		{
			thread_array_[i]->wakeup();
		}
    }

    // 获取线程个数
    unsigned int get_thread_count() const { return thread_count_; }

    // 得到线程池中的线程数组
    ThreadClass** get_thread_array() { return thread_array_; }
    ThreadClass** get_thread_array() const { return thread_array_; }

    // 根据线程编号,得到对应的线程
    ThreadClass* get_thread(unsigned int thread_index)
    {
        if (0 == thread_count_)
		{
			return NULL;
		}
		
        if (thread_index > thread_count_)
		{
			return NULL;
		}
		
        return thread_array_[thread_index];
    }
    
    // 根据线程编号,得到对应的线程
    ThreadClass* get_thread(unsigned int thread_index) const
    {
        if (0 == thread_count_)
		{
			return NULL;
		}
		
        if (thread_index > thread_count_)
		{
			return NULL;
		}
		
        return thread_array_[thread_index];
    }
    
	// 得到指向下个线程的指针,从第一个开始循环遍历,无终结点,即达到最后一个时,又指向第一个
	// 主要应用于采用轮询方式将一堆任务分配均衡分配给池线程
    ThreadClass* get_next_thread()
    {
        if (0 == thread_count_)
		{
			return NULL;
		}
		// 最后一个线程
        if (next_thread_ >= thread_count_)
		{
			next_thread_ = 0;
		}

        return thread_array_[next_thread_++];
    }

private:
	// 下一个线程索引号
    unsigned int next_thread_;
	// 线程数
    unsigned int thread_count_;
	// 线程数组
    ThreadClass** thread_array_;
};

// SYS_NS_END

#endif
