#include "config.h"

char CConfigFile::config_path_[MAX_FILE_NAME] = {0};

const char* CConfigFile::get_config_path()
{
	
    return config_path_;
}

bool CConfigFile::get(const CConfigName& cfgname, CConfigValue& cfgvalue, bool is_write_log)
{
	if (!cfgname.is_valid())
	{
		return false;
	}

	return true;
}

