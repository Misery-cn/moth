#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "crc32.h"
#include "callback.h"
#include "msg_types.h"
#include "connection.h"
#include "throttle.h"



#define MSG_CRC_DATA			(1 << 0)
#define MSG_CRC_HEADER		(1 << 1)
#define MSG_CRC_ALL			(MSG_CRC_DATA | MSG_CRC_HEADER)


class Message : public RefCountable
{
public:
	Message()
	{
		memset(&_header, 0, sizeof(_header));
		memset(&_footer, 0, sizeof(_footer));
	}

	Message(int t, int version = 1, int compat_version = 0)
	{
		memset(&_header, 0, sizeof(_header));
		_header.type = t;
		_header.version = version;
		_header.compat_version = compat_version;
		_header.priority = 0;
		_header.data_off = 0;
		memset(&_footer, 0, sizeof(_footer));
	}

	virtual ~Message()
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_payload.length() + _middle.length() + _data.length());
		}
		
		release_message_throttle();
		
		if (_completion_hook)
		{
			_completion_hook->complete(0);
		}
	}

	Message* get()
	{
		return static_cast<Message*>(RefCountable::get());
	}
	
	// 由具体的消息自己实现编解码
	// virtual int encode() = 0;
	// virtual int decode() = 0;

	class CompletionHook : public Callback
	{
	protected:
		Message* _msg;
		friend class Message;
	public:
		explicit CompletionHook(Message* m) : _msg(m) {}
		virtual void set_message(Message* m) { _msg = m; }
	};


