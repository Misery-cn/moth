#include <netinet/tcp.h>
#include <poll.h>
#include "socket.h"
#include "msg_types.h"
#include "crc32.h"
#include "simple_messenger.h"
// SYS_NS_BEGIN

#define SEQ_MASK  0x7fffffff

Socket::Socket(SimpleMessenger* msgr, int st, SocketConnection* con)
		: _reader_thread(this), _writer_thread(this), _delay_thread(NULL), _msgr(msgr),
		_conn_id(msgr->_dispatch_queue.get_id()), _recv_ofs(0), _recv_len(0), _fd(-1),
		_port(0), _peer_type(-1), _lock(), _state(st), _connection_state(NULL), 
		_reader_running(false), _reader_needs_join(false), _reader_dispatching(false),
		_notify_on_dispatch_done(false), _writer_running(false), _in_q(&(msgr->_dispatch_queue)),
		_send_keepalive(false), _send_keepalive_ack(false), _connect_seq(0), _peer_global_seq(0),
		_out_seq(0), _in_seq(0), _in_seq_acked(0)
{
	if (con)
	{
		_connection_state = con;
		_connection_state->reset_socket(this);
	}
	else
	{
		_connection_state = new SocketConnection(msgr);
		_connection_state->_socket = get();
	} 

	// ms
	_msgr->_timeout = SOCKET_TIMEOUT * 1000;

	// 预取缓冲
	_recv_max_prefetch = IO_BUFFER_MAX;
	_recv_buf = new char[_recv_max_prefetch];
}

Socket::~Socket()
{
	DELETE_P(_delay_thread);
	DELETE_ARRAY(_recv_buf);
}

int Socket::open() throw (SysCallException)
{
    // 创建一个套接字
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 >= _fd)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "socket");
    }

    return 0;
}

int Socket::close() throw (SysCallException)
{
    // 关闭连接
    shutdown(_fd, SHUT_RDWR);
    // 清理套接字
    if (0 != close(_fd))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "close");
    }
	
	_fd = -1;
	
    return 0;
}

int Socket::close(size_t fd) throw (SysCallException)
{
    // 关闭连接
    shutdown(fd, SHUT_RDWR);
    // 清理套接字
    if (0 != close(fd))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "close");
    }
	
	fd = -1;
	
    return 0;
}

int Socket::bind(char* ip, int port) throw (SysCallException)
{
    int	op = 1;
    struct sockaddr_in addr;

    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if ((NULL == ip) || (MIN_IP_LEN > strlen(ip)))
    {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        addr.sin_addr.s_addr = inet_addr(ip);
    }

    // 允许在bind过程中本地地址可重复使用
    // 成功返回0，失败返回-1
    if (0 > setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)))
    {
        return -1;
    }

    // 给套接口分配协议地址,如果前面没有设置地址可重复使用,则要判断errno=EADDRINUSE,即地址已使用
    // 成功返回0，失败返回-1
    if (0 > ::bind(_fd, (struct sockaddr*)&addr, sizeof(addr)))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "bind");
    }

    return 0;
}

int Socket::listen() throw (SysCallException)
{
    if (0 != ::listen(_fd, MAX_CONN))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "listen");
    }

    return 0;
}

int Socket::accept(int* fd, struct sockaddr_in* addr) throw (SysCallException)
{
    int newfd = -1;
    int size = sizeof(struct sockaddr_in);

    newfd = ::accept(_fd, (struct sockaddr*)addr, (socklen_t*)&size);
    if (0 > newfd)
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "accept");
    }

    *fd = newfd;

    return 0;
}

int Socket::connect(char* ip, int port) throw (SysCallException)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    // Socket连接
    if (0 != ::connect(_fd, (struct sockaddr*)&addr, sizeof(addr)))
    {
        THROW_SYSCALL_EXCEPTION(NULL, errno, "connect");
    }

    return 0;
}

