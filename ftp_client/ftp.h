#include <map>
#include <string>
#include "socket.h"

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
