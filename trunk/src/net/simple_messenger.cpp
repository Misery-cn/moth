#include "simple_messenger.h"
#include "log.h"

SimpleMessenger::SimpleMessenger(entity_name_t name, std::string mname, uint64_t nonce)
  : PolicyMessenger(name, mname, nonce),
	_accepter(this, nonce),
    _dispatch_queue(this, mname),
    _reaper_thread(this),
	_nonce(nonce),
    _lock(), _did_bind(false),
    _global_seq(0),
    _dispatch_throttler(std::string("msgr_dispatch_throttler_") + mname),
    _reaper_started(false), _reaper_stop(false),
    _timeout(0),
    _local_connection(new SocketConnection(this))
{
	init_local_connection();
}

SimpleMessenger::~SimpleMessenger()
{
}

void SimpleMessenger::ready()
{
	DEBUG_LOG("messenger ready");
	
	_dispatch_queue.start();

	Mutex::Locker locker(_lock);
	if (_did_bind)
	{
		_accepter.start();
	}
}


int SimpleMessenger::shutdown()
{
	mark_down_all();

	_local_connection->set_priv(NULL);

	Mutex::Locker locker(_lock);
	_stop_cond.signal();
	_stopped = true;

	return 0;
}

int SimpleMessenger::send_message(Message* m, const entity_inst_t& dest)
{
	DEBUG_LOG("SimpleMessenger send_message by entity");
	
	m->get_header().src = get_entity_name();

	if (!m->get_priority())
	{
		m->set_priority(get_default_send_priority());
	}

	if (dest._addr == entity_addr_t())
	{
		m->dec();
		return -EINVAL;
	}

	Mutex::Locker locker(_lock);
	Socket* sockt = lookup_socket(dest._addr);
	submit_message(m, (sockt ? static_cast<SocketConnection*>(sockt->_connection_state->get()) : NULL), dest._addr, dest._name.type(), true);
	
	return 0;
}

int SimpleMessenger::send_message(Message* m, Connection* con)
{
	DEBUG_LOG("SimpleMessenger send_message by connection");
	
	m->get_header().src = get_entity_name();

	if (!m->get_priority())
	{
		m->set_priority(get_default_send_priority());
	}

	submit_message(m, static_cast<SocketConnection*>(con), con->get_peer_addr(), con->get_peer_type(), false);
	return 0;
}

void SimpleMessenger::set_addr_unknowns(entity_addr_t& addr)
{
	if (_entity._addr.is_blank_ip())
	{
		int port = _entity._addr.get_port();
		_entity._addr._addr = addr._addr;
		_entity._addr.set_port(port);
		init_local_connection();
	}
}


void SimpleMessenger::reaper_entry()
{
	DEBUG_LOG("socket reaper start");
	
	Mutex::Locker locker(_lock);
	while (!_reaper_stop)
	{
		reaper();
		if (_reaper_stop)
		{
			break;
		}
		
		_reaper_cond.wait(_lock);
	}
}


void SimpleMessenger::reaper()
{
	DEBUG_LOG("reaper start");
	
	while (!_socket_reap_queue.empty())
	{
		Socket* socket = _socket_reap_queue.front();
		_socket_reap_queue.pop_front();
		
		DEBUG_LOG("reaping socket");

		socket->_lock.lock();
		socket->discard_out_queue();
		
		if (socket->_connection_state)
		{
			bool cleared = socket->_connection_state->clear_socket(socket);
		}
		
		socket->_lock.unlock();
		socket->unregister_socket();
		_sockets.erase(socket);
		
		_lock.unlock();
		socket->join();
		_lock.lock();

		if (0 <= socket->_fd)
		{
			::close(socket->_fd);
		}
		
    	socket->dec();
	}
}

void SimpleMessenger::queue_reap(Socket* s)
{
	Mutex::Locker locker(_lock);
	_socket_reap_queue.push_back(s);
	_reaper_cond.signal();
}

bool SimpleMessenger::is_connected(Connection* con)
{
	bool r = false;
	if (con)
	{
		Socket* socket = static_cast<Socket*>(static_cast<SocketConnection*>(con)->get_socket());
		if (socket)
		{
			r = socket->is_connected();
			socket->dec();
		}
	}
	
	return r;
}

