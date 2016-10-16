#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "ref_countable.h"
#include "mutex.h"

class CMessenger;

class Connection : public CRefCountable
{
public:
	Connection(CMessenger* m) : msger_(m), ref_(NULL)
	{
	}

	virtual ~Connection()
	{
		if (ref_)
		{
			ref_->dec_refcount();
		}
	}

	CMessenger* get_messenger() { return msger_; }

	void set_ref(CRefCountable* ref)
	{
		// CMutexGuard(lock_);
		CMutex::Locker l(lock_);
		// 之前的引用-1
		if (ref_)
		{
			ref_->dec_refcount();
		}

		ref_ = ref;
	}

	CRefCountable* get_ref()
	{
		// CMutexGuard(lock_);
		CMutex::Locker l(lock_);

		if (ref_)
		{
			return ref_->get();
		}

		return NULL;
	}

	virtual bool is_connected() = 0;

	virtual int send_message(Message* m) = 0;

	virtual void mark_down() = 0;

private:
	mutable CMutex lock_;
	CMessenger* msger_;
	CRefCountable* ref_;
	int type_;
	entity_addr_t addr_;
	time_t last_keepalive;
	time_t last_keepalive_ack;
};

#endif
