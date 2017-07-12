#ifndef _ARGPARSE_H_
#define _ARGPARSE_H_

#include <vector>

void argv_to_vec(int argc, const char* argv[], std::vector<const char*>& args);

void parse_config_options(std::vector<const char*>& args);


#endif
