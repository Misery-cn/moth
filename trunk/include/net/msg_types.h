#ifndef _MSG_TYPES_H_
#define _MSG_TYPES_H_

#include <arpa/inet.h>
#include "int_types.h"
#include "encoding.h"
#include "msgr.h"

extern const char * entity_type_name(int type);

// 通信实体名
struct entity_name_t
{
public:
	uint8_t _type;
	int64_t _num;

	static const int TYPE_CLIENT = ENTITY_TYPE_CLIENT;
	static const int TYPE_SERVER = ENTITY_TYPE_SERVER;
	static const int TYPE_MASTER = ENTITY_TYPE_MASTER;
	static const int TYPE_SLAVE = ENTITY_TYPE_SLAVE;

	entity_name_t() : _type(0), _num(0) {}
	entity_name_t(uint8_t t, int64_t n) : _type(t), _num(n) {}
	explicit entity_name_t(const entity_name& e) : _type(e.type), _num(e.num) {}
	
	static entity_name_t SVR(int64_t i) { return entity_name_t(TYPE_SERVER, i); }
	static entity_name_t CLI(int64_t i) { return entity_name_t(TYPE_CLIENT, i); }
	static entity_name_t MASTER(int64_t i) { return entity_name_t(TYPE_MASTER, i); }
	static entity_name_t SLAVE(int64_t i) { return entity_name_t(TYPE_SLAVE, i); }

	int64_t num() const { return _num; }

	int type() const { return _type; }

	const char* type_str() const
	{
		return entity_type_name(type());
	}

	bool is_new() const { return num() < 0; }

	operator entity_name() const
	{
		entity_name n = { _type, init_le64(_num) };
		return n;
	}

	void encode(buffer& buf) const
	{
		::encode(_type, buf);
		::encode(_num, buf);
	}
	
	void decode(buffer::iterator& it)
	{
		::decode(_type, it);
		::decode(_num, it);
	}
	
};
WRITE_CLASS_ENCODER(entity_name_t);

inline bool operator== (const entity_name_t& l, const entity_name_t& r)
{ 
	return (l.type() == r.type()) && (l.num() == r.num());
}

inline bool operator!= (const entity_name_t& l, const entity_name_t& r)
{ 
	return (l.type() != r.type()) || (l.num() != r.num());
}

inline bool operator< (const entity_name_t& l, const entity_name_t& r)
{ 
	return (l.type() < r.type()) || (l.type() == r.type() && l.num() < r.num());
}


#if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
static inline void encode(const sockaddr_storage& a, buffer& buf)
{
	struct sockaddr_storage ss = a;
#if defined(DARWIN) || defined(__FreeBSD__)
	unsigned short *ss_family = reinterpret_cast<unsigned short*>(&ss);
	*ss_family = htons(a.ss_family);
#else
	ss.ss_family = htons(ss.ss_family);
#endif
	encode_raw(ss, buf);
}

static inline void decode(sockaddr_storage& a, buffer::iterator& it)
{
	decode_raw(a, it);
#if defined(DARWIN) || defined(__FreeBSD__)
	unsigned short *ss_family = reinterpret_cast<unsigned short *>(&a);
	a.ss_family = ntohs(*ss_family);
	a.ss_len = 0;
#else
	a.ss_family = ntohs(a.ss_family);
#endif
}
#endif

struct socket_addr_storage
{
	le16 ss_family;
	uint8_t __ss_padding[128 - sizeof(le16)];

	void encode(buffer& buf) const
	{
		struct socket_addr_storage ss = *this;
		ss.ss_family = htons(ss.ss_family);
		encode_raw(ss, buf);
	}

	void decode(buffer::iterator& it)
	{
		struct socket_addr_storage ss;
		decode_raw(ss, it);
		ss.ss_family = ntohs(ss.ss_family);
		*this = ss;
	}
} __attr_packed__;

WRITE_CLASS_ENCODER(socket_addr_storage);

