#ifndef _DISPATCH_QUEUE_H
#define _DISPATCH_QUEUE_H

#include "thread.h"
#include "messenger.h"
#include "connection.h"
#include "prioritizedqueue.h"

class Message;
class Connection;
class DispatchQueue;
class Socket;
class SimpleMessenger;


class DispatchQueue
{
	class QueueItem
	{
	public:
		explicit QueueItem(Message* m) : _type(-1), _con(NULL), _msg(m) {}
		QueueItem(int type, Connection* con) : _type(type), _con(con), _msg(NULL) {}
		
		bool is_code() const
		{
			return _type != -1;
		}
		
		int get_code () const
		{
			return _type;
		}
		
		Message* get_message()
		{
			return _msg->get();
		}
		
		Connection* get_connection()
		{
			return static_cast<Connection*>(_con->get());
		}
		
	private:
		int _type;
		Connection* _con;
		Message* _msg;
	};
    
	Messenger* _msgr;
	mutable Mutex _lock;
	Cond _cond;

	// 优先级队列
	PrioritizedQueue<QueueItem, uint64_t> _mqueue;

	std::set< std::pair<double, Message*> > _marrival;
	std::map<Message*, std::set< std::pair<double, Message*> >::iterator> _marrival_map;
	
	void add_arrival(Message* m)
	{
		_marrival_map.insert(std::make_pair(m, _marrival.insert(std::make_pair(m->get_recv_stamp(), m)).first));
	}
	
	void remove_arrival(Message* m)
	{
		std::map<Message*, std::set<std::pair<double, Message*> >::iterator>::iterator i = _marrival_map.find(m);
		_marrival.erase(i->second);
		_marrival_map.erase(i);
	}

	atomic_t _next_id;
    
	enum { D_CONNECT = 1, D_ACCEPT, D_BAD_REMOTE_RESET, D_BAD_RESET, D_CONN_REFUSED, D_NUM_CODES };

	// 发送线程,处理队列_mqueue中的消息
	class DispatchThread : public Thread
	{
	private:
		DispatchQueue* _dq;
	public:
		explicit DispatchThread(DispatchQueue* dq) : _dq(dq) {}
		void entry()
		{
			_dq->entry();
		}
	} _dispatch_thread;

	Mutex _local_delivery_lock;
	Cond _local_delivery_cond;
	bool _stop_local_delivery;
	std::list< std::pair<Message*, int> > _local_messages;

	// 处理本地消息线程
	class LocalDeliveryThread : public Thread
	{
	private:
		DispatchQueue* _dq;
	public:
		explicit LocalDeliveryThread(DispatchQueue* dq) : _dq(dq) {}
		void entry()
		{
			_dq->run_local_delivery();
		}
	} _local_delivery_thread;

	uint64_t pre_dispatch(Message* m);
	void post_dispatch(Message* m, uint64_t msize);

public:

	Throttle _dispatch_throttler;

	bool _stop;
	void local_delivery(Message* m, int priority);
	void run_local_delivery();

	double get_max_age(utime_t now) const;

	int get_queue_len() const
	{
		Mutex::Locker locker(_lock);
		return _mqueue.length();
	}

	void dispatch_throttle_release(uint64_t msize);

	void queue_connect(Connection* con)
	{
		Mutex::Locker locker(_lock);
		if (_stop)
		{
			return;
		}
		
		_mqueue.enqueue_strict(0, MSG_PRIO_HIGHEST, QueueItem(D_CONNECT, con));
		_cond.signal();
	}
	
	void queue_accept(Connection* con)
	{
		Mutex::Locker locker(_lock);
		if (_stop)
		{
			return;
		}
		
		_mqueue.enqueue_strict(0, MSG_PRIO_HIGHEST, QueueItem(D_ACCEPT, con));
		_cond.signal();
	}
	
	void queue_remote_reset(Connection* con)
	{
		Mutex::Locker locker(_lock);
		if (_stop)
		{
			return;
		}
		
		_mqueue.enqueue_strict(0, MSG_PRIO_HIGHEST, QueueItem(D_BAD_REMOTE_RESET, con));
		_cond.signal();
	}
	
	void queue_reset(Connection* con)
	{
		Mutex::Locker locker(_lock);
		if (_stop)
		{
			return;
		}
		
		_mqueue.enqueue_strict(0, MSG_PRIO_HIGHEST, QueueItem(D_BAD_RESET, con));
		_cond.signal();
	}
	
	void queue_refused(Connection* con)
	{
		Mutex::Locker locker(_lock);
		if (_stop)
		{
			return;
		}
		
		_mqueue.enqueue_strict(0, MSG_PRIO_HIGHEST, QueueItem(D_CONN_REFUSED, con));
		_cond.signal();
	}

	bool can_fast_dispatch(Message* m) const;
	
	void fast_dispatch(Message* m);
	
	void fast_preprocess(Message* m);
	
	void enqueue(Message* m, int priority, uint64_t id);
	
	void discard_queue(uint64_t id);
	
	void discard_local();
	
	uint64_t get_id()
	{
		return _next_id++;
	}

	// 启动发送消息线程和本地消息发送线程
	void start();
	
	void entry();
	
	void wait();
	
	void shutdown();
	
	bool is_started() const {return _dispatch_thread.is_started();}

	// config
	DispatchQueue(Messenger* msgr, std::string& name) : _msgr(msgr), _lock(), _mqueue(16777216, 65536),
				_next_id(1), _dispatch_thread(this), _local_delivery_lock(), _stop_local_delivery(false),
				_local_delivery_thread(this), _dispatch_throttler(std::string("msgr_dispatch_throttler-") + name, 100 << 20),
				_stop(false)
	{}
	
	virtual ~DispatchQueue()
	{
	}
};

#endif
