
#define IN
#define OUT

// 待完成RUNLOG
#define RUNLOG printf

#define DELETE_P(p) \
	if (p) \
	{ \
		delete p; \
		p = NULL; \
	}
	
#define WORK_DIR "WORK_DIR"
	
// socket结果码
enum {
	SOCKET_ERROR = 100,
	SOCKET_CLOSE_ERROR,
	SOCKET_SET_ERROR,
	SOCKET_BIND_ERROR,
	SOCKET_LISTEN_ERROR,
	SOCKET_ACCEPT_ERROR,
	SOCKET_CONNECT_ERROR,
	SOCKET_READ_ERROR,
	SOCKET_WRITE_ERROR,
	SOCKET_TIME_OUT
};

// 已知的FTP结果码
enum {
	// FTP已建立连接
	FTP_CONNECTED = 220,
	// FTP用户名正常
	FTP_USER_RIGHT = 331,
	// FTP用户密码正确
	FTP_PASS_RIGHT = 230,
	// FTP进入文件存放目录正常
	FTP_CWD_RIGHT = 250,
	// FTP设置编码格式正常
	FTP_TYPE_RIGHT = 200,
	// FTP进入被动模式正常
	FTP_PASV_RIGHT = 227,
	// FTP进入主动模式正常
	FTP_PORT_RIGHT = 200,
	// FTP上传命令执行正常,开始传输
	FTP_STOR_TRANS = 125,
	// FTP上传命令执行正常
	FTP_STOR_RIGHT = 150,
	// FTP下载命令执行正常,开始传输
	FTP_RETR_TRANS = 125,
	// FTP下载命令执行正常
	FTP_RETR_RIGHT = 150,
	// FTP上传完成
	FTP_PUT_OVER = 226,
	// FTP下载完成
	FTP_GET_OVER = 226,
	// FTP文件行为完成
	FTP_FILE_OVER = 250,
	// FTP退出成功
	FTP_QUIT_SUCC = 221,
	// 此为自定义结果码
	FTP_COMMAND_NOT_FOUND = 299,
};

//文件操作错误码
#define FILE_OPEN_ERROR			-200	//打开文件错误
//数据流操作错误码
#define DATA_READ_ERROR			-300	//读数据错误
#define DATA_WRITE_ERROR		-301	//写数据错误
//FTP操作错误码
#define FTP_TIME_OUT			-1000	//FTP超时
#define FTP_CONNECT_ERROR		-1001	//FTP连接异常
#define FTP_USER_ERROR			-1002	//FTP用户名错误
#define FTP_PASS_ERROR			-1003	//FTP密码错误
#define FTP_CWD_ERROR			-1004	//FTP进入文件存放目录异常
#define FTP_TYPE_ERROR			-1005	//FTP设置编码格式异常
#define FTP_PASV_ERROR			-1006	//FTP进入被动模式异常
#define FTP_PASV_IP_ERROR		-1007	//FTP数据传输IP地址获取错误
#define FTP_PORT_ERROR			-1008	//FTP进入主动模式异常
#define FTP_STOR_ERROR			-1009	//FTP上传命令执行异常
#define FTP_RETR_ERROR			-1010	//FTP下载命令执行异常
#define FTP_PUT_ERROR			-1011	//FTP上传失败
#define FTP_GET_ERROR			-1012	//FTP下载失败
#define FTP_QUIT_ERROR			-1013	//FTP退出失败
