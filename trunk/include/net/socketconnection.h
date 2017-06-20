#ifndef _SOCKET_CONNECTION_H_
#define _SOCKET_CONNECTION_H_

#include "connection.h"

class Socket;

class SocketConnection : public Connection
{
private:
	Socket* _socket;
	friend class Socket;

public:
	SocketConnection(Messenger* m) : Connection(m), _socket(NULL)
	{}

	virtual ~SocketConnection();

	Socket* get_socket();

	bool try_get_socket(Socket** s);

	bool clear_socket(Socket* s);

	void reset_socket(Socket* s);

	bool is_connected();

	int send_message(Message* m);
	
	void send_keepalive();
	
	void mark_down();
	
	void mark_disposable();

};

#endif