int Socket::read(int fd, char* buf, int len) throw (SysCallException)
{
    /*
	fd_set readfds;
    fd_set errfds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&readfds);
    // 打开sockfd的可读位
    FD_SET(fd, &readfds);

    // 初始化
    FD_ZERO(&errfds);
    // 设置sockfd的异常位
    FD_SET(fd, &errfds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    // 检测套接字状态
    if (select(fd + 1, &readfds, NULL, &errfds, &tv))
    {
        // 判断描述字sockfd的异常位是否打开,如果打开则表示产生错误
        if (FD_ISSET(fd, &errfds))
        {
            return SOCKET_READ_ERROR;
        }

        // 判断描述字sockfd的可读位是否打开,如果打开则读取数据
        if (FD_ISSET(fd, &readfds))
        {
            // 读数据
            // 返回实际所读的字节数
            if (0 > read(fd, buff, len))
            {
                return SOCKET_READ_ERROR;
            }
            else
            {
				return NO_ERROR;
            }
        }
    }
	*/
	
	// 读数据
	// 返回实际所读的字节数
	if (0 > read(fd, buf, len))
	{
		if ((EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
			THROW_SYSCALL_EXCEPTION(NULL, errno, "read");
        }
	}
	else
	{
		return 0;
	}
}

int Socket::write(int fd, char* buf) throw (SysCallException)
{
	/*
    fd_set writefds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&writefds);
    // 打开fd的可写位
    FD_SET(fd, &writefds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    int bufLen = strlen(buff);

    // 检测套接字状态
    if (select(fd + 1, NULL, &writefds, 0, &tv))
    {
        // 判断描述字sockfd的可写位是否打开,如果打开则写入数据
        if (FD_ISSET(fd, &writefds))
        {
            // 写入数据
            // 返回所写字节数
            int wl = write(fd, buff, bufLen);
            if (0 > wl)
            {
                return SOCKET_WRITE_ERROR;
            }
            else
            {
                if (wl == bufLen)
                {
                    return NO_ERROR;
                }
                else
                {
                    return SOCKET_WRITE_ERROR;
                }
            }
        }
    }
	*/
	
	// 写入数据
	// 返回所写字节数
	int bufLen = strlen(buf);
	int wl = ::write(fd, buf, bufLen);
	if (0 > wl)
	{
		if ((EAGAIN != errno) && (EWOULDBLOCK != errno))
        {
			THROW_SYSCALL_EXCEPTION(NULL, errno, "write");
        }
	}
	else
	{
		if (wl == bufLen)
		{
			return 0;
		}
		else
		{
			THROW_SYSCALL_EXCEPTION(NULL, errno, "write");
		}
	}
}

void Socket::handle_ack(uint64_t seq)
{
	while (!_sent.empty() && _sent.front()->get_seq() <= seq)
	{
		Message* m = _sent.front();
		_sent.pop_front();
		m->dec();
	}
}

void Socket::start_reader()
{
	if (_reader_needs_join)
	{
		_reader_thread.join();
		_reader_needs_join = false;
	}
	
	_reader_running = true;
	_reader_thread.create();
}

void Socket::maybe_start_delay_thread()
{
	if (!_delay_thread)
	{
		// config
	    uint32_t pos = std::string("").find(entity_type_name(_connection_state->_peer_type));
	    if (pos != std::string::npos)
		{
			_delay_thread = new DelayedDelivery(this);
			_delay_thread->create();
	    }
	}
}

void Socket::start_writer()
{
	_writer_running = true;
	_writer_thread.create();
}

void Socket::join_reader()
{
	if (!_reader_running)
	{
		return;
	}
	
	_cond.signal();

	_lock.unlock();
	_reader_thread.join();
	_lock.lock();
	_reader_needs_join = false;
}


/*
svr                      cli
                    <----connect
       accept<----
  send banner---->
					---->read banner
                    <----send banner
  read banner<----
  
  send cli info---->
                     ---->read cli info
					 <----send cli info
  read cli info<----
*/

int Socket::accept()
{
	DEBUG_LOG("socket accepting");
	
	_lock.unlock();

	buffer addrs;
	entity_addr_t socket_addr;
	socklen_t len;
	int r;
	char banner[strlen("moth")+1];
	buffer addrbl;
	msg_connect connect;
	msg_connect_reply reply;
	Socket* existing = NULL;
	buffer bp;
	uint64_t feat_missing;
	bool replaced = false;
	bool is_reset_from_peer = false;
	int removed;

	int reply_tag = 0;
	uint64_t existing_seq = -1;
	
	uint64_t newly_acked_seq = 0;

	recv_reset();

	set_socket_options();

	r = tcp_write("moth", strlen("moth"));
	if (0 > r)
	{
		ERROR_LOG("socket accept write banner failed");
		goto fail_unlocked;
	}

	DEBUG_LOG("accept send banner successful");

	::encode(_msgr->_entity._addr, addrs);

	_port = _msgr->_entity._addr.get_port();

	sockaddr_storage ss;
	len = sizeof(ss);
	r = ::getpeername(_fd, (sockaddr*)&ss, &len);
	if (0 > r)
	{
		ERROR_LOG("socket accept get client info failed");
		goto fail_unlocked;
	}
	socket_addr.set_sockaddr((sockaddr*)&ss);
	::encode(socket_addr, addrs);

	r = tcp_write(addrs.c_str(), addrs.length());
	if (0 > r)
	{
		ERROR_LOG("socket accept write client info failed");
		goto fail_unlocked;
	}

	DEBUG_LOG("accept send client info successful");
  
	if (0 > tcp_read(banner, strlen("moth")))
	{
		ERROR_LOG("socket accept read banner failed");
		goto fail_unlocked;
	}
	
	if (memcmp(banner, "moth", strlen("moth")))
	{
		banner[strlen("moth")] = 0;
		goto fail_unlocked;
	}

	DEBUG_LOG("accept read banner successful");
	
	{
		ptr tp(sizeof(entity_addr_t));
		addrbl.push_back(std::move(tp));
	}
	
	if (0 > tcp_read(addrbl.c_str(), addrbl.length()))
	{
		ERROR_LOG("socket accept read client info failed");
		goto fail_unlocked;
	}
	
	{
		buffer::iterator ti = addrbl.begin();
		::decode(_peer_addr, ti);
	}

	if (_peer_addr.is_blank_ip())
	{
		int port = _peer_addr.get_port();
		_peer_addr._addr = socket_addr._addr;
		_peer_addr.set_port(port);
	}
	
	set_peer_addr(_peer_addr);

	DEBUG_LOG("accept read client info successful");
  
	while (1)
	{
		if (0 > tcp_read((char*)&connect, sizeof(connect)))
		{
			ERROR_LOG("socket accept read connect info failed");
			
			goto fail_unlocked;
		}

		DEBUG_LOG("socket accept read connect info successful");
    
		_msgr->_lock.lock();
		_lock.lock();
		if (_msgr->_dispatch_queue._stop)
		{
			goto shutting_down;
		}
		
		if (_state != SOCKET_ACCEPTING)
		{
			goto shutting_down;
		}

		set_peer_type(connect.host_type);
		_policy = _msgr->get_policy(connect.host_type);

		memset(&reply, 0, sizeof(reply));
		reply.protocol_version = 0;
		
		_msgr->_lock.unlock();

		if (connect.protocol_version != reply.protocol_version)
		{
			ERROR_LOG("error version");
			reply.tag = MSGR_TAG_BADPROTOVER;
			goto reply;
		}
    
		_lock.unlock();

	retry_existing_lookup:
		_msgr->_lock.lock();
		_lock.lock();
		if (_msgr->_dispatch_queue._stop)
		{
			goto shutting_down;
		}
		
		if (_state != SOCKET_ACCEPTING)
		{
			goto shutting_down;
		}
    
		existing = _msgr->lookup_socket(_peer_addr);
		if (existing)
		{
			DEBUG_LOG("socket accept existing connection");
			
			existing->_lock.lock();
			if (existing->_reader_dispatching)
			{
				DEBUG_LOG("reader dispatching");
				
				existing->get();
				_lock.unlock();
				_msgr->_lock.unlock();
				existing->_notify_on_dispatch_done = true;
				while (existing->_reader_dispatching)
				{
					existing->_cond.wait(existing->_lock);
				}
				
				existing->_lock.unlock();
				existing->dec();
				existing = NULL;
				goto retry_existing_lookup;
			}

			if (connect.global_seq < existing->_peer_global_seq)
			{
				DEBUG_LOG("global_seq repy");
				
				reply.tag = MSGR_TAG_RETRY_GLOBAL;
				reply.global_seq = existing->_peer_global_seq;
				existing->_lock.unlock();
				_msgr->_lock.unlock();
				goto reply;
			}
			else
			{
			}
      
			if (existing->_policy._lossy)
			{
				DEBUG_LOG("lossy");
				
				existing->was_session_reset();
				goto replace;
			}

			if (0 == connect.connect_seq &&  0 < existing->_connect_seq)
			{
				DEBUG_LOG("connect_seq is 0");
				
				is_reset_from_peer = true;
				
				if (_policy._resetcheck)
				{
					existing->was_session_reset();
				}
				
				goto replace;
			}

			if (connect.connect_seq < existing->_connect_seq)
			{
				DEBUG_LOG("connect.connect_seq < existing->_connect_seq");
				
				goto retry_session;
			}

			if (connect.connect_seq == existing->_connect_seq)
			{
				DEBUG_LOG("connect.connect_seq == existing->_connect_seq");
				
				if (existing->_state == SOCKET_OPEN || existing->_state == SOCKET_STANDBY)
				{
					DEBUG_LOG("existing->_state == SOCKET_OPEN || existing->_state == SOCKET_STANDBY");
					
					goto retry_session;
				}

				if (_peer_addr < _msgr->_entity._addr || existing->_policy._server)
				{
					DEBUG_LOG("_peer_addr < _msgr->_entity._addr || existing->_policy._server");
					
					if (!(existing->_state == SOCKET_CONNECTING || existing->_state == SOCKET_WAIT))
					{
						DEBUG_LOG("replace");
						
						goto replace;
					}
				}
				else
				{
					DEBUG_LOG("_peer_addr > _msgr->_entity._addr && !existing->_policy._server");
					
					if (!(existing->_state == SOCKET_CONNECTING))
					{
					}
					existing->send_keepalive();
					reply.tag = MSGR_TAG_WAIT;
					existing->_lock.unlock();
					_msgr->_lock.unlock();
					goto reply;
				}
			}
			
			if (_policy._resetcheck && 0 == existing->_connect_seq)
			{
				DEBUG_LOG("_policy._resetcheck && 0 == existing->_connect_seq");
				
				reply.tag = MSGR_TAG_RESETSESSION;
				_msgr->_lock.unlock();
				existing->_lock.unlock();
				goto reply;
			}

			DEBUG_LOG("goto replace");
			
			goto replace;
		} 
		else if (0 < connect.connect_seq)
		{
			DEBUG_LOG("0 < connect.connect_seq");
			
			_msgr->_lock.unlock();
			reply.tag = MSGR_TAG_RESETSESSION;
			goto reply;
		}
		else
		{
			DEBUG_LOG("goto open");
			
			existing = NULL;
			goto open;
		}

		abort();

	retry_session:
		reply.tag = MSGR_TAG_RETRY_SESSION;
		reply.connect_seq = existing->_connect_seq + 1;
		existing->_lock.unlock();
		_msgr->_lock.unlock();
		goto reply;    

	reply:
		_lock.unlock();
		r = tcp_write((char*)&reply, sizeof(reply));
		if (0 > r)
		{
			goto fail_unlocked;
		}
	}
  
replace:
	existing->stop();
	existing->unregister_socket();
	replaced = true;

	if (existing->_policy._lossy)
	{
		DEBUG_LOG("existing->_policy._lossy");
		
		if (existing->_connection_state->clear_socket(existing))
		{
			_msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(existing->_connection_state->get()));
		}
	}
	else
	{
		DEBUG_LOG("!existing->_policy._lossy");
		
		_msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
		_connection_state = existing->_connection_state;

		_connection_state->reset_socket(this);

		if (existing->_delay_thread)
		{
			existing->_delay_thread->steal_for_socket(this);
			_delay_thread = existing->_delay_thread;
			existing->_delay_thread = NULL;
			_delay_thread->flush();
		}

		uint64_t replaced_conn_id = _conn_id;
		_conn_id = existing->_conn_id;
		existing->_conn_id = replaced_conn_id;

		_in_seq = is_reset_from_peer ? 0 : existing->_in_seq;
		_in_seq_acked = _in_seq;

		existing->requeue_sent();
		_out_seq = existing->_out_seq;

		for (std::map<int, std::list<Message*> >::iterator iter = existing->_out_q.begin(); iter != existing->_out_q.end(); ++iter)
		{
			_out_q[iter->first].splice(_out_q[iter->first].begin(), iter->second);
		}
	}
	existing->stop_and_wait();
	existing->_lock.unlock();

open:
	_connect_seq = connect.connect_seq + 1;
	_peer_global_seq = connect.global_seq;
	_state = SOCKET_OPEN;

	reply.tag = (reply_tag ? reply_tag : MSGR_TAG_READY);
	reply.global_seq = _msgr->get_global_seq();
	reply.connect_seq = _connect_seq;
	reply.flags = 0;
	if (_policy._lossy)
	{
		reply.flags = reply.flags | MSG_CONNECT_LOSSY;
	}

	_msgr->_dispatch_queue.queue_accept(static_cast<Connection*>(_connection_state->get()));
	_msgr->ms_deliver_handle_fast_accept(static_cast<Connection*>(_connection_state->get()));

	if (_msgr->_dispatch_queue._stop)
	{
		goto shutting_down;
	}
	
	removed = _msgr->_accepting_sockets.erase(this);
	register_socket();
	_msgr->_lock.unlock();
	_lock.unlock();

	r = tcp_write((char*)&reply, sizeof(reply));
	if (0 > r)
	{
		goto fail_registered;
	}

	if (reply_tag == MSGR_TAG_SEQ)
	{
		if (0 > tcp_write((char*)&existing_seq, sizeof(existing_seq)))
		{
			goto fail_registered;
		}
		
		if (0 > tcp_read((char*)&newly_acked_seq, sizeof(newly_acked_seq)))
		{
			goto fail_registered;
		}
	}

	_lock.lock();
	discard_requeued_up_to(newly_acked_seq);
	if (_state != SOCKET_CLOSED)
	{
		start_writer();
	}

	maybe_start_delay_thread();

	return 0;

fail_registered:
	;

fail_unlocked:

	_lock.lock();
	
	if (_state != SOCKET_CLOSED)
	{	
		bool queued = is_queued();

		if (queued)
		{
			_state = _policy._server ? SOCKET_STANDBY : SOCKET_CONNECTING;
		}
		else if (replaced)
		{
			_state = SOCKET_STANDBY;
		}
		else
		{
			_state = SOCKET_CLOSED;
			atomic_set(&_state_closed, 1);
		}
		
		fault();
		
		if (queued || replaced)
		{
			start_writer();
		}
	}
	
	return -1;

shutting_down:

	_msgr->_lock.unlock();
	
shutting_down_msgr_unlocked:

	_state = SOCKET_CLOSED;
	atomic_set(&_state_closed, 1);
	fault();
	return -1;
}

