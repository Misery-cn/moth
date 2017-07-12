#include "message.h"

void Message::encode(int crcflags)
{
	if (empty_payload())
	{
		encode_payload();

		if (_byte_throttler)
		{
			_byte_throttler->take(_payload.length() + _middle.length());
		}

		if (0 == _header.compat_version)
		{
			_header.compat_version = _header.version;
		}
	}
	
	if (crcflags & MSG_CRC_HEADER)
	{
		calc_front_crc();
	}

	_header.front_len = get_payload().length();
	_header.middle_len = get_middle().length();
	_header.data_len = get_data().length();
	
	if (crcflags & MSG_CRC_HEADER)
	{
		calc_header_crc();
	}

	_footer.flags = MSG_FOOTER_COMPLETE;

	if (crcflags & MSG_CRC_DATA)
	{
		calc_data_crc();
	}
	else
	{
		_footer.flags = (unsigned)_footer.flags | MSG_FOOTER_NOCRC;
	}
}

Message* decode_message(int crcflags, msg_header& header, msg_footer& footer, buffer& front, buffer& middle, buffer& data)
{
	if (crcflags & MSG_CRC_HEADER)
	{
		uint32_t front_crc = front.crc32(0);
		uint32_t middle_crc = middle.crc32(0);

	    if (front_crc != footer.front_crc)
		{
			return 0;
		}
		
	    if (middle_crc != footer.middle_crc)
		{
			return 0;
	    }
	}
	
	if (crcflags & MSG_CRC_DATA)
	{
		if ((footer.flags & MSG_FOOTER_NOCRC) == 0)
		{
			uint32_t data_crc = data.crc32(0);
			if (data_crc != footer.data_crc)
			{
				return 0;
			}
		}
	}

	Message* m = NULL;
	int type = header.type;
	switch (type)
	{
		default:
		{
			return NULL;
		}
	}

	if (m->get_header().version && m->get_header().version < header.compat_version)
	{
		m->dec();
		return NULL;
	}

	m->set_header(header);
	m->set_footer(footer);
	m->set_payload(front);
	m->set_middle(middle);
	m->set_data(data);

	try
	{
		m->decode_payload();
	}
	catch (SysCallException& e)
	{
		m->dec();
		return NULL;
	}

	return m;
}

void encode_message(Message* msg, buffer& payload)
{
	buffer front, middle, data;
	msg_footer footer;
	::encode(msg->get_header(), payload);
	::encode(msg->get_footer(), payload);
	::encode(msg->get_payload(), payload);
	::encode(msg->get_middle(), payload);
	::encode(msg->get_data(), payload);
}

Message* decode_message(int crcflags, buffer::iterator& p)
{
	msg_header h;
	msg_footer f;
	buffer fr, mi, da;
	::decode(h, p);
	::decode(f, p);
	::decode(fr, p);
	::decode(mi, p);
	::decode(da, p);
	
	return decode_message(crcflags, h, f, fr, mi, da);
}