// 通信实体地址
struct entity_addr_t
{
public:
	int32_t _type;
	// 唯一标识
	int32_t _nonce;

	union
	{
		// 通用结构体,16字节
		// sockaddr addr;
		
		// 通用结构体,128字节
	    sockaddr_storage _addr;
		// size 16
	    sockaddr_in _addr4;
		// size 28
	    sockaddr_in6 _addr6;
	};

	entity_addr_t() : _type(0), _nonce(0)
	{ 
		memset(&_addr, 0, sizeof(_addr));
	}

	entity_addr_t(const char* str)
	{ 
		memset(&_addr, 0, sizeof(_addr));
	}
	
	explicit entity_addr_t(const entity_addr& a)
	{
		_type = a.type;
		_nonce = a.nonce;
		_addr = a.in_addr;
#if !defined(__FreeBSD__)
		_addr.ss_family = ntohs(_addr.ss_family);
#endif
	}

	
	operator entity_addr() const
	{
		entity_addr a;
		a.type = 0;
		a.nonce = _nonce;
		a.in_addr = _addr;
#if !defined(__FreeBSD__)
		a.in_addr.ss_family = htons(_addr.ss_family);
#endif
		return a;
	}

	uint32_t addr_size()
	{
		switch (_addr.ss_family)
		{
		    case AF_INET:
				return sizeof(_addr4);
		    case AF_INET6:
				return sizeof(_addr6);
	    }
		
	    return sizeof(_addr);
	}

	uint32_t get_nonce() const { return _nonce; }
	void set_nonce(uint32_t n) { _nonce = n; }

	int get_family() const { return _addr.ss_family; }
	void set_family(int f) { _addr.ss_family = f; }

	sockaddr_storage& sock_addr() { return _addr; }
	sockaddr_in& sock_addr4() { return _addr4; }
	sockaddr_in6& sock_addr6() { return _addr6; }

	const sockaddr* get_sockaddr() const
	{
		return (const sockaddr*)&_addr4;
	}

	size_t get_sockaddr_len() const
	{
		switch (_addr.ss_family)
		{
			case AF_INET:
			{
				return sizeof(_addr4);
			}
			case AF_INET6:
			{
				return sizeof(_addr6);
			}
		}
		
		return sizeof(_addr);
	}

	bool set_sockaddr(struct sockaddr* sa)
	{
		switch (sa->sa_family)
		{
			case AF_INET:
			{
				memcpy(&_addr4, sa, sizeof(sockaddr_in));
				break;
			}
			case AF_INET6:
			{
				memcpy(&_addr6, sa, sizeof(sockaddr_in6));
				break;
			}
			default:
			{
				return false;
			}
		}
		
		return true;
	}

	sockaddr_storage get_sockaddr_storage() const
	{
		return _addr;
	}

	void set_port(int port)
	{
		switch (_addr.ss_family)
		{
			case AF_INET:
			{
				_addr4.sin_port = htons(port);
				break;
			}
			case AF_INET6:
			{
				_addr6.sin6_port = htons(port);
				break;
			}
			default:
			{
				return;
			}
		}
	}

	int get_port() const 
	{
		switch (_addr.ss_family)
		{
			case AF_INET:
			{
				return ntohs(_addr4.sin_port);
			}
			case AF_INET6:
			{
				return ntohs(_addr6.sin6_port);
			}
		}
		
		return 0;
	}

	bool probably_equals(const entity_addr_t& other) const 
	{
		if (get_port() != other.get_port())
		{
			return false;
		}
		
		if (get_nonce() != other.get_nonce())
		{
			return false;
		}
		
		if (is_blank_ip() || other.is_blank_ip())
		{
			return true;
		}
		
		if (0 == memcmp(&_addr, &other._addr, sizeof(_addr)))
		{
			return true;
		}
		
		return false;
	}