void Socket::set_socket_options()
{
	// 禁用Nagle’s Algorithm
	int flag = 1;
	int r = ::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
	if (0 > r)
	{
		r = -errno;
	}

	// 设置滑动窗口大小
	/*
	int size = 0;
	int r = ::setsockopt(_fd, SOL_SOCKET, SO_RCVBUF, (void*)&size, sizeof(size));
	if (0 > r)
	{
		r = -errno;
	}
	*/


#if defined(SO_NOSIGPIPE)
	int val = 1;
	int r = ::setsockopt(_fd, SOL_SOCKET, SO_NOSIGPIPE, (void*)&val, sizeof(val));
	if (r)
	{
		r = -errno;
	}
#endif

#ifdef SO_PRIORITY
	int prio = _msgr->get_socket_priority();
	if (0 <= prio)
	{
		int r = -1;
	#ifdef IPTOS_CLASS_CS6
		int iptos = IPTOS_CLASS_CS6;
		int addr_family = 0;
		if (!_peer_addr.is_blank_ip())
		{
			addr_family = _peer_addr.get_family();
		}
		else
		{
			addr_family = _msgr->get_entity_addr().get_family();
		}
		
		switch (addr_family)
		{
			case AF_INET:
			{
				r = ::setsockopt(sd, IPPROTO_IP, IP_TOS, &iptos, sizeof(iptos));
				break;
			}
			case AF_INET6:
			{
				r = ::setsockopt(sd, IPPROTO_IPV6, IPV6_TCLASS, &iptos, sizeof(iptos));
				break;
			}
			default:
				return;
		}
		
		if (0 > r)
		{
			r = -errno;
		}
	#endif
		r = ::setsockopt(_fd, SOL_SOCKET, SO_PRIORITY, &prio, sizeof(prio));
		if (0 > r)
		{
			r = -errno;
		}
	}
#endif
}

