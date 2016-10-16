#ifndef _MSG_TYPES_H_
#define _MSG_TYPES_H_

#include "define.h"
#include "socket.h"

// 通信实体名
struct entity_name_t
{
public:
	int8_t type_;
	int64_t num_;

	enum 
	{
		ENTITY_TYPE_CLIENT = 0,
		ENTITY_TYPE_SERVER
	};

	entity_name_t() : type_(0), num_(0) { }
	entity_name_t(int8_t t, int64_t n) : type_(t), num_(n) { }
};


// 通信实体地址
struct entity_addr_t
{
public:
	int32_t type_;
	// 唯一标识
	int32_t nonce_;

	union 
	{
	    sockaddr_storage addr;
	    sockaddr_in addr4;
	    sockaddr_in6 addr6;
	};

	entity_addr_t() : type_(0), nonce_(0) { 
    	memset(&addr, 0, sizeof(addr));
	}

	uint32_t addr_size()
	{
		switch (addr.ss_family)
		{
		    case AF_INET:
				return sizeof(addr4);
		    case AF_INET6:
				return sizeof(addr6);
	    }
		
	    return sizeof(addr);
	}

	uint32_t get_nonce() const { return nonce_; }
	void set_nonce(uint32_t n) { nonce_ = n; }

	int get_family() const { return addr.ss_family; }
	void set_family(int f) { addr.ss_family = f; }

	sockaddr_storage& sock_addr() { return addr; }
	sockaddr_in& sock_addr4() { return addr4; }
	sockaddr_in6& sock_addr6() { return addr6; }

	bool set_sockaddr(struct sockaddr* sa)
	{
		switch (sa->sa_family)
		{
			case AF_INET:
			{
				memcpy(&addr4, sa, sizeof(sockaddr_in));
				break;
			}
			case AF_INET6:
			{
				memcpy(&addr6, sa, sizeof(sockaddr_in6));
				break;
			}
			default:
			{
				return false;
			}
		}
		
		return true;
	}

	int get_port() const 
	{
		switch (addr.ss_family)
		{
			case AF_INET:
			{
				return ntohs(addr4.sin_port);
			}
			case AF_INET6:
			{
				return ntohs(addr6.sin6_port);
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
		
		if (0 == memcmp(&addr, &other.addr, sizeof(addr)))
		{
			return true;
		}
		
		return false;
	}

	bool is_same_host(const entity_addr_t& other) const 
	{
		if (addr.ss_family != other.addr.ss_family)
		{
			return false;
		}
		
		if (AF_INET == addr.ss_family)
		{
			return addr4.sin_addr.s_addr == other.addr4.sin_addr.s_addr;
		}
		
		if (AF_INET6 == addr.ss_family)
		{
			return (0 == memcmp(addr6.sin6_addr.s6_addr, other.addr6.sin6_addr.s6_addr, sizeof(addr6.sin6_addr.s6_addr)));
		}
		
		return false;
	}

	bool is_blank_ip() const {
		
		switch (addr.ss_family)
		{
			case AF_INET:
				return (INADDR_ANY == addr4.sin_addr.s_addr);
			case AF_INET6:
				return (0 == memcmp(&addr6.sin6_addr, &in6addr_any, sizeof(in6addr_any)));
			default:
				return true;
		}
	}
	
};

// 通信实体
struct entity_inst_t
{
public:
	
	entity_name_t name_;
	entity_addr_t addr_;
	entity_inst_t() {}
	entity_inst_t(entity_name_t n, const entity_addr_t& a) : name_(n), addr_(a) {}
};

#endif