#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "ref_countable.h"
#include "mutex.h"

class CMessenger;

class Connection : public CRefCountable
{
public:
	Connection(CMessenger* m) : _msger(m), _ref(NULL)
	{
	}

	virtual ~Connection()
	{
		if (_ref)
		{
			_ref->dec_refcount();
		}
	}

	CMessenger* get_messenger() { return _msger; }

	void set_ref(CRefCountable* ref)
	{
		// CMutexGuard(_lock);
		CMutex::Locker l(_lock);
		// 之前的引用-1
		if (_ref)
		{
			_ref->dec_refcount();
		}

		_ref = ref;
	}

	CRefCountable* get_ref()
	{
		// CMutexGuard(_lock);
		CMutex::Locker l(_lock);

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
	mutable CMutex _lock;
	CMessenger* _msger;
	CRefCountable* _ref;
	int _type;
	entity_addr_t _addr;
	time_t last_keepalive;
	time_t last_keepalive_ack;
};

#endif
