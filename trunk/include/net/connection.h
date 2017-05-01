#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "ref_countable.h"
#include "mutex.h"

class Messenger;

class Connection : public RefCountable
{
public:
	Connection(Messenger* m) : _msger(m), _ref(NULL)
	{
	}

	virtual ~Connection()
	{
		if (_ref)
		{
			_ref->dec();
		}
	}

	Messenger* get_messenger() { return _msger; }

	void set_ref(RefCountable* ref)
	{
		// MutexGuard(_lock);
		Mutex::Locker l(_lock);
		// 之前的引用-1
		if (_ref)
		{
			_ref->dec();
		}

		_ref = ref;
	}

	RefCountable* get_ref()
	{
		// MutexGuard(_lock);
		Mutex::Locker l(_lock);

		if (_ref)
		{
			return _ref->get();
		}

		return NULL;
	}

	virtual bool is_connected() = 0;

	virtual int send_message(Message* m) = 0;

	virtual void mark_down() = 0;

private:
	mutable Mutex _lock;
	Messenger* _msger;
	RefCountable* _ref;
	int _type;
	entity_addr_t _addr;
	time_t last_keepalive;
	time_t last_keepalive_ack;
};

#endif