int Socket::connect()
{
	DEBUG_LOG("socket connecting");
	
	bool got_bad_auth = false;

	uint32_t cseq = _connect_seq;
	uint32_t gseq = _msgr->get_global_seq();

	join_reader();

	_lock.unlock();
  
	char tag = -1;
	int rc = -1;
	struct msghdr msg;
	struct iovec msgvec[2];
	int msglen;
	char banner[strlen("moth") + 1];
	entity_addr_t paddr;
	entity_addr_t peer_addr_for_me, socket_addr;
	buffer addrbl;
	buffer myaddrbl;

	if (0 <= _fd)
	{
		::close(_fd);
	}

	_fd = ::socket(_peer_addr.get_family(), SOCK_STREAM, 0);
	if (0 > _fd)
	{
		rc = -errno;
		goto fail;
	}

	recv_reset();

	set_socket_options();

	rc = ::connect(_fd, (sockaddr*)&_peer_addr._addr, _peer_addr.addr_size());
	if (0 > rc)
	{
		ERROR_LOG("connect %s faild", inet_ntoa(((sockaddr_in*)&_peer_addr._addr)->sin_addr));
		
		int stored_errno = errno;
		if (stored_errno == ECONNREFUSED)
		{
			_msgr->_dispatch_queue.queue_refused(static_cast<Connection*>(_connection_state->get()));
		}
		goto fail;
	}

	rc = tcp_read((char*)&banner, strlen("moth"));
	if (0 > rc)
	{
		ERROR_LOG("socket connect read banner failed");
		goto fail;
	}
	
	if (memcmp(banner, "moth", strlen("moth")))
	{
		goto fail;
	}

	DEBUG_LOG("connect read banner successful");

	memset(&msg, 0, sizeof(msg));
	msgvec[0].iov_base = banner;
	msgvec[0].iov_len = strlen("moth");
	msg.msg_iov = msgvec;
	msg.msg_iovlen = 1;
	msglen = msgvec[0].iov_len;
	rc = do_sendmsg(&msg, msglen);
	if (0 > rc)
	{
		ERROR_LOG("socket connect send banner failed");
		goto fail;
	}

	DEBUG_LOG("connect send banner successful");

	{
		#if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
			ptr p(sizeof(entity_addr_t) * 2);
		#else
			int wirelen = sizeof(__u32) * 2 + sizeof(sock_addr_storage);
	    	ptr p(wirelen * 2);
		#endif
			addrbl.push_back(std::move(p));
	}
	
	rc = tcp_read(addrbl.c_str(), addrbl.length());
	if (0 > rc)
	{
		ERROR_LOG("socket connect read client info failed");
		goto fail;
	}

	DEBUG_LOG("connect read client info successful");
	
	try
	{
		buffer::iterator p = addrbl.begin();
		::decode(paddr, p);
		::decode(peer_addr_for_me, p);
	}
	catch (...)
	{
		ERROR_LOG("decode client info failed");
		goto fail;
	}
	
	_port = peer_addr_for_me.get_port();

	if (_peer_addr != paddr)
	{	
		if (paddr.is_blank_ip() && _peer_addr.get_port() == paddr.get_port() && _peer_addr.get_nonce() == paddr.get_nonce())
		{
			ERROR_LOG("same node");
		}
		else
		{
			ERROR_LOG("not same node");
			goto fail;
		}
  	}
	
	::encode(_msgr->_entity._addr, myaddrbl);

	memset(&msg, 0, sizeof(msg));
	msgvec[0].iov_base = myaddrbl.c_str();
	msgvec[0].iov_len = myaddrbl.length();
	msg.msg_iov = msgvec;
	msg.msg_iovlen = 1;
	msglen = msgvec[0].iov_len;
	rc = do_sendmsg(&msg, msglen);
	if (0 > rc)
	{
		ERROR_LOG("socket connect send client info failed");
		goto fail;
	}

	DEBUG_LOG("connect send client info successful");

	while (1)
	{
		msg_connect connect;
		connect.host_type = _msgr->get_entity()._name.type();
		connect.global_seq = gseq;
		connect.connect_seq = cseq;
		connect.protocol_version = 0;
		connect.flags = 0;
		
		if (_policy._lossy)
		{
			connect.flags |= MSG_CONNECT_LOSSY;
		}
		
		memset(&msg, 0, sizeof(msg));
		msgvec[0].iov_base = (char*)&connect;
		msgvec[0].iov_len = sizeof(connect);
		msg.msg_iov = msgvec;
		msg.msg_iovlen = 1;
		msglen = msgvec[0].iov_len;

		rc = do_sendmsg(&msg, msglen);
		if (0 > rc)
		{
			ERROR_LOG("connect send connect info failed");
			goto fail;
		}

		DEBUG_LOG("connect send connect info successful");

		msg_connect_reply reply;
		rc = tcp_read((char*)&reply, sizeof(reply));
		if (0 > rc)
		{
			ERROR_LOG("connect read connect reply failed");
			goto fail;
		}

		DEBUG_LOG("connect read connect reply successful");

		_lock.lock();
		if (_state != SOCKET_CONNECTING)
		{
			goto stop_locked;
		}

		if (reply.tag == MSGR_TAG_FEATURES)
		{
			goto fail_locked;
		}

		if (reply.tag == MSGR_TAG_BADPROTOVER)
		{
			goto fail_locked;
		}

		if (reply.tag == MSGR_TAG_BADAUTHORIZER)
		{
			if (got_bad_auth)
			{
				goto stop_locked;
			}
			
			got_bad_auth = true;
			_lock.unlock();
			continue;
		}
		
		if (reply.tag == MSGR_TAG_RESETSESSION)
		{
			was_session_reset();
			cseq = 0;
			_lock.unlock();
			continue;
		}
		
		if (reply.tag == MSGR_TAG_RETRY_GLOBAL)
		{
			gseq = _msgr->get_global_seq(reply.global_seq);
			_lock.unlock();
			continue;
		}
		
		if (reply.tag == MSGR_TAG_RETRY_SESSION)
		{
			cseq = _connect_seq = reply.connect_seq;
			_lock.unlock();
			continue;
		}

		if (reply.tag == MSGR_TAG_WAIT)
		{
			_state = SOCKET_WAIT;
			goto stop_locked;
		}

		if (reply.tag == MSGR_TAG_READY || reply.tag == MSGR_TAG_SEQ)
		{
			if (reply.tag == MSGR_TAG_SEQ)
			{
				uint64_t newly_acked_seq = 0;
				rc = tcp_read((char*)&newly_acked_seq, sizeof(newly_acked_seq));
				if (0 > rc)
				{
					goto fail_locked;
				}

				while (newly_acked_seq > _out_seq)
				{
					Message *m = get_next_outgoing();
					m->dec();
					++_out_seq;
				}
				
				if (0 > tcp_write((char*)&_in_seq, sizeof(_in_seq)))
				{
					goto fail_locked;
				}
			}

			_peer_global_seq = reply.global_seq;
			_policy._lossy = reply.flags & MSG_CONNECT_LOSSY;
			_state = SOCKET_OPEN;
			_connect_seq = cseq + 1;
			
			_backoff = utime_t();
			_msgr->_dispatch_queue.queue_connect(static_cast<Connection*>(_connection_state->get()));
			_msgr->ms_deliver_handle_fast_connect(static_cast<Connection*>(_connection_state->get()));
      
			if (!_reader_running)
			{
				start_reader();
			}
			
			maybe_start_delay_thread();
			
			return 0;
		}
    
		goto fail_locked;
	}

fail:

	_lock.lock();
	
fail_locked:

	if (_state == SOCKET_CONNECTING)
	{
		fault();
	}
	else
	{
		ERROR_LOG("connect fault, but current state is not connecting");
	}

stop_locked:
	return rc;
}

