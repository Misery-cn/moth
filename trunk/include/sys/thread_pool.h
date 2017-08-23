#ifndef _SYS_THREAD_POOL_H_
#define _SYS_THREAD_POOL_H_

#include <list>
#include <vector>
#include <set>
#include "thread.h"

// SYS_NS_BEGIN

// 线程池类
class ThreadPool
{
public:

    ThreadPool(uint32_t thread_num);

    virtual ~ThreadPool();
	
	// 任务队列基类
    class BaseWorkQueue
    {
    public:
        BaseWorkQueue() {}

        virtual ~BaseWorkQueue() {}

        virtual void _clear() = 0;

        virtual bool _empty() = 0;

        virtual void* _void_dequeue() = 0;

        virtual void _void_process(void*) = 0;

        virtual void _void_process_finish(void*) = 0;
    };

    // 获取线程数
    uint32_t get_num_threads()
    {
        Mutex::Locker locker(_lock);
        return _num_threads;
    }

    // 添加任务队列
    void add_work_queue(BaseWorkQueue* wq)
    {
        Mutex::Locker locker(_lock);
        _work_queues.push_back(wq);
    }

	// 删除任务队列
    void remove_work_queue(BaseWorkQueue* wq)
    {
        Mutex::Locker locker(_lock);

        uint32_t i = 0;
        while (_work_queues[i] != wq)
        {
            i++;
        }

        for (i++; i < _work_queues.size(); i++)
        {
            _work_queues[i - 1] = _work_queues[i];
        }

        _work_queues.resize(i - 1);
    }

    void lock()
    {
        _lock.lock();
    }

    void unlock()
    {
        _lock.unlock();
    }

    void wakeup()
    {
        _cond.signal();
    }

    void wait()
    {
        _cond.wait(_lock);
    }

	// 启动线程池
    void start();

	// 停止线程池
    void stop(bool clear_after=true);

	// 暂停线程池中所有线程
    void pause();
	// 恢复被暂停的线程
    void unpause();
	// 等待任务完成
    void drain(BaseWorkQueue* wq = NULL);

private:
	// 工作线程类
    class WorkThread : public Thread
    {
	public:
	
        WorkThread(ThreadPool* p) : _pool(p) {}
        // 工作线程启动后,即调用pool的worker函数
        void entry()
        {
            _pool->worker(this);
        }
		
	private:	
		ThreadPool* _pool;
    };

	// 启动线程池中的线程
    void start_threads();

    // join线程
    void join_old_threads();

	// 线程池工作函数
    void worker(WorkThread* wt);

public:
    template<class T>
    class BatchWorkQueue : public BaseWorkQueue
    {
    public:
		// 添加任务队列到线程池
        BatchWorkQueue(ThreadPool* p) : _pool(p)
        {
            _pool->add_work_queue(this);
        }

		
        virtual ~BatchWorkQueue()
        {
            _pool->remove_work_queue(this);
        }

        bool queue(T* item)
        {
        	Mutex::Locker locker(_pool->_lock);
			
            bool r = _enqueue(item);
			
			// 唤醒挂起线程
            _pool->_cond.signal();
			
            return r;
        }

        void dequeue(T* item)
        {
            Mutex::Locker locker(_pool->_lock);
			
            _dequeue(item);
        }

        void clear()
        {
            Mutex::Locker locker(_pool->_lock);
			
            _clear();
        }

        void lock()
        {
            _pool->lock();
        }
        void unlock()
        {
            _pool->unlock();
        }

        void wakeup()
        {
            _pool->wakeup();
        }

        void drain()
        {
            _pool->drain(this);
        }

	protected:
        virtual void _process(const std::list<T*>& items) = 0;

	private:
        virtual bool _enqueue(T*) = 0;

        virtual void _dequeue(T*) = 0;

        virtual void _dequeue(std::list<T*>*) = 0;

        virtual void _process_finish(const std::list<T*>& ) {}

		// 一次返回整个队列
        void* _void_dequeue()
        {
            std::list<T*>* out(new std::list<T*>);
            _dequeue(out);
            if (!out->empty())
            {
                return (void*)out;
            }
            else
            {
                delete out;
                return NULL;
            }
        }

        void _void_process(void* p)
        {
            _process(*((std::list<T*>*)p));
        }

        void _void_process_finish(void* p)
        {
            _process_finish(*(std::list<T*>*)p);
            delete (std::list<T*>*)p;
        }
		
	private:
		// 线程池
		ThreadPool* _pool;
    };

	template<class T>
	class WorkQueue : public BaseWorkQueue
	{
	public:
		WorkQueue(ThreadPool* p) : _pool(p)
		{
			_pool->add_work_queue(this);
		}
		
		virtual ~WorkQueue()
		{
			_pool->remove_work_queue(this);
		}

		// 入列函数,需要调用派生类实现的_enqueue
		bool queue(T* item)
		{
      		Mutex::Locker locker(_pool->_lock);
			bool r = _enqueue(item);
			_pool->_cond.signal();
			return r;
		}

		// 出列函数,需要调用派生类实现的_dequeue
		void dequeue(T *item)
		{
			Mutex::Locker locker(_pool->_lock);
			_dequeue(item);
		}
		
		void clear()
		{
			Mutex::Locker locker(_pool->_lock);
			_clear();
		}

		void lock()
		{
			_pool->lock();
		}
		
		void unlock()
		{
			_pool->unlock();
		}
		
		void wakeup()
		{
			_pool->wakeup();
		}
		
		void _wait()
		{
			_pool->wait();
		}
		
		void drain()
		{
			_pool->drain(this);
		}

	protected:

		virtual void _process(T* t) = 0;

	private:
		// 入列
	    virtual bool _enqueue(T*) = 0;
		// 出列
	    virtual void _dequeue(T*) = 0;
		// 出列
	    virtual T* _dequeue() = 0;
		
	    virtual void _process_finish(T*) {}

	    void* _void_dequeue()
		{
			return (void *)_dequeue();
	    }
		
	    void _void_process(void* p)
		{
			_process(static_cast<T*>(p));
	    }
		
	    void _void_process_finish(void* p)
		{
			_process_finish(static_cast<T*>(p));
	    }

	private:
		ThreadPool* _pool;
	};

private:
    Mutex _lock;
    Cond _cond;
    // 等待正在执行线程的条件变量
    Cond _wait_cond;
    // 线程池是否停止
    bool _stop;
	uint32_t _pause;
	uint32_t _draining;
    // 线程数
    uint32_t _num_threads;
    // 工作队列
    std::vector<BaseWorkQueue*> _work_queues;
    // 上一次工作队列索引号
    uint32_t _last_work_index;
    // 工作线程
    std::set<WorkThread*> _threads;
    // 等待join的线程
    std::list<WorkThread*> _old_threads;
    // 正在工作的线程数
    uint32_t _processing;
};

// SYS_NS_END

#endif
