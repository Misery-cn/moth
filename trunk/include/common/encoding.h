#ifndef _ENCODING_H_
#define _ENCODING_H_

#include "byteorder.h"
#include "buffer.h"

#define ENCODE_STR(x) #x

template<class T>
inline void encode_raw(const T& t, buffer& buf)
{
	buf.append((char*)&t, sizeof(t));
}

template<class T>
inline void decode_raw(T& t, buffer::iterator& p)
{
	p.copy(sizeof(t), (char*)&t);
}


#define WRITE_RAW_ENCODER(type)	\
	inline void encode(const type& v, buffer& buf, uint64_t features=0) { encode_raw(v, buf); } \
	inline void decode(type &v, buffer::iterator& p) { decode_raw(v, p); }


WRITE_RAW_ENCODER(uint8_t)
#ifndef _CHAR_IS_SIGNED
WRITE_RAW_ENCODER(int8_t)
#endif
WRITE_RAW_ENCODER(char)
WRITE_RAW_ENCODER(le64)		// defined byteorder.h
WRITE_RAW_ENCODER(le32)
WRITE_RAW_ENCODER(le16)
WRITE_RAW_ENCODER(float)
WRITE_RAW_ENCODER(double)

inline void encode(const bool &v, buffer& buf)
{
	uint8_t vv = v;
	encode_raw(vv, buf);
}

inline void decode(bool &v, buffer::iterator& p)
{
	uint8_t vv;
	decode_raw(vv, p);
	v = vv;
}


// -----------------------------------------------------------int types------------------------------------------------

#define WRITE_INTTYPE_ENCODER(type, letype) \
	inline void encode(type v, buffer& buf, uint64_t features=0) \
	{ \
		letype e; \
		e = v; \
		encode_raw(e, buf); \
	}	\
	inline void decode(type& v, buffer::iterator& p) \
	{ \
		letype e; \
		decode_raw(e, p); \
		v = e; \
	}

WRITE_INTTYPE_ENCODER(uint64_t, le64)
WRITE_INTTYPE_ENCODER(int64_t, le64)
WRITE_INTTYPE_ENCODER(uint32_t, le32)
WRITE_INTTYPE_ENCODER(int32_t, le32)
WRITE_INTTYPE_ENCODER(uint16_t, le16)
WRITE_INTTYPE_ENCODER(int16_t, le16)


// �������Լ���encode��decode����
#define WRITE_CLASS_ENCODER(cl) \
	inline void encode(const cl& c, buffer& buf, uint64_t features=0) { c.encode(buf); } \
	inline void decode(cl& c, buffer::iterator &p) { c.decode(p); }

#define WRITE_CLASS_MEMBER_ENCODER(cl) \
	inline void encode(const cl& c, buffer& buf) const { c.encode(buf); } \
	inline void decode(cl &c, buffer::iterator& p) { c.decode(p); }

#define WRITE_CLASS_ENCODER_FEATURES(cl)				\
	inline void encode(const cl& c, buffer& buf, uint64_t features) { c.encode(buf, features); } \
	inline void decode(cl& c, buffer::iterator& p) { c.decode(p); }

#define WRITE_CLASS_ENCODER_OPTIONAL_FEATURES(cl) \
	inline void encode(const cl& c, buffer& buf, uint64_t features = 0) { c.encode(buf, features); }	\
	inline void decode(cl& c, buffer::iterator& p) { c.decode(p); }


/*
template<class T>
inline void decode(T& o, buffer& buf)
{
	buffer::iterator p = buf.begin();
	decode(o, p);
}
*/


// string
inline void encode(const std::string& s, buffer& buf, uint64_t features = 0)
{
	uint32_t len = s.length();
	encode(len, buf);
	if (len)
	{
		buf.append(s.data(), len);
	}
}

inline void decode(std::string& s, buffer::iterator& it)
{
	uint32_t len;
	decode(len, it);
	s.clear();
	it.copy(len, s);
}


// char*
inline void encode(const char* s, buffer& buf) 
{
	uint32_t len = strlen(s);
	encode(len, buf);
	if (len)
	{
		buf.append(s, len);
	}
}

// ptr
inline void encode(const ptr& p, buffer& buf) 
{
	uint32_t len = p.length();
	encode(len, buf);
	if (len)
	{
		buf.append(p);
	}
}

inline void decode(ptr& p, buffer::iterator& bi)
{
	uint32_t len;
	decode(len, bi);

	buffer s;
	bi.copy(len, s);

	if (len)
	{
		if (1 == s.get_num_buffers())
		{
			p = s.front();
		}
		else
		{
			p = copy(s.c_str(), s.length());
		}
	}
}

// buffer
inline void encode(const buffer& s, buffer& buf) 
{
	uint32_t len = s.length();
	encode(len, buf);
	buf.append(s);
	// bl.claim_append(s);
}

inline void decode(buffer& buf, buffer::iterator& bi)
{
	uint32_t len;
	decode(len, bi);
	buf.clear();
	bi.copy(len, buf);
}

#endif