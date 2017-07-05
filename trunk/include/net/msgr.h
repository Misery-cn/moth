#ifndef _MSGR_H_
#define _MSGR_H_

#ifndef __KERNEL__
#include <sys/socket.h> // sockaddr_storage
#endif
#include "byteorder.h"

#define ENTITY_TYPE_CLIENT	0x01
#define ENTITY_TYPE_SERVER	0x02
#define ENTITY_TYPE_MASTER	0x03
#define ENTITY_TYPE_SLAVE	0x04

#define MSGR_TAG_MSG				1
#define MSGR_TAG_KEEPALIVE		2
#define MSGR_TAG_ACK				3
#define MSGR_TAG_CLOSE			4
#define MSGR_TAG_KEEPALIVE2_ACK	5
#define MSGR_TAG_KEEPALIVE2		6
#define MSGR_TAG_SEQ				7
#define MSGR_TAG_READY			8
#define MSGR_TAG_WAIT			9
#define MSGR_TAG_RETRY_SESSION	10
#define MSGR_TAG_RETRY_GLOBAL	11
#define MSGR_TAG_RESETSESSION	12
#define MSGR_TAG_BADAUTHORIZER	13
#define MSGR_TAG_BADPROTOVER		14
#define MSGR_TAG_FEATURES		15



#define MSG_PRIO_LOW		64
#define MSG_PRIO_DEFAULT	127
#define MSG_PRIO_HIGH	196
#define MSG_PRIO_HIGHEST	255


#define MSG_FOOTER_COMPLETE	(1<<0)
#define MSG_FOOTER_NOCRC		(1<<1)
#define MSG_FOOTER_SIGNED	(1<<2)


#define MSG_CONNECT_LOSSY  1


struct entity_name
{
	uint8_t type;
	le64 num;
} __attr_packed__;

struct entity_addr
{
	le32 type;
	le32 nonce;
	struct sockaddr_storage in_addr;
} __attr_packed__;

struct entity_inst
{
	struct entity_name name;
	struct entity_addr addr;
} __attr_packed__;


struct msg_connect
{
	le64 features;
	le32 host_type;
	le32 global_seq;
	le32 connect_seq;
	le32 protocol_version;
	// le32 authorizer_protocol;
	// le32 authorizer_len;
	uint8_t flags;
} __attr_packed__;


struct msg_connect_reply
{
	uint8_t tag;
	le64 features;
	le32 global_seq;
	le32 connect_seq;
	le32 protocol_version;
	// le32 authorizer_len;
	uint8_t flags;
} __attr_packed__;

struct msg_header
{
	le64 seq;
	le64 tid;
	le16 type;
	le16 priority;
	le16 version;

	le32 front_len;
	le32 middle_len;
	le32 data_len;
	le16 data_off;
	
	struct entity_name src;

	le16 compat_version;
	le16 reserved;
	le32 crc;
} __attr_packed__;
WRITE_RAW_ENCODER(msg_header);


struct msg_footer
{
	le32 front_crc;
	le32 middle_crc;
	le32 data_crc;
	le64 sig;
	uint8_t flags;
} __attr_packed__;
WRITE_RAW_ENCODER(msg_footer);


#endif
