#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_

#include <map>
#include <string>
#include "socket.h"

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


// 全局函数指针
typedef int (*gfunc)(void);

class FtpClient
{
public:
    FtpClient();
    virtual ~FtpClient();
	
	struct ftp_param_s
	{
		// 客户端要连接的主机ip
		char* ip_;
		// 客户端要连接的主机端口
		int port_;
		// ftp用户名
		char* usr_;
		// ftp用户的密码
		char* pwd_;
		// 远程目录
		char* desDir_;
		// 本地目录
		char* localDir_;
		// 传输的文件名
		char* filename_;
		// 传输方式(A - ASCII,I - bin,默认二进制)
		char type_;
		// 被动传输模式下服务器返回的ip
		// char* pasv_ip_;
		// 被动传输模式下服务器返回的端口
		// 往这个端口发送数据
		int pasv_port_;
		
		ftp_param_s()
		{
			ip_ = NULL;
			port_ = 0;
			usr_ = NULL;
			pwd_ = NULL;
			desDir_ = NULL;
			localDir_ = NULL;
			filename_ = NULL;
			// Binary mode
			type_ = 'I';
			// 暂不考虑公网(公网需要ip映射)
			// pasv_ip_ = new char[MAX_IP_LEN];
			pasv_port_ = 0;
		}
		
		~ftp_param_s()
		{
			// if (pasv_ip_)
			// {
			//     delete[] pasv_ip_; 
			// }
		}
	};
	
	// 该类的函数指针
	typedef int (FtpClient::*func)(ftp_param_s* param);
	
    enum {
        MKD = 1,	// 创建目录
        CWD = 2,	// 进入到工作目录
		USR = 3,	// 用户名
		PWD = 4,	// 用户密码
		TYPE = 5,	// 传输方式(asccii和二进制)
		PASV = 6,	// 传输模式(主动和被动)
		STOR = 7,	// 上传文件
		RETR = 8,	// 下载文件
		QUIT = 9	// 退出
    };
	
private:
	int pre_transfer();
	
    // 连接ftp服务器
	int create_cmd_link(char* ip, int port);
	int create_data_link(char* ip, int port);
	
    // 发送ftp命令
    int execute_cmd(int cmd);
    int mkd(ftp_param_s* param);
	int usr(ftp_param_s* param);
	int pwd(ftp_param_s* param);
	int type(ftp_param_s* param);
	int pasv(ftp_param_s* param);
	int cwd(ftp_param_s* param);
	int stor(ftp_param_s* param);
	int retr(ftp_param_s* param);
	int quit(ftp_param_s* param);
	
	int put_file();
	int puting();
	int get_file();
	int geting();
	
	int ftp_write(sys::CSocket* sock, char* buff);
	
	int ftp_read(sys::CSocket* sock, char* buff, int len);
	
	int parse_pasv_result(char* result);
    // 递归创建目录
    int recursive_mkd();
	
public:
    // 上传文件
    int upload(char* ip, int port, char* usr, char* pwd, char* desDir, char* localDir, char* filename);
    // 下载文件
    int download(char* ip, int port, char* usr, char* pwd, char* desDir, char* localDir, char* filename);

private:
	// 命令连接
    sys::CSocket* sock_cmd_;
	// pasv数据传输连接
	sys::CSocket* sock_data_;
	ftp_param_s* param_;
    std::map<int, func> cmds_;
};

#endif