public:
	const Connection* get_connection() const { return _connection; }
	void set_connection(Connection* c)
	{
		_connection = c;
	}
	
	CompletionHook* get_completion_hook() { return _completion_hook; }
	void set_completion_hook(CompletionHook* hook) { _completion_hook = hook; }
	
	void set_byte_throttler(Throttle* t) { _byte_throttler = t; }
	Throttle* get_byte_throttler() { return _byte_throttler; }
	
	void set_message_throttler(Throttle* t) { _msg_throttler = t; }
	Throttle *get_message_throttler() { return _msg_throttler; }
	
	void set_dispatch_throttle_size(uint64_t s) { _dispatch_throttle_size = s; }
	uint64_t get_dispatch_throttle_size() const { return _dispatch_throttle_size; }
	
	const msg_header& get_header() const { return _header; }
	msg_header& get_header() { return _header; }
	
	void set_header(const msg_header& h) { _header = h; }
	void set_footer(const msg_footer& f) { _footer = f; }
	
	const msg_footer& get_footer() const { return _footer; }
	msg_footer& get_footer() { return _footer; }
	
	void set_src(const entity_name_t& src) { _header.src = src; }
	
	uint32_t get_magic() const { return _magic; }
	void set_magic(int magic) { _magic = magic; }
	
	void clear_payload()
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_payload.length() + _middle.length());
		}
		
		_payload.clear();
		_middle.clear();
	}
	
	virtual void clear_buffers() {}
	
	void clear_data()
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_data.length());
		}
		
		_data.clear();
		clear_buffers();
	}
	
	void release_message_throttle()
	{
		if (_msg_throttler)
		{
			_msg_throttler->put();
		}
		
		_msg_throttler = NULL;
	}
	
	bool empty_payload() const { return _payload.length() == 0; }
	
	buffer& get_payload() { return _payload; }
	
	void set_payload(buffer& buf)
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_payload.length());
		}
		
		_payload.claim(buf, buffer::CLAIM_ALLOW_NONSHAREABLE);
		
		if (_byte_throttler)
		{
			_byte_throttler->take(_payload.length());
		}
	}
	
	void set_middle(buffer& buf)
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_middle.length());
		}
		
		_middle.claim(buf, buffer::CLAIM_ALLOW_NONSHAREABLE);
		
		if (_byte_throttler)
		{
			_byte_throttler->take(_middle.length());
		}
	}
	
	buffer& get_middle() { return _middle; }
	
	void set_data(const buffer& buf)
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_data.length());
		}
		
		_data.share(buf);
		
		if (_byte_throttler)
		{
			_byte_throttler->take(_data.length());
		}
	}
	
	const buffer& get_data() const { return _data; }
	
	buffer& get_data() { return _data; }
	
	void claim_data(buffer& buf, uint32_t flags = buffer::CLAIM_DEFAULT)
	{
		if (_byte_throttler)
		{
			_byte_throttler->put(_data.length());
		}
		
		buf.claim(_data, flags);
	}
	
	off_t get_data_len() const { return _data.length(); }
	
	void set_recv_stamp(utime_t t) { _recv_stamp = t; }
	const utime_t& get_recv_stamp() const { return _recv_stamp; }
	
	void set_dispatch_stamp(utime_t t) { _dispatch_stamp = t; }
	const utime_t& get_dispatch_stamp() const { return _dispatch_stamp; }
	
	void set_throttle_stamp(utime_t t) { _throttle_stamp = t; }
	const utime_t& get_throttle_stamp() const { return _throttle_stamp; }
	
	void set_recv_complete_stamp(utime_t t) { _recv_complete_stamp = t; }
	const utime_t& get_recv_complete_stamp() const { return _recv_complete_stamp; }
	
	void calc_header_crc()
	{
		_header.crc = crc32c(0, (unsigned char*)&_header, sizeof(_header) - sizeof(_header.crc));
	}
	
	void calc_front_crc()
	{
		_footer.front_crc = _payload.crc32(0);
		_footer.middle_crc = _middle.crc32(0);
	}
	
	void calc_data_crc()
	{
		_footer.data_crc = _data.crc32(0);
	}
	
	virtual int get_cost() const
	{
		return _data.length();
	}
	
	int get_type() const { return _header.type; }
	void set_type(int t) { _header.type = t; }
	
	uint64_t get_tid() const { return _header.tid; }
	void set_tid(uint64_t t) { _header.tid = t; }
	
	uint64_t get_seq() const { return _header.seq; }
	void set_seq(uint64_t s) { _header.seq = s; }
	
	unsigned get_priority() const { return _header.priority; }
	void set_priority(int16_t p) { _header.priority = p; }
	
	entity_inst_t get_source_inst() const
	{
		return entity_inst_t(get_source(), get_source_addr());
	}
	
	entity_name_t get_source() const
	{
		return entity_name_t(_header.src);
	}
	
	entity_addr_t get_source_addr() const
	{
		if (_connection)
		{
			return _connection->get_peer_addr();
		}
		
		return entity_addr_t();
	}
	
	entity_inst_t get_orig_source_inst() const
	{
		return get_source_inst();
	}
	
	entity_name_t get_orig_source() const
	{
		return get_source();
	}
	
	entity_addr_t get_orig_source_addr() const
	{
		return get_source_addr();
	}
	
	virtual void decode_payload() = 0;
	virtual void encode_payload(uint64_t features) = 0;
	virtual const char* get_type_name() const = 0;
	
	void encode(uint64_t features, int crcflags);

protected:
	msg_header _header;
	msg_footer  _footer;
	buffer _payload;
	buffer _middle;
	buffer _data;

	utime_t _recv_stamp;
	utime_t _dispatch_stamp;
	utime_t _throttle_stamp;
	utime_t _recv_complete_stamp;

	Connection* _connection;

	uint32_t _magic;

	CompletionHook* _completion_hook;
		
	Throttle* _byte_throttler;
	Throttle* _msg_throttler;
	uint64_t _dispatch_throttle_size;

	friend class Messenger;
};

extern Message* decode_message(int crcflags, msg_header& header, msg_footer& footer, buffer& front, buffer& middle, buffer& data);
extern void encode_message(Message* m, uint64_t features, buffer& buf);
extern Message* decode_message(int crcflags, buffer::iterator& it);


#endif
