#include "config.h"

namespace sys
{

char CConfigFile::config_path_[MAX_FILE_NAME] = {0};

const char* CConfigFile::get_config_path()
{
    if (0 == strlen(config_path_))
    {
        if (!utils::get_work_dir(config_path_))
        {			
            return NULL;
        }
		
        strcat(config_path_, "/config");
    }
	
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

}