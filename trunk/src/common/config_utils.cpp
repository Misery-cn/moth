#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <iostream>
#include "config_utils.h"
#include "string_utils.h"

ConfLine::ConfLine(const std::string& key, const std::string val,
      const std::string section, const std::string comment, int lineno)
    : _key(key), _val(val), _section(section)
{
}

bool ConfLine::operator<(const ConfLine& oth) const
{
    if (_key < oth._key)
    {
        return true;
    }
    else
    {
        return false;
    }
}

std::ostream& operator<<(std::ostream& oss, const ConfLine& line)
{
    oss << line._key << line._val << line._section;
    return oss;
}

ConfFile::ConfFile()
{
}

ConfFile::ConfFile(const std::string& filename)
{
    parse_file(filename);
}


ConfFile::~ConfFile()
{
}

void ConfFile::clear()
{
    _sections.clear();
}

int ConfFile::parse_file(const std::string& filename)
{
    clear();
    int r = 0;
    size_t sz = 0;
    char* buf = NULL;
    
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

void ConfFile::load_from_buffer(const char* buf, size_t sz)
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
        
        line_no++;
        
        const char* end = (const char*)memchr(b, '\n', rem);

        if (!end)
        {
            break;
        }
        
        line_len = 0;
        bool found_null = false;

        for (const char* i = b; i != end; ++i)
        {
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
        
        if ((1 <= line_len) && ('\\' == b[line_len - 1]))
        {
            acc.append(b, line_len - 1);
            continue;    
        }
    
        acc.append(b, line_len);
        
        ConfLine* cline = process_line(line_no, acc.c_str());
        
        acc.clear();

        if (!cline)
        {
            continue;
        }
        
        const std::string& csection(cline->_section);

        if (!csection.empty())
        {
            std::map<std::string, ConfSection>::value_type vt(csection, ConfSection());
            std::pair<section_iter_t, bool> nr(_sections.insert(vt));
            cur_section = nr.first;
        }
        else
        {
            if (cur_section->second._lines.count(*cline))
            {
                cur_section->second._lines.erase(*cline);
                if (cline->_key.length())
                {
                    // 
                }
            }

            cur_section->second._lines.insert(*cline);
        }

        delete cline;
    }
    
    if (!acc.empty())
    {
        // 
    }    
}

ConfLine* ConfFile::process_line(int line_no, const char* line)
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
                if ('\0' == c)
                {
                    return NULL;
                }
                // section
                else if ('[' == c)
                {
                    state = ACCEPT_SECTION_NAME;
                }
                else if (('#' == c) || ';' == c)
                {
                    state = ACCEPT_COMMENT_TEXT;
                }
                else if (']' == c)
                {
                    return NULL;
                }
                else if (isspace(c))
                {
                    // 
                }
                else
                {
                    state = ACCEPT_KEY;
                    --l;
                }
                break;
            }
            case ACCEPT_SECTION_NAME:
            {
                if ('\0' == c)
                {
                    return NULL;
                }
                else if ((']' == c) && (!escaping))
                {
                    trim(section, true);
                    if (section.empty())
                    {
                        return NULL;
                    }
                    
                    state = ACCEPT_COMMENT_START;
                }
                else if ((('#' == c) || (';' == c)) && !escaping)
                {
                    return NULL;
                }
                else if (('\\' == c) && !escaping)
                {
                    escaping = true;
                }
                else 
                {
                    escaping = false;
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
                    key = normalize_key_name(key);
                    
                    if (key.empty())
                    {
                        return NULL;
                    }
                    
                    state = ACCEPT_VAL_START;
                }
                else if ('\\' == c && !escaping)
                {
                    escaping = true;
                }
                else
                {
                    escaping = false;
                    key += c;
                }
                break;
            }
            case ACCEPT_VAL_START:
            {
                if ('\0' == c)
                {
                    return new ConfLine(key, val, section, comment, line_no);
                }
                else if ('#' == c || ';' == c)
                {
                    state = ACCEPT_COMMENT_TEXT;
                }
                else if ('"' == c)
                {
                    state = ACCEPT_QUOTED_VAL;
                }
                else if (isspace(c))
                {
                }
                else
                {
                    state = ACCEPT_UNQUOTED_VAL;
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
                    
                    trim(val, false);
                    return new ConfLine(key, val, section, comment, line_no);
                }
                else if ((('#' == c) || (';' == c)) && !escaping)
                {
                    trim(val, false);
                    state = ACCEPT_COMMENT_TEXT;
                }
                else if ('\\' == c && !escaping)
                {
                    escaping = true;
                }
                else
                {
                    escaping = false;
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
                    val += c;
                }
                break;
            }
            case ACCEPT_COMMENT_START:
            {
                if ('\0' == c)
                {
                    return new ConfLine(key, val, section, comment, line_no);
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
                    return new ConfLine(key, val, section, comment, line_no);
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

void ConfFile::trim(std::string& str, bool strip_internal)
{
    const char* in = str.c_str();
    
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

std::string ConfFile::normalize_key_name(const std::string& key)
{
    std::string k(key);
    ConfFile::trim(k, true);
    std::replace(k.begin(), k.end(), ' ', '_');
    return k;
}

int ConfFile::get_val(const std::string& section, const std::string& key, std::string& val) const
{
    std::string k(normalize_key_name(key));
    const_section_iter_t s = _sections.find(section);
    if (s == _sections.end())
    {
        return -ENOENT;
    }

    ConfLine line(k, "", "", "", 0);
    ConfSection::const_line_iter_t l = s->second._lines.find(line);
    if (l == s->second._lines.end())
    {
        return -ENOENT;
    }

    val = l->_val;
    return 0;
}

bool ConfFile::get_val_as_int(const std::string& section, const std::string& key, uint32_t& val) const
{
    std::string strval;
    
    if (get_val(section, key, strval))
    {
        return false;
    }

    // 是否全是数字
    if (!StringUtils::is_digit_string(strval.c_str()))
    {
        return false;
    }
    
    return StringUtils::string2int(strval.c_str(), val);
}

ConfFile::const_section_iter_t ConfFile::sections_begin()
{
    return _sections.begin();
}

ConfFile::const_section_iter_t ConfFile::sections_end()
{
    return _sections.end();
}

std::vector<std::string> ConfFile::get_all_sections()
{
    Mutex::Locker locker(_lock);
    
    std::vector<std::string> secs;

    const_section_iter_t l = sections_begin();

    for (; l != sections_end(); l++)
    {
        secs.push_back(l->first);
    }

    return secs;
}

