#ifndef _MASTER_H_
#define _MASTER_H_

#include "msg_types.h"
#include "dispatcher.h"

class Master : public Dispatcher
{
public:
	Master();
	virtual ~Master();
	
	virtual bool ms_dispatch(Message* m);
	
	virtual bool ms_handle_reset(Connection *con);
	
	virtual void ms_handle_remote_reset(Connection* con);
	
	virtual bool ms_handle_refused(Connection* con);
	
private:
	
};

#endif