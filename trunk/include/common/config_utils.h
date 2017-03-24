#include <deque>
#include <map>
#include <set>
#include <string>

class CConfLine
{
public:
	CConfLine(const std::string& key, const std::string val,
		const std::string section, const std::string comment, int lineno);
		
	bool operator<(const CConfLine& oth) const;
	
public:
	friend std::ostream& operator<<(std::ostream& oss, const CConfLine& line);
	
public:
	// 配置项名
	std::string _key;
	// 配置项的值
	std::string _val;

	std::string _section;
};


class CConfSection
{
public:
	typedef std::set<CConfLine>::const_iterator const_line_iter_t;

	std::set<CConfLine> _lines;
};

class CConfFile
{
public:
	typedef std::map<std::string, CConfSection>::iterator section_iter_t;
	typedef std::map<std::string, CConfSection>::const_iterator const_section_iter_t;

	CConfFile();
	~CConfFile();
	
	void clear();

	int parse_file(const std::string& filename);

	int read(const std::string& section, const std::string& key, std::string& val) const;
	
	const_section_iter_t sections_begin();
	const_section_iter_t sections_end();
	
	static void trim_whitespace(std::string& str, bool strip_internal);

	static std::string normalize_key_name(const std::string& key);
	
public:
	friend std::ostream& operator<<(std::ostream& oss, const CConfFile& cf);

private:
	void load_from_buffer(const char* buf, size_t sz);

	// 解析一行配置文件
	static CConfLine* process_line(int line_no, const char* line);

	std::map<std::string, CConfSection> _sections;
};
