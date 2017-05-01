#include <stdlib.h>
#include <strings.h>
#include "env.h"

bool get_env_bool(const char* key)
{
	const char* val = getenv(key);
	if (!val)
		return false;
	if (0 == strcasecmp(val, "off"))
		return false;
	if (0 == strcasecmp(val, "no"))
		return false;
	if (0 == strcasecmp(val, "false"))
		return false;
	if (0 == strcasecmp(val, "0"))
		return false;
	
	return true;
}

int get_env_int(const char* key)
{
	const char* val = getenv(key);
	if (!val)
		return 0;
	
	return atoi(val);
}
