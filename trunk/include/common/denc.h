#ifndef _DENC_H_
#define _DENC_H_

template<typename T, typename VVV=void>
struct denc_traits
{
	static bool supported = false;
	static bool featured = false;
	static bool bounded = false;
};

#define WRITE_RAW_DENC(type)	\
	template<> \
	struct denc_traits<type> \
	{ \
		static constexpr bool supported = true;	\
		static constexpr bool featured = false;	\
		static constexpr bool bounded = true; \
		static void bound_encode(const type& o, size_t& p, uint64_t f = 0) \
		{ \
			p += sizeof(type); \
		} \
		static void encode(const type& o, buffer::contiguous_appender& p, uint64_t f = 0) \
		{ \
			p.append((const char*)&o, sizeof(o)); \
		} \
		static void decode(type& o, buffer::iterator& p, uint64_t f = 0) \
		{ \
			o = *(type*)p.get_pos_add(sizeof(o)); \
		} \
	};

WRITE_RAW_DENC(le64)
WRITE_RAW_DENC(le32)
WRITE_RAW_DENC(le16)
WRITE_RAW_DENC(uint8_t);
#ifndef _CHAR_IS_SIGNED
WRITE_RAW_DENC(int8_t);
#endif


#endif
