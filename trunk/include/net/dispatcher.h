#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "connection.h"

class Dispatcher {
public:
	explicit Dispatcher()
	{
	}
	
	virtual ~Dispatcher() {}

	virtual bool ms_can_fast_dispatch(const Message* m) const { return false; }

	virtual bool ms_can_fast_dispatch_any() const { return false; }

	virtual void ms_fast_dispatch(Message* m) { abort(); }

	virtual void ms_fast_preprocess(Message* m) {}

	virtual bool ms_dispatch(Message* m) = 0;

	virtual void ms_handle_connect(Connection* con) {}

	virtual void ms_handle_fast_connect(Connection* con) {}

	virtual void ms_handle_accept(Connection* con) {}

	virtual void ms_handle_fast_accept(Connection* con) {}

	virtual bool ms_handle_reset(Connection *con) = 0;

	virtual void ms_handle_remote_reset(Connection* con) = 0;

	virtual bool ms_handle_refused(Connection* con) = 0;

	// virtual bool ms_get_authorizer(int dest_type, AuthAuthorizer** a, bool force_new) { return false; }

	// virtual bool ms_verify_authorizer(Connection* con, int peer_type, int protocol, buffer& authorizer,
	//				buffer& authorizer_reply, bool& isvalid, CryptoKey& session_key) { return false; }
private:
	explicit Dispatcher(const Dispatcher& other);
	Dispatcher& operator=(const Dispatcher& rhs);
};


#endif
