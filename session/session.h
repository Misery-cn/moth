
class CSession
{
public:
	CSession();
	virtual ~CSession();
private:
	// 会话ID
	int sessionID_;
	// 状态
	// 会话创建时间
	// 会话超时时间
};

class CSessionMgr
{
public:
	CSessionMgr();
	virtual ~CSessionMgr();
private:
	
};