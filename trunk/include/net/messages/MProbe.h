#ifndef _MPROBE_H_
#define _MPROBE_H_

#include "message.h"

class MProbe : public Message
{
public:
	enum
	{
		OP_PROBE = 1,
		OP_REPLY
	};

	MProbe() : Message(MSG_PROBE)
	{
	}
	
	MProbe(int op, uint32_t rank, bool join) : Message(MSG_PROBE), _op(op), _rank(rank), _paxos_first_version(0),
			_paxos_last_version(0), _has_ever_joined(join)
	{
	}

	~MProbe()
	{
	}

	void encode_payload()
	{
		if (_mastermap.length())
		{
			MasterMap t;
			t.decode(_mastermap);
			_mastermap.clear();
			t.encode(_mastermap);
		}

	    ::encode(_op, _payload);
	    ::encode(_rank, _payload);
	    ::encode(_quorum, _payload);
	    ::encode(_mastermap, _payload);
	    ::encode(_has_ever_joined, _payload);
	    ::encode(_paxos_first_version, _payload);
	    ::encode(_paxos_last_version, _payload);
	}
	
	void decode_payload()
	{
		buffer::iterator p = _payload.begin();
		::decode(_op, p);
		::decode(_rank, p);
		::decode(_quorum, p);
		::decode(_mastermap, p);
		::decode(_has_ever_joined, p);
		::decode(_paxos_first_version, p);
		::decode(_paxos_last_version, p);
	}

	const char* get_type_name() const { return "mprobe"; }

	int32_t _op;
	uint32_t _rank;
	std::set<int32_t> _quorum;
	buffer _mastermap;
	uint64_t _paxos_first_version;
	uint64_t _paxos_last_version;
	bool _has_ever_joined;
};

#endif
