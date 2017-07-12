#include "mastermap.h"
#include "config.h"

MasterMap::MasterMap()
{
}

MasterMap::~MasterMap()
{
}

int MasterMap::build_initial()
{
	std::vector<std::string> secs = sconfig.get_all_sections();

	for (int i = 0; i < secs.size(); i++)
	{
		DEBUG_LOG("section name is %s", secs[i].c_str());
	}

	std::vector<std::string> master_name;

	typedef std::vector<std::string>::iterator vec_iter;

	vec_iter it = secs.begin();

	for (; it != secs.end(); it++)
	{
		if ((it->substr(0, PREFIX_MASTE_LEN) == PREFIX_MASTER) && (it->size() > PREFIX_MASTE_LEN))
		{
			master_name.push_back(*it);
		}
	}

	it = master_name.begin();

	for (; it != master_name.end(); it++)
	{
		std::string straddr;
		sconfig.get_val((*it).c_str(), "addr", straddr);
		DEBUG_LOG("%s addr is %s", (*it).c_str(), straddr.c_str());
		entity_addr_t addr(straddr.c_str());

		if (0 == addr.get_port())
		{
			addr.set_port(9999);
		}

		add(*it, addr);
	}

	return 0;
}

void MasterMap::encode(buffer& buf) const
{
	::encode(_epoch, buf);
	// ::encode(_name_addr, buf);
}

void MasterMap::decode(buffer::iterator& it)
{
	::decode(_epoch, it);
}


