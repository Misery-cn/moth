#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "singleton.h"
#include "config_utils.h"

#define sconfig Singleton<ConfFile>::instance()

#endif