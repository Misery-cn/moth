#include <deque>
#include <map>
#include <set>
#include <string>

class ConfLine
{
public:
	ConfLine(const std::string& key, const std::string val,
		const std::string section, const std::string comment, int lineno);
		
	bool operator<(const ConfLine& oth) const;
	
public:
	friend std::ostream& operator<<(std::ostream& oss, const ConfLine& line);
	
public:
	// 配置项名
	std::string _key;
	// 配置项的值
	std::string _val;

	std::string _section;
};


class ConfSection
{
public:
	typedef std::set<ConfLine>::const_iterator const_line_iter_t;

	std::set<ConfLine> _lines;
};

class ConfFile
{
public:
	typedef std::map<std::string, ConfSection>::iterator section_iter_t;
	typedef std::map<std::string, ConfSection>::const_iterator const_section_iter_t;

	ConfFile();
	ConfFile(const std::string& filename);
	virtual ~ConfFile();
	
	void clear();

	int parse_file(const std::string& filename);

	int get_val(const std::string& section, const std::string& key, std::string& val) const;
	
	const_section_iter_t sections_begin();
	const_section_iter_t sections_end();
	
	static void trim(std::string& str, bool strip_internal);

	static std::string normalize_key_name(const std::string& key);
	
public:
	friend std::ostream& operator<<(std::ostream& oss, const ConfFile& cf);

private:
	void load_from_buffer(const char* buf, size_t sz);

	// 解析一行配置文件
	static ConfLine* process_line(int line_no, const char* line);

	std::map<std::string, ConfSection> _sections;
};
