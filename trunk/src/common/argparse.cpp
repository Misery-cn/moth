#include "argparse.h"

void argv_to_vec(int argc, const char* argv[], std::vector<const char*>& args)
{
    for (int i = 1; i < argc; i++)
    {
        args.push_back(argv[i]);
    }
}

void parse_config_options(std::vector<const char*>& args)
{

}