int SimpleMessenger::bind(const entity_addr_t& bind_addr)
{	
	{
		Mutex::Locker locker(_lock);

		if (_started)
		{
			return -1;
		}
	}

	std::set<int> avoid_ports;
	int r = _accepter.bind(bind_addr, avoid_ports);
	if (0 <= r)
	{
		_did_bind = true;
	}
	
	return r;
}

int SimpleMessenger::rebind(const std::set<int>& avoid_ports)
{
	_accepter.stop();
	mark_down_all();
	return _accepter.rebind(avoid_ports);
}

int SimpleMessenger::start()
{
	DEBUG_LOG("messenger start");
	
	Mutex::Locker locker(_lock);

	_started = true;
	_stopped = false;

	if (!_did_bind)
	{
		_entity._addr._nonce = _nonce;
		init_local_connection();
	}

	_reaper_started = true;
	_reaper_thread.create();
	
	return 0;
}

Socket* SimpleMessenger::add_accept_socket(int fd)
{
	Mutex::Locker locker(_lock);
	Socket* socket = new Socket(this, Socket::SOCKET_ACCEPTING, NULL);
	socket->_fd = fd;
	socket->_lock.lock();
	socket->start_reader();
	socket->_lock.unlock();
	
	_sockets.insert(socket);
	_accepting_sockets.insert(socket);
	return socket;
}


Socket* SimpleMessenger::connect_rank(const entity_addr_t& addr, int type, SocketConnection* con, Message* first)
{
	DEBUG_LOG("SimpleMessenger connect_rank");
	
	Socket* socket = new Socket(this, Socket::SOCKET_CONNECTING, static_cast<SocketConnection*>(con));
	socket->_lock.lock();
	socket->set_peer_type(type);
	socket->set_peer_addr(addr);
	socket->_policy = get_policy(type);
	socket->start_writer();
	if (first)
	{
		socket->send(first);
	}
	socket->_lock.unlock();
	socket->register_socket();
	_sockets.insert(socket);

	return socket;
}

Connection* SimpleMessenger::get_connection(const entity_inst_t& dest)
{
	Mutex::Locker locker(_lock);
	if (_entity._addr == dest._addr)
	{
		return _local_connection;
	}

	while (true)
	{
		Socket* socket = lookup_socket(dest._addr);
		if (socket)
		{
		}
		else
		{
			socket = connect_rank(dest._addr, dest._name.type(), NULL, NULL);
		}
		
		Mutex::Locker locker(socket->_lock);
		if (socket->_connection_state)
		{
			return socket->_connection_state;
		}
	}
}

void SimpleMessenger::submit_message(Message* m, SocketConnection* con, const entity_addr_t& dest_addr, int dest_type, bool already_locked)
{
	DEBUG_LOG("SimpleMessenger submit_message");

	if (con)
	{
		DEBUG_LOG("connection already exist");
		
		Socket* socket = NULL;
		bool ok = static_cast<SocketConnection*>(con)->try_get_socket(&socket);
		if (!ok)
		{
			m->dec();
			return;
		}
		
		while (socket && ok)
		{
			socket->_lock.lock();
			if (socket->_state != Socket::SOCKET_CLOSED)
			{
				socket->send(m);
				socket->_lock.unlock();
				socket->dec();
				return;
			}
			Socket* current_socket;
			ok = con->try_get_socket(&current_socket);
			socket->_lock.unlock();
			if (current_socket == socket)
			{
				socket->dec();
				current_socket->dec();
				m->dec();
				return;
			}
			else
			{
				socket->dec();
				socket = current_socket;
			}
		}
	}

	if (_entity._addr == dest_addr)
	{
		DEBUG_LOG("dest addr is local");
		
		m->set_connection(static_cast<Connection*>(_local_connection->get()));
		_dispatch_queue.local_delivery(m, m->get_priority());
		return;
	}

	const Policy& policy = get_policy(dest_type);
	if (policy._server)
	{
		m->dec();
	}
	else
	{
		if (!already_locked)
		{
			Mutex::Locker locker(_lock);
			submit_message(m, con, dest_addr, dest_type, true);
		}
		else
		{
			connect_rank(dest_addr, dest_type, static_cast<SocketConnection*>(con), m);
		}
	}
}

int SimpleMessenger::send_keepalive(Connection* con)
{
	int ret = 0;
	Socket* socket = static_cast<Socket*>(static_cast<SocketConnection*>(con)->get_socket());
	if (socket)
	{
		socket->_lock.lock();
		socket->send_keepalive();
		socket->_lock.unlock();
		socket->dec();
	}
	else
	{
		ret = -EPIPE;
	}
	
	return ret;
}

