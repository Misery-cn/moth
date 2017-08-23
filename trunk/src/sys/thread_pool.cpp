#include "thread_pool.h"

// SYS_NS_BEGIN

ThreadPool::ThreadPool(uint32_t thread_num) : _stop(false), _pause(0), _num_threads(thread_num), _last_work_index(0), _processing(0)
{
}

ThreadPool::~ThreadPool()
{
}

void ThreadPool::worker(WorkThread* wt)
{
    _lock.lock();

    while (!_stop)
    {
        join_old_threads();

        // 大于默认线程数
        if (_threads.size() > _num_threads)
        {
            // 加入待join队列
            _threads.erase(wt);
            _old_threads.push_back(wt);
            break;
        }

        if (!_pause && !_work_queues.empty())
        {
            BaseWorkQueue* wq = NULL;
            int wqsize = _work_queues.size();
            bool did = false;
            while (wqsize--)
            {
                _last_work_index++;
                _last_work_index %= _work_queues.size();
                wq = _work_queues[_last_work_index];

				// 任务队列中取一个任务
                void* item = wq->_void_dequeue();
                if (item)
                {
                    _processing++;
                    _lock.unlock();
                    wq->_void_process(item);
                    _lock.lock();
                    wq->_void_process_finish(item);
                    _processing--;
                    if (_pause || _draining)
                    {
                        _wait_cond.signal();
                    }
                    did = true;
                    break;
                }
            }

            if (did)
            {
                continue;
            }
        }

		_cond.timed_wait(_lock, 1);
    }
	
    _lock.unlock();
}

// 创建工作线程
void ThreadPool::start_threads()
{
    while (_threads.size() < _num_threads)
    {
        WorkThread* wt = new WorkThread(this);

        _threads.insert(wt);

        // 启动工作线程
        wt->create();
    }
}

// 关闭待join的线程
void ThreadPool::join_old_threads()
{
    while (!_old_threads.empty())
    {
        _old_threads.front()->join();
        delete _old_threads.front();
        _old_threads.pop_front();
    }
}

void ThreadPool::start()
{
    Mutex::Locker locker(_lock);
	
	start_threads();
}

void ThreadPool::stop(bool clear_after)
{
    _lock.lock();
    _stop = true;
    _cond.signal();
    join_old_threads();
    _lock.unlock();

    for (std::set<WorkThread*>::iterator it = _threads.begin(); it != _threads.end(); ++it)
    {
        (*it)->join();
        delete *it;
	}

    _threads.clear();

    _lock.lock();
    for (uint32_t i = 0; i < _work_queues.size(); i++)
    {
        _work_queues[i]->_clear();
    }
	
    _stop = false;
	
    _lock.unlock();
}

void ThreadPool::pause()
{
    Mutex::Locker locker(_lock);
	
    _pause++;
	
    // 如果还有线程正在运行,先挂起,等待线程执行完成唤醒
    while (_processing)
    {
        _wait_cond.wait(_lock);
    }
}

void ThreadPool::unpause()
{
	Mutex::Locker locker(_lock);
	
	_pause--;
	
	_cond.signal();
}

void ThreadPool::drain(BaseWorkQueue* wq)
{
	Mutex::Locker locker(_lock);
	
	_draining++;
	
	while (_processing || (wq != NULL && !wq->_empty()))
	{
		_wait_cond.wait(_lock);
	}
	
	_draining--;
}

// SYS_NS_END


// int main()
// {
//        return 0;
// }

