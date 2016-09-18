#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include "config_utils.h"

CConfLine::CConfLine(const std::string& key, const std::string val,
      const std::string section, const std::string comment, int lineno)
	: key_(key), val_(val), section_(section)
{
}

bool CConfLine::operator<(const CConfLine& oth) const
{
	if (key_ < oth.key_)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::ostream& operator<<(std::ostream& oss, const CConfLine& line)
{
	oss << line.key_ << line.val_ << line.section_;
	return oss;
}

CConfFile::CConfFile()
{
}

CConfFile::~CConfFile()
{
}

void CConfFile::clear()
{
	sections_.clear();
}

int CConfFile::parse_file(const std::string& filename)
{
	clear();
	int r = 0;
	size_t sz = 0;
	char* buf = NULL;
	
	// 打开配置文件
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp)
	{
		r = errno;
		return r;
	}
	
	do 
	{	
		struct stat st_buf;

		if (fstat(fileno(fp), &st_buf))
		{
			r = -EINVAL;
			break;
		}
		
		// 检查配置文件大小
		// if (MAX_CONFIG_FILE_SZ < st_buf.st_size)
		// {
		// }
		
		sz = (size_t)st_buf.st_size;
		buf = new char[sz];
		if (!buf)
		{
			r = -EINVAL;
			break;
		}

		if (sz != fread(buf, 1, sz, fp))
		{
			if (ferror(fp))
			{
				r = -errno;
				break;
			}
			else
			{
				r = -EIO;
				break;
			}
		}
	
		load_from_buffer(buf, sz);
		
		r = 0;
		
	} while (false);

	if (buf)
	{
		delete buf;
		buf = NULL;
	}
	
	fclose(fp);
	
	return r;
}

void CConfFile::load_from_buffer(const char* buf, size_t sz)
{
	section_iter_t cur_section;

	const char* b = buf;
	int line_no = 0;
	size_t line_len = -1;
	size_t rem = sz;
	std::string acc;

	while (1)
	{
		b += line_len + 1;
		rem -= line_len + 1;
		if (0 == rem)
		{
			break;
		}
		
		// 行号+1
		line_no++;
		// 查找换行
		const char* end = (const char*)memchr(b, '\n', rem);

		// 没有换行
		if (!end)
		{
			break;
		}
		
		line_len = 0;
		bool found_null = false;
		// 查找该行是否有结束符
		for (const char* i = b; i != end; ++i)
		{
			// 该行长度+1
			line_len++;
			if ('\0' == *i)
			{
				found_null = true;
			}
		}

		if (found_null)
		{
			continue;
		}

		// if (check_utf8(b, line_len))
		// {
		// }
		
		// 已反斜杠结尾
		if ((1 <= line_len) && ('\\' == b[line_len - 1]))
		{
			// 把该行保存起来
			acc.append(b, line_len - 1);
			continue;	
		}
	
		acc.append(b, line_len);
		
		// 解析该行
		CConfLine* cline = process_line(line_no, acc.c_str());
		
		// 清空
		acc.clear();

		// 该行没有有效配置项
		if (!cline)
		{
			continue;
		}
		
		const std::string& csection(cline->section_);
		// 该行包含section
		if (!csection.empty())
		{
			// 保存section
			std::map<std::string, CConfSection>::value_type vt(csection, CConfSection());
			std::pair<section_iter_t, bool> nr(sections_.insert(vt));
			cur_section = nr.first;
		}
		else
		{
			// 是否重复
			if (cur_section->second.lines_.count(*cline))
			{
				cur_section->second.lines_.erase(*cline);
				if (cline->key_.length())
				{
					// 配置项重复了
				}
			}
			// [section]
			// test = 1
			// test = 2
			// 取最后一次test=2
			cur_section->second.lines_.insert(*cline);
		}

		delete cline;
	}
	
	if (!acc.empty())
	{
		// 错误行
	}	
}

