#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include "connection.h"

class CDispatcher
{
public:
	CDispatcher();
	virtual ~CDispatcher();

	virtual bool ms_dispatch(Message* msg) = 0;

	virtual void ms_handle_connect(Connection* con) {}

	virtual void ms_handle_accept(Connection* con) {}
};

#endif
