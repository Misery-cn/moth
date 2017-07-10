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

	int32_t _op;
	std::string _name;
	std::set<int32_t> _quorum;
	buffer _mastermap;
	uint64_t _paxos_first_version;
	uint64_t _paxos_last_version;
	bool _has_ever_joined;
};

#endif
