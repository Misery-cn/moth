#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "utils.h"
#include "log.h"
#include "config.h"

void global_init()
{
    // 获取配置文件路径
    std::string path = Utils::get_program_path();
    INFO_LOG("current program path is %s", path.c_str());

    std::string config_file;

    int pos = path.rfind('/');
    if (0 < pos)
    {
        config_file = path.substr(0, pos) + "/conf/moth.conf";
    }
    else
    {
        // exit(1);
    }

    // 检查配置文件是否存在
    INFO_LOG("current program config file path is %s", config_file.c_str());
    if (!FileUtils::exist(config_file.c_str()))
    {
        ERROR_LOG("config file not exist");
        exit(1);
    }

    sconfig.parse_file(config_file);
    
    // 根据配置文件更新日志级别
    uint32_t log_level;
    sconfig.get_val_as_int("global", "log_level", log_level);
    slog.set_log_level(log_level);
}

#endif