void SimpleMessenger::wait()
{
	{
		Mutex::Locker locker(_lock);
		if (!_started)
		{
			return;
		}
		
		if (!_stopped)
		{
			_stop_cond.wait(_lock);
		}
	}

	if (_did_bind)
	{
		_accepter.stop();
		_did_bind = false;
	}

	_dispatch_queue.shutdown();
	if (_dispatch_queue.is_started())
	{
		_dispatch_queue.wait();
		_dispatch_queue.discard_local();
	}

	if (_reaper_started)
	{
	    Mutex::Locker locker(_lock);
		_reaper_cond.signal();
	    _reaper_stop = true;

	    _reaper_thread.join();
	    _reaper_started = false;
	}

	Mutex::Locker locker(_lock);
	while (!_rank_socket.empty())
	{
		Socket* socket = _rank_socket.begin()->second;
		socket->unregister_socket();
		socket->_lock.lock();
		socket->stop_and_wait();
		SocketConnection* con = socket->_connection_state;
		
		if (con)
		{
			con->clear_socket(socket);
		}
		
		socket->_lock.unlock();
    }

	reaper();
    while (!_sockets.empty())
	{
		_reaper_cond.wait(_lock);
		reaper();
    }

	_started = false;
}


void SimpleMessenger::mark_down_all()
{
	Mutex::Locker locker(_lock);
	for (std::set<Socket*>::iterator iter = _accepting_sockets.begin(); iter != _accepting_sockets.end(); ++iter)
	{
		Socket* socket = *iter;
		socket->_lock.lock();
		socket->stop();
		SocketConnection* con = socket->_connection_state;
		if (con && con->clear_socket(socket))
		{
			_dispatch_queue.queue_reset(static_cast<Connection*>(con->get()));
		}
		
		socket->_lock.unlock();
	}
	_accepting_sockets.clear();

	while (!_rank_socket.empty())
	{
		// std::unordered_map<entity_addr_t, Socket*>::iterator iter = _rank_socket.begin();
		std::map<entity_addr_t, Socket*>::iterator iter = _rank_socket.begin();
	    Socket* socket = iter->second;
	    _rank_socket.erase(iter);
	    socket->unregister_socket();
	    socket->_lock.lock();
	    socket->stop();
	    SocketConnection* con = socket->_connection_state;
	    if (con && con->clear_socket(socket))
	    {
			_dispatch_queue.queue_reset(static_cast<Connection*>(con->get()));
	    }
	    socket->_lock.unlock();
	}
}

void SimpleMessenger::mark_down(const entity_addr_t& addr)
{
	Mutex::Locker locker(_lock);
	Socket* socket = lookup_socket(addr);
	if (socket)
	{
		socket->unregister_socket();
		socket->_lock.lock();
		socket->stop();
		if (socket->_connection_state)
		{
			SocketConnection* con = socket->_connection_state;
			if (con && con->clear_socket(socket))
			{
				_dispatch_queue.queue_reset(static_cast<Connection*>(con->get()));
			}
		}
		socket->_lock.unlock();
	}
	else
	{
	}
}

void SimpleMessenger::mark_down(Connection* con)
{
	if (NULL == con)
	{
		return;
	}

	Mutex::Locker locker(_lock);
	Socket* socket = static_cast<SocketConnection*>(con)->get_socket();
	if (socket)
	{
	    socket->unregister_socket();
	    socket->_lock.lock();
	    socket->stop();
	    if (socket->_connection_state)
		{
			socket->_connection_state->clear_socket(socket);
	    }
	    socket->_lock.unlock();
	    socket->dec();
	}
	else
	{
	}
}

void SimpleMessenger::mark_disposable(Connection* con)
{
	Mutex::Locker locker(_lock);
	Socket* socket = static_cast<SocketConnection*>(con)->get_socket();
	if (socket)
	{
		socket->_lock.lock();
		socket->_policy._lossy = true;
		socket->_lock.unlock();
		socket->dec();
	}
	else
	{
	}
}

void SimpleMessenger::init_local_connection()
{
	_local_connection->_peer_addr = _entity._addr;
	_local_connection->_peer_type = _entity._name.type();
	ms_deliver_handle_fast_connect(dynamic_cast<Connection*>(_local_connection->get()));
}