void Socket::register_socket()
{
	Socket* existing = _msgr->lookup_socket(_peer_addr);
	_msgr->_rank_socket[_peer_addr] = this;
}

void Socket::unregister_socket()
{
	// std::unordered_map<entity_addr_t, Socket*>::iterator iter = _msgr->_rank_socket.find(_peer_addr);
	std::map<entity_addr_t, Socket*>::iterator iter = _msgr->_rank_socket.find(_peer_addr);
	if (iter != _msgr->_rank_socket.end() && iter->second == this)
	{
		_msgr->_rank_socket.erase(iter);
	}
	else
	{
		_msgr->_accepting_sockets.erase(this);
	}
}

void Socket::join()
{
	if (_writer_thread.is_started())
	{
		_writer_thread.join();
	}
	
	if (_reader_thread.is_started())
	{
		_reader_thread.join();
	}
	
	if (_delay_thread)
	{
		_delay_thread->stop();
		_delay_thread->join();
	}
}

void Socket::requeue_sent()
{
	if (_sent.empty())
	{
		return;
	}

	std::list<Message*>& rq = _out_q[MSG_PRIO_HIGHEST];
	while (!_sent.empty())
	{
		Message* m = _sent.back();
		_sent.pop_back();
		rq.push_front(m);
		_out_seq--;
	}
}

void Socket::discard_requeued_up_to(uint64_t seq)
{
	if (_out_q.count(MSG_PRIO_HIGHEST) == 0)
	{
		return;
	}
	
	std::list<Message*>& rq = _out_q[MSG_PRIO_HIGHEST];
	while (!rq.empty())
	{
		Message* m = rq.front();
		if (m->get_seq() == 0 || m->get_seq() > seq)
		{
			break;
		}
		m->dec();
		rq.pop_front();
		_out_seq++;
	}
	
	if (rq.empty())
	{
		_out_q.erase(MSG_PRIO_HIGHEST);
	}
}

void Socket::discard_out_queue()
{
	for (std::list<Message*>::iterator iter = _sent.begin(); iter != _sent.end(); ++iter)
	{
		(*iter)->dec();
	}
	
	_sent.clear();
	
	for (std::map<int, std::list<Message*> >::iterator iter = _out_q.begin(); iter != _out_q.end(); ++iter)
	{
		for (std::list<Message*>::iterator r = iter->second.begin(); r != iter->second.end(); ++r)
		{
			(*r)->dec();
		}
	}
	
	_out_q.clear();
}

void Socket::fault(bool onread)
{
	DEBUG_LOG("socket fault, current state is %s", get_state_name(_state));
	
	_cond.signal();

	if (onread && _state == SOCKET_CONNECTING)
	{
		return;
	}

	if (_state == SOCKET_CLOSED || _state == SOCKET_CLOSING)
	{
		if (_connection_state->clear_socket(this))
		{
			_msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
		}
		return;
	}

	shutdown_socket();

	if (_policy._lossy && _state != SOCKET_CONNECTING)
	{
		stop();
		bool cleared = _connection_state->clear_socket(this);
		
		_lock.unlock();

		_msgr->_lock.lock();
		_lock.lock();
		unregister_socket();
		_msgr->_lock.unlock();

		if (_delay_thread)
		{
			_delay_thread->discard();
		}
		
		_in_q->discard_queue(_conn_id);
		discard_out_queue();
		if (cleared)
		{
			_msgr->_dispatch_queue.queue_reset(static_cast<Connection*>(_connection_state->get()));
		}
		
		return;
	}

	if (_delay_thread)
	{
		_delay_thread->flush();
	}

	requeue_sent();

	if (_policy._standby && !is_queued())
	{
		_state = SOCKET_STANDBY;
		return;
	}

	if (_state != SOCKET_CONNECTING)
	{
		if (_policy._server)
		{
			_state = SOCKET_STANDBY;
		}
		else
		{
			_connect_seq++;
			_state = SOCKET_CONNECTING;
		}
		
		_backoff = utime_t();
	}
	else if (_backoff == utime_t())
	{
		_backoff.set_from_double(.2);
	}
	else
	{
		_cond.timed_wait(_lock, _backoff.to_msec());
	    _backoff += _backoff;
	    if (_backoff > 15.0)
	    {
			_backoff.set_from_double(15.0);
	    }
	}
}

void Socket::was_session_reset()
{
	_in_q->discard_queue(_conn_id);
	
	if (_delay_thread)
	{
		_delay_thread->discard();
	}
	
	discard_out_queue();

	_msgr->_dispatch_queue.queue_remote_reset(static_cast<Connection*>(_connection_state->get()));

	_in_seq = 0;
	_connect_seq = 0;
}

void Socket::stop()
{
	_state = SOCKET_CLOSED;
	atomic_set(&_state_closed, 1);
	_cond.signal();
	shutdown_socket();
}

void Socket::stop_and_wait()
{
	if (_state != SOCKET_CLOSED)
	{
		stop();
	}
  
	if (_delay_thread)
	{
		_lock.unlock();
		_delay_thread->stop_fast_dispatching();
		_lock.lock();
	}
	
	while (_reader_running && _reader_dispatching)
	{
		_cond.wait(_lock);
	}
}

void Socket::reader()
{
	DEBUG_LOG("socket reader start");
	
	_lock.lock();

	if (_state == SOCKET_ACCEPTING)
	{
		accept();
	}

	while (_state != SOCKET_CLOSED && _state != SOCKET_CONNECTING)
	{
		if (_state == SOCKET_STANDBY)
		{
			_cond.wait(_lock);
			continue;
		}

		_lock.unlock();

		char tag = -1;
		if (0 > tcp_read((char*)&tag, 1))
		{
			_lock.lock();
			fault(true);
			continue;
		}

		if (tag == MSGR_TAG_KEEPALIVE)
		{
			_lock.lock();
			_connection_state->set_last_keepalive(clock_now());
			continue;
		}
		
		if (tag == MSGR_TAG_KEEPALIVE2)
		{
			struct timespec t;
			int rc = tcp_read((char*)&t, sizeof(t));
			_lock.lock();
			if (0 > rc)
			{
				fault(true);
			}
			else
			{
				_send_keepalive_ack = true;
				_keepalive_ack_stamp = utime_t(t);
				_connection_state->set_last_keepalive(clock_now());
				_cond.signal();
			}
			
			continue;
		}
		
		if (tag == MSGR_TAG_KEEPALIVE2_ACK)
		{
			struct timespec t;
			int rc = tcp_read((char*)&t, sizeof(t));
			_lock.lock();
			if (0 > rc)
			{
				fault(true);
			}
			else
			{
				_connection_state->set_last_keepalive_ack(utime_t(t));
			}
			
			continue;
		}

		if (tag == MSGR_TAG_ACK)
		{
			le64 seq;
			int rc = tcp_read((char*)&seq, sizeof(seq));
			_lock.lock();
			if (0 > rc)
			{
				fault(true);
			}
			else if (_state != SOCKET_CLOSED)
			{
				handle_ack(seq);
			}
			
			continue;
		}
		else if (tag == MSGR_TAG_MSG)
		{
			Message* m = NULL;
			int r = read_message(&m);

			_lock.lock();
      
			if (!m)
			{
				if (0 > r)
				{
					fault(true);
				}
				
				continue;
			}

			if (_state == SOCKET_CLOSED || _state == SOCKET_CONNECTING)
			{
				_in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
				m->dec();
				continue;
			}
			
			if (m->get_seq() <= _in_seq)
			{
				_in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
				m->dec();
			}
			
			if (m->get_seq() > _in_seq + 1)
			{
				ERROR_LOG("reader missed message? skipped from seq %d to %d", _in_seq, m->get_seq());
			}

			m->set_connection(static_cast<Connection*>(_connection_state->get()));

			_in_seq = m->get_seq();

			_cond.signal();
      
			_in_q->fast_preprocess(m);

			if (_delay_thread)
			{
				utime_t release;				
				_delay_thread->queue(release, m);
			}
			else
			{
				if (_in_q->can_fast_dispatch(m))
				{
					_reader_dispatching = true;
					_lock.unlock();
					_in_q->fast_dispatch(m);
					_lock.lock();
					_reader_dispatching = false;
					if (_state == SOCKET_CLOSED || _notify_on_dispatch_done)
					{
						_notify_on_dispatch_done = false;
						_cond.signal();
					}
				}
				else
				{
					_in_q->enqueue(m, m->get_priority(), _conn_id);
				}
			}
		}
    	else if (tag == MSGR_TAG_CLOSE)
		{
			_lock.lock();
			if (_state == SOCKET_CLOSING)
			{
				_state = SOCKET_CLOSED;
				atomic_set(&_state_closed, 1);
			}
			else
			{
				_state = SOCKET_CLOSING;
			}
			
			_cond.signal();
			break;
		}
		else
		{
			_lock.lock();
			fault(true);
		}
	}
	
	_reader_running = false;
	_reader_needs_join = true;
	unlock_maybe_reap();
}

