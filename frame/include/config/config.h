#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

#include "util.h"

const int MAX_FILE_NAME = 256;

const int MAX_LINE_LEN = 1024;

namespace sys
{

class CConfigName
{
public:
	// 不同的节点下可能有重名的配置项,所以要加上section
	CConfigName(char* filename = NULL, char* sectionname = NULL, char* itemname = NULL)
			: file_name_(filename), section_name_(sectionname), item_name_(itemname)
	{
		// 
	}
	
	virtual ~CConfigName(){}
	
	bool is_valid() const
	{
		return (NULL != file_name_ && NULL != section_name_ && NULL != item_name_);
	}
	
	bool operator==(const CConfigName& other)
	{
		if (this != &other)
		{
			file_name_ = other.file_name_;
			section_name_ = other.section_name_;
			item_name_ = other.item_name_;
		}
		
		return true;
	}
	
	void set_file_name(char* filename) {file_name_ = filename;}
	
	const char* get_file_name() const { return file_name_; }
	
	void set_section_name(char* section) {section_name_ = section;}
	
	const char* get_section_name() const { return section_name_;}
	
	void set_item_name(char* item) {item_name_ = item;}
	
	const char* get_item_name() const { return item_name_; }
	
private:
    // google style
	CConfigName(const CConfigName& c);
	CConfigName & operator=(CConfigName& c);
	
	// 配置文件名(全路径)
	char* file_name_;
	// section的名字
	char* section_name_;
	// 配置项名
	char* item_name_;
};

class CConfigValue
{
	enum{TYPE_INT, TYPE_STR};
	
public:
	CConfigValue() : value_type_(TYPE_INT), int_value_(0), str_value_(NULL)
	{
		//
	}
	
	virtual ~CConfigValue()
	{
		if (TYPE_STR == value_type_)
        {
			//use inline not Macro function
            DELETE_P(str_value_);
        }
	}
	
	CConfigValue& operator=(int i)
	{
		if (TYPE_STR == value_type_)
		{
			DELETE_P(str_value_);
		}
		
		value_type_ = TYPE_INT;
		int_value_ = i;
		
		return *this;
	}
	
	CConfigValue& operator=(const char* str)
	{
		value_type_ = TYPE_STR;
		int_value_ = 0;
		
		if (NULL == str)
		{
			DELETE_P(str_value_);
		}
		else
		{
			DELETE_P(str_value_);
			
			str_value_ = new char[strlen(str) + 1];
			if (NULL == str_value_)
			{
				return *this;
			}
			
			strcpy(str_value_, str);
		}
		
		
		return *this;
	}
	
	int get_type()
	{
		return value_type_;
	}

	int as_int()
	{
		return int_value_;
	}

	const char* as_string()
	{
		if (NULL == str_value_)
		{
			return "";
		}
		
		return str_value_;
	}
	
private:
	int value_type_;
	int int_value_;
	char* str_value_;
};

// 配置文件类
class CConfigFile
{
public:
	CConfigFile() {};
	virtual ~CConfigFile() {};
	
	// 获取配置项值
	static bool get(const CConfigName& cfgname, CConfigValue& cfgvalue, bool is_write_log = false);

    static bool set(const CConfigName& cfgname, const CConfigValue& cfgvalue);
    
    static bool get_config(const char* section, const char* item, const char* value, int maxLen, const char* filename);
    static bool set(const char* filename, const char* section, const char* item, const char* value);
    static bool set(const char* filename, const char* section, const char* item, int value);
    static const char* get_config_path();
	
private:

    static char config_path_[MAX_FILE_NAME];
};

}

#endif
