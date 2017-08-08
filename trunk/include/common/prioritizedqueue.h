#ifndef _PRIORITIZED_QUEUE_H_
#define _PRIORITIZED_QUEUE_H_

#include "opqueue.h"

template <typename T, typename K>
class PrioritizedQueue : public OpQueue <T, K>
{
	int64_t _total_priority;
	int64_t _max_tokens_per_subqueue;
	int64_t _min_cost;

	typedef std::list<std::pair<unsigned, T> > ListPairs;
	static unsigned filter_list_pairs(ListPairs* l, std::function<bool (T)> f)
	{
		unsigned ret = 0;
		for (typename ListPairs::iterator i = l->end(); i != l->begin();)
		{
			auto next = i;
			--next;
			if (f(next->second))
			{
				++ret;
				l->erase(next);
			}
			else
			{
				i = next;
			}
		}
		
		return ret;
	}

	struct SubQueue
	{
	private:
		typedef std::map<K, ListPairs> Classes;
		Classes q;
		unsigned _tokens, _max_tokens;
		int64_t _size;
		typename Classes::iterator cur;
	public:
		SubQueue(const SubQueue& other) : q(other.q), _tokens(other._tokens),
					_max_tokens(other._max_tokens), _size(other._size), cur(q.begin())
		{}
		
		SubQueue() : _tokens(0), _max_tokens(0), _size(0), cur(q.begin())
		{}
		
		void set_max_tokens(unsigned mt)
		{
			_max_tokens = mt;
		}
		
		unsigned get_max_tokens() const
		{
			return _max_tokens;
		}
		
		unsigned num_tokens() const
		{
			return _tokens;
		}
		
		void put_tokens(unsigned t)
		{
			_tokens += t;
			if (_tokens > _max_tokens)
			{
				_tokens = _max_tokens;
			}
		}
		
		void take_tokens(unsigned t)
		{
			if (_tokens > t)
			{
				_tokens -= t;
			}
			else
			{
				_tokens = 0;
			}
		}
		
		void enqueue(K cl, unsigned cost, T item)
		{
			q[cl].push_back(std::make_pair(cost, item));
			
			if (cur == q.end())
			{
				cur = q.begin();
			}
			
			_size++;
		}
		
		void enqueue_front(K cl, unsigned cost, T item)
		{
			q[cl].push_front(std::make_pair(cost, item));
			if (cur == q.end())
			{
				cur = q.begin();
			}
			
			_size++;
		}
		
		std::pair<unsigned, T> front() const
		{
			return cur->second.front();
		}
		
		void pop_front()
		{
			cur->second.pop_front();
			if (cur->second.empty())
			{
				q.erase(cur++);
			}
			else
			{
				++cur;
			}
			
			if (cur == q.end())
			{
				cur = q.begin();
			}
			
			_size--;
		}
		
		unsigned length() const
		{
			return (unsigned)_size;
		}
		
		bool empty() const
		{
			return q.empty();
		}
		
		void remove_by_filter(std::function<bool (T)> f)
		{
			for (typename Classes::iterator i = q.begin(); i != q.end();)
			{
				_size -= filter_list_pairs(&(i->second), f);
				if (i->second.empty())
				{
					if (cur == i)
					{
						++cur;
					}
					q.erase(i++);
				}
				else
				{
					++i;
				}
			}
			
			if (cur == q.end())
			{
				cur = q.begin();
			}
		}
		
		void remove_by_class(K k, std::list<T> *out)
		{
			typename Classes::iterator i = q.find(k);
			if (i == q.end())
			{
				return;
			}
			
			_size -= i->second.size();
			if (i == cur)
			{
				++cur;
			}
			
			if (out)
			{
				for (typename ListPairs::reverse_iterator j = i->second.rbegin(); j != i->second.rend(); ++j)
				{
					out->push_front(j->second);
				}
			}
			
			q.erase(i);
			
			if (cur == q.end())
			{
				cur = q.begin();
			}
		}
	};

	typedef std::map<uint32_t, SubQueue> SubQueues;
	SubQueues _high_queue;
	SubQueues _queue;

	SubQueue* create_queue(unsigned priority)
	{
		typename SubQueues::iterator p = _queue.find(priority);
		if (p != _queue.end())
		{
			return &p->second;
		}
		
		_total_priority += priority;
		SubQueue* sq = &_queue[priority];
		sq->set_max_tokens(_max_tokens_per_subqueue);
		return sq;
	}

