#ifndef _UTILS_TOKENER_H_
#define _UTILS_TOKENER_H_

#include <list>
#include <map>
#include <string>
#include <vector>
#include "string_utils.h"


// UTILS_NS_BEGIN

class CTokener
{
public:
	// 以指定的字符串作为分隔符，将整个字符中各Token解析到一个容器中
	// tokens:存储Token的容器，可为list或vector等，只要它支持push_back()
	// source:被解析的字符串
	// sep:Token分隔符
	// skip_sep:是否跳过连续的sep，即如果连接相同的sep存在，则只算作一个sep
    template <class ContainerType>
    static int split(ContainerType* tokens, const std::string& source, const std::string& sep, bool skip_sep = false)
    {
        if (sep.empty())
        {
            tokens->push_back(source);
        }
        else if (!source.empty())
        {
            std::string str = source;
            std::string::size_type pos = str.find(sep);

            for (;;)
            {
                std::string token = str.substr(0, pos);
                tokens->push_back(token);

                if (std::string::npos == pos)
                {
                    break;
                }
				
                if (skip_sep)
                {
                    bool end = false;
                    while (0 == strncmp(sep.c_str(), &str[pos + 1], sep.size()))
                    {
                        pos += sep.size();
                        if (pos >= str.size())
                        {
                            end = true;
                            tokens->push_back(std::string(""));
                            break;
                        }
                    }

                    if (end)
                    {
                        break;
                    }
                }

                str = str.substr(pos + sep.size());
                pos = str.find(sep);
            }
        }

        return static_cast<int>(tokens->size());
    }
};

// UTILS_NS_END

#endif