void Socket::writer()
{
	DEBUG_LOG("socket writer start");
	
	_lock.lock();
	while (_state != SOCKET_CLOSED)
	{
		if (is_queued() && _state == SOCKET_STANDBY && !_policy._server)
		{
			_state = SOCKET_CONNECTING;
		}

		if (_state == SOCKET_CONNECTING)
		{
			DEBUG_LOG("socket state is SOCKET_CONNECTING, will try connect");
			connect();
			continue;
		}
    
		if (_state == SOCKET_CLOSING)
		{
			char tag = MSGR_TAG_CLOSE;
			_state = SOCKET_CLOSED;
			atomic_set(&_state_closed, 1);
			_lock.unlock();
			if (0 <= _fd)
			{
				int r = ::write(_fd, &tag, 1);
				(void)r;
			}
			_lock.lock();
			continue;
		}

		if (_state != SOCKET_CONNECTING && _state != SOCKET_WAIT && _state != SOCKET_STANDBY &&
			(is_queued() || _in_seq > _in_seq_acked))
		{
			if (_send_keepalive)
			{
				int rc;

				_lock.unlock();
				rc = write_keepalive();
				
				_lock.lock();
				if (0 > rc)
				{
					fault();
					continue;
				}
				_send_keepalive = false;
			}
			
			if (_send_keepalive_ack)
			{
				utime_t t = _keepalive_ack_stamp;
				_lock.unlock();
				int rc = write_keepalive();
				_lock.lock();
				if (0 > rc)
				{
					fault();
					continue;
				}
				_send_keepalive_ack = false;
			}

			if (_in_seq > _in_seq_acked)
			{
				uint64_t send_seq = _in_seq;
				_lock.unlock();
				int rc = write_ack(send_seq);
				_lock.lock();
				if (0 > rc)
				{
					fault();
					continue;
				}
				_in_seq_acked = send_seq;
			}

			Message* m = get_next_outgoing();
			if (m)
			{
				m->set_seq(++_out_seq);
				if (!_policy._lossy)
				{
					_sent.push_back(m); 
					m->get();
				}

				m->set_connection(static_cast<Connection*>(_connection_state->get()));

				if (m->empty_payload())
				{
				}
				else
				{
				}

				m->encode(_msgr->_crc_flag);

				const msg_header& header = m->get_header();
				const msg_footer& footer = m->get_footer();

				buffer buf = m->get_payload();
				buf.append(m->get_middle());
				buf.append(m->get_data());

				_lock.unlock();

				int rc = write_message(header, footer, buf);

				_lock.lock();
				if (0 > rc)
				{
					fault();
				}
				m->dec();
			}
			
			continue;
		}
    
		_cond.wait(_lock);
	}
	
	_writer_running = false;
	unlock_maybe_reap();
}

void Socket::unlock_maybe_reap()
{
	if (!_reader_running && !_writer_running)
	{
		shutdown_socket();
		_lock.unlock();
		if (_delay_thread && _delay_thread->is_flushing())
		{
			_delay_thread->wait_for_flush();
		}
		_msgr->queue_reap(this);
	}
	else
	{
		_lock.unlock();
	}
}

static void alloc_aligned_buffer(buffer& data, uint32_t len, uint32_t off)
{
	uint32_t left = len;
	if (off & ~PAGE_MASK)
	{
		uint32_t head = 0;
		head = MIN(PAGE_SIZE - (off & ~PAGE_MASK), left);
		data.push_back(create(head));
		left -= head;
	}
	
	uint32_t middle = left & PAGE_MASK;
	if (0 < middle)
	{
		data.push_back(create_page_aligned(middle));
		left -= middle;
	}
	
	if (left)
	{
		data.push_back(create(left));
	}
}