	bool is_same_host(const entity_addr_t& other) const 
	{
		if (_addr.ss_family != other._addr.ss_family)
		{
			return false;
		}
		
		if (AF_INET == _addr.ss_family)
		{
			return _addr4.sin_addr.s_addr == other._addr4.sin_addr.s_addr;
		}
		
		if (AF_INET6 == _addr.ss_family)
		{
			return (0 == memcmp(_addr6.sin6_addr.s6_addr, other._addr6.sin6_addr.s6_addr, sizeof(_addr6.sin6_addr.s6_addr)));
		}
		
		return false;
	}

	bool is_blank_ip() const
	{
		
		switch (_addr.ss_family)
		{
			case AF_INET:
			{
				return (INADDR_ANY == _addr4.sin_addr.s_addr);
			}
			case AF_INET6:
			{
				return (0 == memcmp(&_addr6.sin6_addr, &in6addr_any, sizeof(in6addr_any)));
			}
			default:
			{
				return true;
			}
		}
	}

	void encode(buffer& buf) const
	{
		::encode(_type, buf);
		::encode(_nonce, buf);
#if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
		::encode(_addr, buf);
#else
		socket_addr_storage wireaddr;
		memset(&wireaddr, '\0', sizeof(wireaddr));
		uint32_t copysize = MIN(sizeof(wireaddr), sizeof(_addr));
		memcpy(&wireaddr, &_addr, copysize);
		::encode(wireaddr, buf);
#endif
	}
	
	void decode(buffer::iterator& it)
	{
		::decode(_type, it);
		::decode(_nonce, it);
#if defined(__linux__) || defined(DARWIN) || defined(__FreeBSD__)
		::decode(_addr, it);
#else
		socket_addr_storage wireaddr;
		memset(&wireaddr, '\0', sizeof(wireaddr));
		::decode(wireaddr, it);
		unsigned copysize = MIN(sizeof(wireaddr), sizeof(_addr));
		memcpy(&_addr, &wireaddr, copysize);
#endif
	}
	
};
WRITE_CLASS_ENCODER(entity_addr_t);

inline bool operator== (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) == 0; }
inline bool operator!= (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) != 0; }
inline bool operator< (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) < 0; }
inline bool operator<= (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) <= 0; }
inline bool operator> (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) > 0; }
inline bool operator>= (const entity_addr_t& a, const entity_addr_t& b) { return memcmp(&a, &b, sizeof(a)) >= 0; }

// 通信实体
struct entity_inst_t
{
public:
	
	entity_name_t _name;
	entity_addr_t _addr;
	entity_inst_t() {}
	entity_inst_t(entity_name_t n, const entity_addr_t& a) : _name(n), _addr(a) {}

	entity_inst_t(const entity_inst& i) : _name(i.name), _addr(i.addr) {}
	entity_inst_t(const entity_name& n, const entity_addr& a) : _name(n), _addr(a) {}
	operator entity_inst()
	{
		entity_inst i = {_name, _addr};
		return i;
	}

	void encode(buffer& buf) const
	{
		::encode(_name, buf);
		::encode(_addr, buf);
	}
	
	void decode(buffer::iterator& it)
	{
		::decode(_name, it);
		::decode(_addr, it);
	}
};
WRITE_CLASS_ENCODER(entity_inst_t);

inline bool operator==(const entity_inst_t& a, const entity_inst_t& b)
{ 
	return a._name == b._name && a._addr == b._addr;
}

inline bool operator!=(const entity_inst_t& a, const entity_inst_t& b)
{ 
	return a._name != b._name || a._addr != b._addr;
}

inline bool operator<(const entity_inst_t& a, const entity_inst_t& b)
{ 
	return a._name < b._name || (a._name == b._name && a._addr < b._addr);
}

inline bool operator<=(const entity_inst_t& a, const entity_inst_t& b)
{
	return a._name < b._name || (a._name == b._name && a._addr <= b._addr);
}

inline bool operator>(const entity_inst_t& a, const entity_inst_t& b) { return b < a; }

inline bool operator>=(const entity_inst_t& a, const entity_inst_t& b) { return b <= a; }

#endif
