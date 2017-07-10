#ifndef _MASTERMAP_H_
#define _MASTERMAP_H_

#include <map>
#include "msg_types.h"
#include "log.h"

#define PREFIX_MASTER "master."
#define PREFIX_MASTE_LEN 7

class MasterMap
{
public:
	MasterMap();
	virtual ~MasterMap();

	// 构造mastermap
	int build_initial();

	void add(const std::string& name, const entity_addr_t& addr)
	{
		_name_addr[name] = addr;

		std::string strrank = name.substr(PREFIX_MASTE_LEN, name.length());


		// 检查是否是数字
		if (StringUtils::is_numeric_string(strrank.c_str()))
		{
			uint32_t rank = 0;
			// 是否转换成功
			if (StringUtils::string2int(strrank.c_str(), rank))
			{
				_rank_addr[rank] = addr;
			}
			else
			{
				// ERROR
			}
		}
		else
		{
			// ERROR
		}
	}

	unsigned size()
	{
		return _name_addr.size();
	}

	bool contains(const std::string& name)
	{
		return _name_addr.count(name);
	}

	bool contains(const uint32_t i)
	{
		return _rank_addr.count(i);
	}

	bool contains(const entity_addr_t& addr)
	{
		std::map<std::string, entity_addr_t>::iterator it = _name_addr.begin();
		for (; it != _name_addr.end(); it++)
		{
			if (addr == it->second)
			{
				return true;
			}
		}
		
		return false;
	}

	const entity_addr_t& get_addr_by_rank(const uint32_t i)
	{
		return _rank_addr[i];
	}

private:
	// 版本号
	uint64_t _epoch;

	// master信息
	std::map<std::string, entity_addr_t> _name_addr;

	std::map<uint32_t, entity_addr_t> _rank_addr;
};

#endif