int Socket::read_message(Message** pm)
{
	int ret = -1;
  
	msg_header header; 
	msg_footer footer;
	uint32_t header_crc = 0;

	if (tcp_read((char*)&header, sizeof(header)) < 0)
	{
		return -1;
	}
	
	if (_msgr->_crc_flag & MSG_CRC_HEADER)
	{
		header_crc = crc32c(0, (unsigned char *)&header, sizeof(header) - sizeof(header.crc));
	}

	if ((_msgr->_crc_flag & MSG_CRC_HEADER) && header_crc != header.crc)
	{
		return -1;
	}

	buffer front, middle, data;
	int front_len, middle_len;
	uint32_t data_len, data_off;
	int aborted;
	Message* message;
	utime_t recv_stamp = clock_now();

	if (_policy._throttler_messages)
	{
		_policy._throttler_messages->get();
	}

	uint64_t message_size = header.front_len + header.middle_len + header.data_len;
	if (message_size)
	{
		if (_policy._throttler_bytes)
		{
			_policy._throttler_bytes->get(message_size);
		}
		
		_msgr->_dispatch_throttler.get(message_size);
	}

	utime_t throttle_stamp = clock_now();

	front_len = header.front_len;
	if (front_len)
	{
		ptr bp = create(front_len);
		if (0 > tcp_read(bp.c_str(), front_len))
		{
			goto out_dethrottle;
		}
		
		front.push_back(std::move(bp));
	}

	middle_len = header.middle_len;
	if (middle_len)
	{
		ptr bp = create(middle_len);
		if (0 > tcp_read(bp.c_str(), middle_len))
		{
			goto out_dethrottle;
		}
		
		middle.push_back(std::move(bp));
	}


	data_len = le32_to_cpu(header.data_len);
	data_off = le32_to_cpu(header.data_off);
	if (data_len)
	{
		uint32_t offset = 0;
		uint32_t left = data_len;

		buffer newbuf, rxbuf;
		buffer::iterator blp;
		int rxbuf_version = 0;
	
		while (0 < left)
		{
			if (0 > tcp_read_wait())
			{
				goto out_dethrottle;
			}

			_connection_state->_lock.lock();
			std::map<uint64_t, std::pair<buffer, int> >::iterator p = _connection_state->_rx_buffers.find(header.tid);
			if (p != _connection_state->_rx_buffers.end())
			{
				if (rxbuf.length() == 0 || p->second.second != rxbuf_version)
				{
					rxbuf = p->second.first;
					rxbuf_version = p->second.second;
					
					if (rxbuf.length() < data_len)
					{
						rxbuf.push_back(create(data_len - rxbuf.length()));
					}
					blp = p->second.first.begin();
					blp.advance(offset);
				}
			}
			else
			{
				if (!newbuf.length())
				{
					alloc_aligned_buffer(newbuf, data_len, data_off);
					blp = newbuf.begin();
					blp.advance(offset);
				}
			}
			
			ptr bp = blp.get_current_ptr();
			int read = MIN(bp.length(), left);
			ssize_t got = tcp_read_nonblocking(bp.c_str(), read);
			_connection_state->_lock.unlock();
			
			if (0 > got)
			{
				goto out_dethrottle;
			}
			
			if (0 < got)
			{
				blp.advance(got);
				data.append(bp, 0, got);
				offset += got;
				left -= got;
			}
		}
	}

	if (0 > tcp_read((char*)&footer, sizeof(footer)))
	{
		goto out_dethrottle;
	}
  
	aborted = (footer.flags & MSG_FOOTER_COMPLETE) == 0;

	if (aborted)
	{
		ret = 0;
		goto out_dethrottle;
	}

	message = decode_message(_msgr->_crc_flag, header, footer, front, middle, data);
	if (!message)
	{
		ret = -EINVAL;
		goto out_dethrottle;
	}

	message->set_byte_throttler(_policy._throttler_bytes);
	message->set_message_throttler(_policy._throttler_messages);

	message->set_dispatch_throttle_size(message_size);

	message->set_recv_stamp(recv_stamp);
	message->set_throttle_stamp(throttle_stamp);
	message->set_recv_complete_stamp(clock_now());

	*pm = message;
	return 0;

out_dethrottle:
	
	if (_policy._throttler_messages)
	{
		_policy._throttler_messages->put();
	}
	
	if (message_size)
	{
		if (_policy._throttler_bytes)
		{
			_policy._throttler_bytes->put(message_size);
		}

		_in_q->dispatch_throttle_release(message_size);
	}
	
	return ret;
}

void Socket::suppress_signal()
{
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	sigset_t pending;
	sigemptyset(&pending);
	sigpending(&pending);
	sigpipe_pending = sigismember(&pending, SIGPIPE);
	if (!sigpipe_pending)
	{
		sigset_t blocked;
		sigemptyset(&blocked);
		pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &blocked);

		sigpipe_unblock = ! sigismember(&blocked, SIGPIPE);
	}
#endif
}

void Socket::restore_signal()
{
#if !defined(MSG_NOSIGNAL) && !defined(SO_NOSIGPIPE)
	if (!sigpipe_pending)
	{
		sigset_t pending;
		sigemptyset(&pending);
		sigpending(&pending);
		if (sigismember(&pending, SIGPIPE))
		{
			static const struct timespec nowait = { 0, 0 };
			TEMP_FAILURE_RETRY(sigtimedwait(&sigpipe_mask, NULL, &nowait));
		}

		if (sigpipe_unblock)
		{
			pthread_sigmask(SIG_UNBLOCK, &sigpipe_mask, NULL);
		}
	}
#endif
}

int Socket::do_sendmsg(struct msghdr* msg, uint32_t len, bool more)
{
	suppress_signal();
	while (0 < len)
	{
		int r;
#if defined(MSG_NOSIGNAL)
		r = ::sendmsg(_fd, msg, MSG_NOSIGNAL | (more ? MSG_MORE : 0));
#else
		r = ::sendmsg(_fd, msg, (more ? MSG_MORE : 0));
#endif
		if (0 == r)
		{
		}
		
		if (0 > r)
		{
			r = -errno; 
			restore_signal();
			return r;
		}
		
		if (_state == SOCKET_CLOSED)
		{
			restore_signal();
			return -EINTR; // close enough
		}

		len -= r;
		if (0 == len)
		{
			break;
		}
    
		while (0 < r)
		{
			if (msg->msg_iov[0].iov_len <= (size_t)r)
			{
				r -= msg->msg_iov[0].iov_len;
				msg->msg_iov++;
				msg->msg_iovlen--;
			}
			else
			{
				msg->msg_iov[0].iov_base = (char *)msg->msg_iov[0].iov_base + r;
				msg->msg_iov[0].iov_len -= r;
				break;
			}
		}
	}
	
	restore_signal();
	return 0;
}

int Socket::write_ack(uint64_t seq)
{
	char c = MSGR_TAG_ACK;
	le64 s;
	s = seq;

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	struct iovec msgvec[2];
	msgvec[0].iov_base = &c;
	msgvec[0].iov_len = 1;
	msgvec[1].iov_base = &s;
	msgvec[1].iov_len = sizeof(s);
	msg.msg_iov = msgvec;
	msg.msg_iovlen = 2;
  
	if (0 > do_sendmsg(&msg, 1 + sizeof(s), true))
	{
		return -1;
	}
	
	return 0;
}

int Socket::write_keepalive()
{
	char c = MSGR_TAG_KEEPALIVE;

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	struct iovec msgvec[2];
	msgvec[0].iov_base = &c;
	msgvec[0].iov_len = 1;
	msg.msg_iov = msgvec;
	msg.msg_iovlen = 1;
  
	if (0 > do_sendmsg(&msg, 1))
	{
		return -1;
	}
	
	return 0;
}

int Socket::write_message(const msg_header& header, const msg_footer& footer, buffer& buf)
{
	int ret = 0;
	
	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = _msgvec;
	int msglen = 0;
  
	char tag = MSGR_TAG_MSG;
	_msgvec[msg.msg_iovlen].iov_base = &tag;
	_msgvec[msg.msg_iovlen].iov_len = 1;
	msglen++;
	msg.msg_iovlen++;

	_msgvec[msg.msg_iovlen].iov_base = (char*)&header;
	_msgvec[msg.msg_iovlen].iov_len = sizeof(header);
	msglen += sizeof(header);
	msg.msg_iovlen++;

	std::list<ptr>::const_iterator pb = buf.buffers().begin();
	uint32_t b_off = 0;
	uint32_t bl_pos = 0;
	uint32_t left = buf.length();

	while (0 < left)
	{
		uint32_t donow = MIN(left, pb->length() - b_off);
		if (0 == donow)
		{
		
		}
    
		if (msg.msg_iovlen >= SM_IOV_MAX - 2)
		{
			if (do_sendmsg(&msg, msglen, true))
			{
				goto fail;
			}
      
			msg.msg_iov = _msgvec;
			msg.msg_iovlen = 0;
			msglen = 0;
		}
    
		_msgvec[msg.msg_iovlen].iov_base = (void*)(pb->c_str()+b_off);
		_msgvec[msg.msg_iovlen].iov_len = donow;
		msglen += donow;
		msg.msg_iovlen++;
    
		left -= donow;
		b_off += donow;
		bl_pos += donow;
		
		if (0 == left)
		{
			break;
		}
		
		while (b_off == pb->length())
		{
			++pb;
			b_off = 0;
		}
	}

	_msgvec[msg.msg_iovlen].iov_base = (void*)&footer;
	_msgvec[msg.msg_iovlen].iov_len = sizeof(footer);
	msglen += sizeof(footer);
	msg.msg_iovlen++;

	if (do_sendmsg(&msg, msglen))
	{
		goto fail;
	}

	ret = 0;

out:
	return ret;

fail:
	ret = -1;
	goto out;
}