CConfLine* CConfFile::process_line(int line_no, const char* line)
{
	enum acceptor_state_t
	{
		ACCEPT_INIT,
		ACCEPT_SECTION_NAME,
		ACCEPT_KEY,
		ACCEPT_VAL_START,
		ACCEPT_UNQUOTED_VAL,
		ACCEPT_QUOTED_VAL,
		ACCEPT_COMMENT_START,
		ACCEPT_COMMENT_TEXT,
	};

	const char* l = line;
	acceptor_state_t state = ACCEPT_INIT;

	std::string key, val, section, comment;
	bool escaping = false;

	while (1)
	{
		char c = *l++;
		switch (state)
		{
			case ACCEPT_INIT:
			{
				// 结束符
				if ('\0' == c)
				{
					return NULL;
				}
				// section
				else if ('[' == c)
				{
					state = ACCEPT_SECTION_NAME;
				}
				// 注释
				else if (('#' == c) || ';' == c)
				{
					state = ACCEPT_COMMENT_TEXT;
				}
				// 错误
				else if (']' == c)
				{
					return NULL;
				}
				else if (isspace(c))
				{
					// 空格
				}
				else
				{
					// 普通的字符,可能是配置项的名字
					state = ACCEPT_KEY;
					// 此处要回退到配置项的头,由ACCEPT_KEY处理
					--l;
				}
				break;
			}
			case ACCEPT_SECTION_NAME:
			{
				// 结束符
				if ('\0' == c)
				{
					return NULL;
				}
				// section结束
				else if ((']' == c) && (!escaping))
				{
					// 格式化
					trim_whitespace(section, true);
					if (section.empty())
					{
						return NULL;
					}
					
					// 检查是否有注释
					state = ACCEPT_COMMENT_START;
				}
				else if ((('#' == c) || (';' == c)) && !escaping)
				{
					return NULL;
				}
				// 反斜杠,继续
				else if (('\\' == c) && !escaping)
				{
					escaping = true;
				}
				else 
				{
					escaping = false;
					// 记录下section
					section += c;
				}
				break;
			}
			case ACCEPT_KEY:
			{
				if (((('#' == c) || (';' == c)) && !escaping) || ('\0' == c))
				{
					return NULL;
				}
				else if ('=' == c && !escaping)
				{
					// 格式化配置项名
					key = normalize_key_name(key);
					if (key.empty())
					{
						return NULL;
					}
					// 后面的字符有ACCEPT_VAL_START处理
					state = ACCEPT_VAL_START;
				}
				else if ('\\' == c && !escaping)
				{
					escaping = true;
				}
				else
				{
					escaping = false;
					// 记录写配置项名
					key += c;
				}
				break;
			}
			case ACCEPT_VAL_START:
			{
				if ('\0' == c)
				{
					return new CConfLine(key, val, section, comment, line_no);
				}
				else if ('#' == c || ';' == c)
				{
					state = ACCEPT_COMMENT_TEXT;
				}
				// 配置项的值有引号包含
				else if ('"' == c)
				{
					state = ACCEPT_QUOTED_VAL;
				}
				else if (isspace(c))
				{
				}
				else
				{
					// 没有引号包含
					state = ACCEPT_UNQUOTED_VAL;
					// 回退
					--l;
				}
				break;
			}
			case ACCEPT_UNQUOTED_VAL:
			{
				if ('\0' == c)
				{
					if (escaping)
					{
						return NULL;
					}
					
					trim_whitespace(val, false);
					return new CConfLine(key, val, section, comment, line_no);
				}
				else if ((('#' == c) || (';' == c)) && !escaping)
				{
					trim_whitespace(val, false);
					state = ACCEPT_COMMENT_TEXT;
				}
				else if ('\\' == c && !escaping)
				{
					escaping = true;
				}
				else
				{
					escaping = false;
					// 记录配置项的值
					val += c;
				}
				break;
			}
			case ACCEPT_QUOTED_VAL:
			{
				if ('\0' == c)
				{
					return NULL;
				}
				else if ('"' == c && !escaping)
				{
					state = ACCEPT_COMMENT_START;
				}
				else if ('\\' == c && !escaping)
				{
					escaping = true;
				}
				else
				{
					escaping = false;
					// 记录下配置项的值
					val += c;
				}
				break;
			}
			case ACCEPT_COMMENT_START:
			{
				if ('\0' == c)
				{
					return new CConfLine(key, val, section, comment, line_no);
				}
				else if ('#' == c || ';' == c)
				{
					state = ACCEPT_COMMENT_TEXT;
				}
				else if (isspace(c))
				{
				}
				else
				{
					return NULL;
				}
				break;
			}
			case ACCEPT_COMMENT_TEXT:
			{
				if ('\0' == c)
				{
					return new CConfLine(key, val, section, comment, line_no);
				}
				else
				{
					comment += c;
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

void CConfFile::trim_whitespace(std::string& str, bool strip_internal)
{
	const char* in = str.c_str();
	// 去掉头部空字符
	while (1)
	{
		char c = *in;
		if (!c || !isspace(c))
		{
			break;
		}
		++in;
	}
	
	char output[strlen(in) + 1];
	strcpy(output, in);

	// 去掉尾部空字符
	char* o = output + strlen(output);
	while (1)
	{
		if (o == output)
		{
			break;
		}
		--o;
		
		if (!isspace(*o))
		{
			++o;
			*o = '\0';
			break;
		}
	}

	if (!strip_internal)
	{
		str.assign(output);
		return;
	}

	char output2[strlen(output) + 1];
	char* out2 = output2;
	bool prev_was_space = false;

	// 去掉中间的空字符
	for (char* u = output; *u; ++u)
	{
		char c = *u;
		if (isspace(c))
		{
			if (!prev_was_space)
			{
				*out2++ = c;
				prev_was_space = true;
			}
		}
		else
		{
			*out2++ = c;
			prev_was_space = false;
		}
	}
	
	*out2++ = '\0';
	str.assign(output2);
}

std::string CConfFile::normalize_key_name(const std::string& key)
{
	std::string k(key);
	CConfFile::trim_whitespace(k, true);
	std::replace(k.begin(), k.end(), ' ', '_');
	return k;
}

int CConfFile::read(const std::string& section, const std::string& key, std::string& val) const
{
	std::string k(normalize_key_name(key));
	const_section_iter_t s = sections_.find(section);
	if (s == sections_.end())
	{
		return -ENOENT;
	}

	CConfLine line(k, "", "", "", 0);
	CConfSection::const_line_iter_t l = s->second.lines_.find(line);
	if (l == s->second.lines_.end())
	{
		return -ENOENT;
	}

	val = l->val_;
	return 0;
}

CConfFile::const_section_iter_t CConfFile::sections_begin()
{
	return sections_.begin();
}

CConfFile::const_section_iter_t CConfFile::sections_end()
{
	return sections_.end();
}
 
int main()
{
	CConfFile cf;
	std::string val;
	
	cf.parse_file("/home/rhino/zkb/test.conf");
	
	cf.read("test2", "hehe and haha", val);
	
	std::cout << val << std::endl;
	return 0;
}