	void remove_queue(unsigned priority)
	{
		_queue.erase(priority);
		_total_priority -= priority;
	}

	void distribute_tokens(unsigned cost)
	{
		if (0 == _total_priority)
		{
			return;
		}
		
		for (typename SubQueues::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			i->second.put_tokens(((i->first * cost) / _total_priority) + 1);
		}
	}

public:
	PrioritizedQueue(unsigned max_per, unsigned min_c) : _total_priority(0), _max_tokens_per_subqueue(max_per), _min_cost(min_c)
	{}

	unsigned length() const
	{
		unsigned total = 0;
		for (typename SubQueues::const_iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			total += i->second.length();
		}
		
		for (typename SubQueues::const_iterator i = _high_queue.begin(); i != _high_queue.end(); ++i)
		{
			total += i->second.length();
		}
		
		return total;
	}

	void remove_by_filter(std::function<bool (T)> f)
	{
		for (typename SubQueues::iterator i = _queue.begin(); i != _queue.end();)
		{
			unsigned priority = i->first;
      
			i->second.remove_by_filter(f);
			if (i->second.empty())
			{
				++i;
				remove_queue(priority);
			}
			else
			{
				++i;
			}
		}

		
		for (typename SubQueues::iterator i = _high_queue.begin(); i != _high_queue.end();)
		{
			i->second.remove_by_filter(f);
			if (i->second.empty())
			{
				_high_queue.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}

	void remove_by_class(K k, std::list<T>* out = 0)
	{
		for (typename SubQueues::iterator i = _queue.begin(); i != _queue.end();)
		{
			i->second.remove_by_class(k, out);
			if (i->second.empty())
			{
				unsigned priority = i->first;
				++i;
				remove_queue(priority);
			}
			else
			{
				++i;
			}
		}
		
		for (typename SubQueues::iterator i = _high_queue.begin(); i != _high_queue.end();)
		{
			i->second.remove_by_class(k, out);
			if (i->second.empty())
			{
				_high_queue.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}

	void enqueue_strict(K cl, unsigned priority, T item)
	{
		_high_queue[priority].enqueue(cl, 0, item);
	}

	void enqueue_strict_front(K cl, unsigned priority, T item)
	{
		_high_queue[priority].enqueue_front(cl, 0, item);
	}

	void enqueue(K cl, unsigned priority, unsigned cost, T item)
	{
		if (cost < _min_cost)
		{
			cost = _min_cost;
		}
		
		if (cost > _max_tokens_per_subqueue)
		{
			cost = _max_tokens_per_subqueue;
		}
		
		create_queue(priority)->enqueue(cl, cost, item);
	}

	void enqueue_front(K cl, unsigned priority, unsigned cost, T item)
	{
		if (cost < _min_cost)
		{
			cost = _min_cost;
		}
		
		if (cost > _max_tokens_per_subqueue)
		{
			cost = _max_tokens_per_subqueue;
		}
		
		create_queue(priority)->enqueue_front(cl, cost, item);
	}

	bool empty() const
	{
		return _queue.empty() && _high_queue.empty();
	}

	T dequeue()
	{
		if (!(_high_queue.empty()))
		{
			T ret = _high_queue.rbegin()->second.front().second;
			_high_queue.rbegin()->second.pop_front();
			if (_high_queue.rbegin()->second.empty())
			{
				_high_queue.erase(_high_queue.rbegin()->first);
			}
			
			return ret;
		}

		for (typename SubQueues::iterator i = _queue.begin(); i != _queue.end(); ++i)
		{
			if (i->second.front().first < i->second.num_tokens())
			{
				T ret = i->second.front().second;
				unsigned cost = i->second.front().first;
				i->second.take_tokens(cost);
				i->second.pop_front();
				if (i->second.empty())
				{
					remove_queue(i->first);
				}
				distribute_tokens(cost);
				return ret;
			}
		}

		T ret = _queue.rbegin()->second.front().second;
		unsigned cost = _queue.rbegin()->second.front().first;
		_queue.rbegin()->second.pop_front();
		if (_queue.rbegin()->second.empty())
		{
			remove_queue(_queue.rbegin()->first);
		}
		
		distribute_tokens(cost);
		
		return ret;
	}
};

#endif