int Socket::tcp_read(char* buf, uint32_t len)
{
	if (0 > _fd)
	{
		return -EINVAL;
	}

	while (0 < len)
	{
		if (0 > tcp_read_wait())
		{
			return -1;
		}

		ssize_t got = tcp_read_nonblocking(buf, len);

		if (0 > got)
		{
			return -1;
		}

		len -= got;
		buf += got;
	}
	
	return 0;
}

int Socket::tcp_read_wait()
{
	if (0 > _fd)
	{
		return -EINVAL;
	}
	
	struct pollfd pfd;
	short evmask;
	pfd.fd = _fd;
	pfd.events = POLLIN;
#if defined(__linux__)
	pfd.events |= POLLRDHUP;
#endif

	if (has_pending_data())
	{
		return 0;
	}

	int r = poll(&pfd, 1, _msgr->_timeout);
	if (0 > r)
	{
		return -errno;
	}
	
	if (0 == r)
	{
		return -EAGAIN;
	}

	evmask = POLLERR | POLLHUP | POLLNVAL;
#if defined(__linux__)
	evmask |= POLLRDHUP;
#endif
	if (pfd.revents & evmask)
		return -1;

	if (!(pfd.revents & POLLIN))
		return -1;

	return 0;
}

ssize_t Socket::do_recv(char* buf, size_t len, int flags)
{
again:
	ssize_t got = ::recv(_fd, buf, len, flags);
	if (0 > got)
	{
		if (errno == EINTR)
		{
			goto again;
		}

		return -1;
	}
	
	if (0 == got)
	{
		return -1;
	}
	
	return got;
}

ssize_t Socket::buffered_recv(char* buf, size_t len, int flags)
{
	size_t left = len;
	ssize_t total_recv = 0;
	if (_recv_len > _recv_ofs)
	{
		int to_read = MIN(_recv_len - _recv_ofs, left);
		memcpy(buf, &_recv_buf[_recv_ofs], to_read);
		_recv_ofs += to_read;
		left -= to_read;
		if (0 == left)
		{
			return to_read;
		}
		buf += to_read;
		total_recv += to_read;
	}

	if (left > _recv_max_prefetch)
	{
		ssize_t ret = do_recv(buf, left, flags );
		if (0 > ret)
		{
			if (0 < total_recv)
			{
				return total_recv;
			}
			return ret;
		}
		
		total_recv += ret;
		return total_recv;
	}


	ssize_t got = do_recv(_recv_buf, _recv_max_prefetch, flags);
	if (0 > got)
	{
		if (0 < total_recv)
		{
			return total_recv;
		}

		return got;
	}

	_recv_len = (size_t)got;
	got = MIN(left, (size_t)got);
	memcpy(buf, _recv_buf, got);
	_recv_ofs = got;
	total_recv += got;
	return total_recv;
}

ssize_t Socket::tcp_read_nonblocking(char* buf, uint32_t len)
{
	ssize_t got = buffered_recv(buf, len, MSG_DONTWAIT);
	if (0 > got)
	{
		return -1;
	}
	
	if (0 == got)
	{
		return -1;
	}
	
	return got;
}

int Socket::tcp_write(const char* buf, uint32_t len)
{
	if (0 > _fd)
	{
		return -1;
	}
	
	struct pollfd pfd;
	pfd.fd = _fd;
	pfd.events = POLLOUT | POLLHUP | POLLNVAL | POLLERR;
#if defined(__linux__)
	pfd.events |= POLLRDHUP;
#endif

	if (0 > poll(&pfd, 1, -1))
	{
		return -1;
	}

	if (!(pfd.revents & POLLOUT))
	{
		return -1;
	}
	
	suppress_signal();

	while (0 < len)
	{
		int did;
#if defined(MSG_NOSIGNAL)
		did = ::send(_fd, buf, len, MSG_NOSIGNAL);
#else
		did = ::send(_fd, buf, len, 0);
#endif
	    if (0 > did)
		{
			return did;
	    }
		
		len -= did;
		buf += did;
	}
	
	restore_signal();

	return 0;
}

void Socket::DelayedDelivery::discard()
{
	Mutex::Locker locker(_delay_lock);
	while (!_delay_queue.empty())
	{
		Message* m = _delay_queue.front().second;
		_socket->_in_q->dispatch_throttle_release(m->get_dispatch_throttle_size());
		m->dec();
		_delay_queue.pop_front();
	}
}

void Socket::DelayedDelivery::flush()
{
	Mutex::Locker locker(_delay_lock);
	_flush_count = _delay_queue.size();
	_delay_cond.signal();
}

void Socket::DelayedDelivery::entry()
{
	Mutex::Locker locker(_delay_lock);

	while (!_stop_delayed_delivery)
	{
		if (_delay_queue.empty())
		{
			_delay_cond.wait(_delay_lock);
			continue;
		}
		
		utime_t release = _delay_queue.front().first;
		Message* m = _delay_queue.front().second;
		// config
		std::string delay_msg_type = "";
		if (!_flush_count && (release > clock_now() && (delay_msg_type.empty() || m->get_type_name() == delay_msg_type)))
		{
			_delay_cond.timed_wait(_delay_lock, release.to_msec());
			continue;
		}
		
		_delay_queue.pop_front();
		
		if (0 < _flush_count)
		{
			--_flush_count;
			_active_flush = true;
		}
		
		if (_socket->_in_q->can_fast_dispatch(m))
		{
			if (!_stop_fast_dispatching_flag)
			{
				_delay_dispatching = true;
				_delay_lock.unlock();
				_socket->_in_q->fast_dispatch(m);
				_delay_lock.lock();
				_delay_dispatching = false;
				if (_stop_fast_dispatching_flag)
				{
					_delay_cond.signal();
					_delay_lock.unlock();
					_delay_lock.lock();
				}
			}
		}
		else
		{
			_socket->_in_q->enqueue(m, m->get_priority(), _socket->_conn_id);
		}
		
		_active_flush = false;
	}
}

void Socket::DelayedDelivery::stop_fast_dispatching()
{
	Mutex::Locker locker(_delay_lock);
	_stop_fast_dispatching_flag = true;
	while (_delay_dispatching)
		_delay_cond.wait(_delay_lock);
}

// SYS_NS_END
