#include "ftp.h"

#define REG_CMD(cmd, func) \
    cmds_.insert(std::make_pair(cmd, func));

FtpClient::FtpClient()
{
    sock_cmd_ = new sys::CSocket();
	sock_data_ = new sys::CSocket();
	param_ = new ftp_param_s();

    REG_CMD(MKD, &FtpClient::mkd);
	REG_CMD(USR, &FtpClient::usr);
	REG_CMD(PWD, &FtpClient::pwd);
	REG_CMD(TYPE, &FtpClient::type);
	REG_CMD(PASV, &FtpClient::pasv);
	REG_CMD(CWD, &FtpClient::cwd);
	REG_CMD(STOR, &FtpClient::stor);
	REG_CMD(RETR, &FtpClient::retr);
	REG_CMD(QUIT, &FtpClient::quit);
}

FtpClient::~FtpClient()
{
	DELETE_P(sock_cmd_);
	DELETE_P(sock_data_);
	DELETE_P(param_);
}

int FtpClient::mkd(ftp_param_s* param)
{
	return NO_ERROR;
}

int FtpClient::usr(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送user命令
   	snprintf(buff, BUFF_SIZE, "USER %s\r\n", param->usr_);

	RUNLOG("USER %s\n", param->usr_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取user命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP用户名不对
	if (FTP_USER_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::pwd(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送pass命令
   	snprintf(buff, BUFF_SIZE, "PASS %s\r\n", param->pwd_);

	RUNLOG("PASS %s\n", param->pwd_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取pass命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP用户密码错误
	if (FTP_PASS_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_PASS_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::type(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送type命令
   	snprintf(buff, BUFF_SIZE, "TYPE %c\r\n", param->type_);

	RUNLOG("TYPE %c\n", param->type_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取type命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP设置传输方式错误
	if (FTP_TYPE_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_TYPE_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::pasv(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送pasv命令
   	strcpy(buff, "PASV\r\n");

	RUNLOG("PASV\n");

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取pasv命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP设置被动模式错误
	if (FTP_PASV_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_PASV_ERROR;
	}
	
	return parse_pasv_result(buff);;
}

int FtpClient::parse_pasv_result(char* result)
{
	// 返回值如下：(127,0,0,1,28,6)
	if (result)
	{
		int ilPos = strcspn(result, "(");
		int irPos = strcspn(result, ")");
		
		// 取出括号中的内容
		char buf[30 + 1] = {0};
		strncpy(buf, result + ilPos + 1, irPos - ilPos - 1);
		
		int a=0,b=0,c=0,d=0,e=0,f=0;
		int n = sscanf(buf, "%d,%d,%d,%d,%d,%d", &a, &b, &c, &d, &e, &f);
		if (6 != n)
		{
			return FTP_PASV_IP_ERROR;
		}

		param_->pasv_port_ = e * 256 + f;
		// ip就不解析了
		// sprintf(param_->pasv_ip_, "%d.%d.%d.%d", a, b, c, d);
		return NO_ERROR;
	}
	
	return ALLOC_MEM_ERROR;
}

int FtpClient::cwd(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送进入工作目录命令
   	snprintf(buff, BUFF_SIZE, "CWD %s\r\n", param->desDir_);

	RUNLOG("CWD %s\n", param->desDir_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取cwd命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP进入工作目录错误
	if (FTP_CWD_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_CWD_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::stor(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送stor上传文件命令
   	snprintf(buff, BUFF_SIZE, "STOR %s\r\n", param->filename_);

	RUNLOG("STOR %s\n", param->filename_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取上传文件命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// 上传文件命出错
	if (FTP_STOR_TRANS != atol(buff) && FTP_STOR_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_STOR_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::retr(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送下载文件命令
   	snprintf(buff, BUFF_SIZE, "RETR %s\r\n", param->filename_);

	RUNLOG("RETR %s\n", param->filename_);

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取下载文件命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// 下载文件命出错
	if (FTP_RETR_TRANS != atol(buff) && FTP_RETR_RIGHT != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_RETR_ERROR;
	}

	return NO_ERROR;
}

int FtpClient::quit(ftp_param_s* param)
{
	int r = NO_ERROR;
	char* buff = new char[BUFF_SIZE + 1];
	memset(buff, 0, BUFF_SIZE + 1);
	// 托管该资源
	utils::CPointGuard<char> g(buff);

	// 发送退出命令
   	strcpy(buff, "QUIT\r\n");

	RUNLOG("QUIT \n");

   	r = ftp_write(sock_cmd_, buff);
   	if (r)
   	{
   		RUNLOG("write message failed, [%d][%s]", r, strerror(errno));
   	    sock_cmd_->s_close();
   	    return FTP_USER_ERROR;
   	}

	// 读取退出命令返回消息
	memset(buff, 0, BUFF_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[BUFF_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// 客户端退出错误
	if (FTP_QUIT_SUCC != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_QUIT_ERROR;
	}
	
	sock_cmd_->s_close();

	return NO_ERROR;
}

int FtpClient::put_file()
{
	// 打开数据传输连接
	// 此处的ip和port是服务端返回的
	int r = create_data_link(param_->ip_, param_->pasv_port_);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	// 进入到目标目录
	r = execute_cmd(CWD);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	
	// 准备传输文件
	r = execute_cmd(STOR);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	
	return puting();
}

int FtpClient::puting()
{
	int len = 0;
	int fd = 0;
	int r = NO_ERROR;
	char* buff = new char[MAX_READ_SIZE + 1];
	memset(buff, 0, MAX_READ_SIZE + 1);
	utils::CPointGuard<char> g(buff);
	
	std::string f(param_->localDir_);
	f.append("/").append(param_->filename_);

	// 打开本地文件
	fd = open(f.c_str(), O_RDONLY);
	if (-1 == fd)
	{
		return FILE_OPEN_ERROR;
	}
	
	// 读取文件数据
	while (0 != (len = read(fd, buff, MAX_READ_SIZE)))
	{
		if (0 < len)
		{
			// 写数据
			char* p = buff;
			int wl = 0;
			while (0 != (wl = write(sock_data_->getfd(), p, len)))
			{
				// 全部写完
				if (wl == len)
				{
					break;
				}
				else if (0 < wl)
				{
					// 继续
					p = buff + wl;
					len = len - wl;
				}
				else
				{
					return DATA_WRITE_ERROR;
				}
			}
		}
		else
		{
			return DATA_READ_ERROR;
		}
	}
	
	// 关闭文件
	close(fd);

	// 关闭数据传输连接
	sock_data_->s_close();

	// 读取文件上传完成返回消息
	memset(buff, 0, MAX_READ_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, MAX_READ_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[MAX_READ_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP用户名不正常
	if (FTP_PUT_OVER != atol(buff) && FTP_FILE_OVER != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_PUT_ERROR;
	}
	
	return NO_ERROR;
}

int FtpClient::get_file()
{
	// 打开数据传输连接
	// 此处的ip和port是服务端返回的
	int r = create_data_link(param_->ip_, param_->pasv_port_);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	// 进入到目标目录
	r = execute_cmd(CWD);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	
	// 准备传输文件
	r = execute_cmd(RETR);
	if (r)
	{
		sock_data_->s_close();
		return r;
	}
	
	return geting();
}

int FtpClient::geting()
{
	int len = 0;
	int fd = 0;
	int r = NO_ERROR;
	char* buff = new char[MAX_READ_SIZE + 1];
	memset(buff, 0, MAX_READ_SIZE + 1);
	utils::CPointGuard<char> g(buff);
	
	std::string f(param_->localDir_);
	f.append("/").append(param_->filename_);

	// 打开本地文件
	fd = open(f.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);
	if (-1 == fd)
	{
		return FILE_OPEN_ERROR;
	}
	
	// 读取文件数据
	while (0 != (len = read(sock_data_->getfd(), buff, MAX_READ_SIZE)))
	{
		if (0 < len)
		{
			// 写数据
			char* p = buff;
			int wl = 0;
			while (0 != (wl = write(fd, p, len)))
			{
				// 全部写完
				if (wl == len)
				{
					break;
				}
				else if (0 < wl)
				{
					// 继续
					p = buff + wl;
					len = len - wl;
				}
				else
				{
					return DATA_WRITE_ERROR;
				}
			}
		}
		else
		{
			return DATA_READ_ERROR;
		}
	}
	
	// 关闭文件
	close(fd);

	// 关闭数据传输连接
	sock_data_->s_close();

	// 读取文件上传完成返回消息
	memset(buff, 0, MAX_READ_SIZE + 1);
	r = ftp_read(sock_cmd_, buff, MAX_READ_SIZE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	buff[MAX_READ_SIZE + 1] = '\0';
	RUNLOG("%s\n", buff);

	// FTP用户名不正常
	if (FTP_GET_OVER != atol(buff) && FTP_FILE_OVER != atol(buff))
	{
		sock_cmd_->s_close();
   	    return FTP_GET_ERROR;
	}
	
	return NO_ERROR;
}

int FtpClient::ftp_write(sys::CSocket* sock, char* buff)
{
	fd_set writefds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&writefds);
    // 打开fd的可写位
    FD_SET(sock->getfd(), &writefds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    // 检测套接字状态
    if (select(sock->getfd() + 1, NULL, &writefds, 0, &tv))
    {
        // 判断描述字sockfd的可写位是否打开,如果打开则写入数据
        if (FD_ISSET(sock->getfd(), &writefds))
        {
            // 写入数据
            // 返回所写字节数
            return sock->s_write(sock->getfd(), buff);
        }
    }
	
	return FTP_TIME_OUT;
}

int FtpClient::ftp_read(sys::CSocket* sock, char* buff, int len)
{
	fd_set readfds;
    fd_set errfds;
    struct timeval tv;

    // 初始化
    FD_ZERO(&readfds);
    // 打开sockfd的可读位
    FD_SET(sock->getfd(), &readfds);

    // 初始化
    FD_ZERO(&errfds);
    // 设置sockfd的异常位
    FD_SET(sock->getfd(), &errfds);

    tv.tv_sec = WAIT_TIME_OUT;
    tv.tv_usec = 0;

    // 检测套接字状态
    if (select(sock->getfd() + 1, &readfds, NULL, &errfds, &tv))
    {
        // 判断描述字sockfd的异常位是否打开,如果打开则表示产生错误
        if (FD_ISSET(sock->getfd(), &errfds))
        {
            return SOCKET_READ_ERROR;
        }

        // 判断描述字sockfd的可读位是否打开,如果打开则读取数据
        if (FD_ISSET(sock->getfd(), &readfds))
        {
            // 读数据
            // 返回实际所读的字节数
            return sock->s_read(sock->getfd(), buff, len);
        }
    }
	
	return FTP_TIME_OUT;
}

int FtpClient::execute_cmd(int cmd)
{
	std::map<int, func>::iterator i = cmds_.find(cmd);
    if (i == cmds_.end()) 
	{
		// not found
        return FTP_COMMAND_NOT_FOUND;
    } 
	else 
	{
		return (this->*(i->second))(param_);
    }
}

int FtpClient::create_cmd_link(char* ip, int port)
{
	if (sock_cmd_)
	{
		int r = NO_ERROR;
		char* buff = new char[BUFF_SIZE + 1];
		memset(buff, 0, BUFF_SIZE + 1);
		// 托管该资源
		utils::CPointGuard<char> g(buff);
		
		// 创建一个套接字
		r = sock_cmd_->s_open();
		if (r)
		{
			sock_cmd_->s_close();
			return r;
		}

		// 连接服务端
		r = sock_cmd_->s_connect(ip, port);
		if (r)
		{
			sock_cmd_->s_close();
			return r;
		}
		
		r = ftp_read(sock_cmd_, buff, BUFF_SIZE);
		if (r)
		{
			sock_cmd_->s_close();
			return r;
		}
		
		buff[BUFF_SIZE + 1] = '\0';
		RUNLOG("%s\n", buff);
		
		// 连接失败返回错误信息
		if (FTP_CONNECTED != atol(buff))
		{
			sock_cmd_->s_close();
			return FTP_CONNECT_ERROR;
		}
		
		return NO_ERROR;
	}
	
	return ALLOC_MEM_ERROR;
}

int FtpClient::create_data_link(char* ip, int port)
{
	// 数据连接不处理响应，直接返回
	if (sock_data_)
	{
		int r = NO_ERROR;
		// 创建一个套接字
		r = sock_data_->s_open();
		if (r)
		{
			sock_data_->s_close();
			return r;
		}

		// 连接服务端
		r = sock_data_->s_connect(ip, port);
		if (r)
		{
			sock_data_->s_close();
			return r;
		}
		
		return NO_ERROR;
	}
	
	return ALLOC_MEM_ERROR;
}

int FtpClient::pre_transfer()
{
	// 打开命令连接
	int r = create_cmd_link(param_->ip_, param_->port_);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 请求用户登录
	r = execute_cmd(USR);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 输入用户密码
	r = execute_cmd(PWD);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 设置二进制传输方式
	r = execute_cmd(TYPE);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 设置被动工作方式
	r = execute_cmd(PASV);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	return NO_ERROR;
}

int FtpClient::upload(char* ip, int port, char* usr, char* pwd, char* desDir, char* localDir, char* filename)
{
	RUNLOG("IP=[%s] ", ip);
	RUNLOG("port=[%d] ", port);
	RUNLOG("usr=[%s] ", usr);
	RUNLOG("pwd=[%s] ", pwd);
	RUNLOG("destDir=[%s] ", desDir);
	RUNLOG("localDir=[%s] ", localDir);
	RUNLOG("filename=[%s]\n", filename);
	
	param_->ip_ = ip;
	param_->port_ = port;
	param_->usr_ = usr;
	param_->pwd_ = pwd;
	param_->desDir_ = desDir;
	param_->localDir_ = localDir;
	param_->filename_ = filename;
	
	int r = NO_ERROR;
	// 登录并设置传输方式等
	r = pre_transfer();
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	r = put_file();
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 退出
	r = execute_cmd(QUIT);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	return NO_ERROR;
}

int FtpClient::download(char* ip, int port, char* usr, char* pwd, char* desDir, char* localDir, char* filename)
{
	RUNLOG("IP=[%s] ", ip);
	RUNLOG("port=[%d] ", port);
	RUNLOG("usr=[%s] ", usr);
	RUNLOG("pwd=[%s] ", pwd);
	RUNLOG("destDir=[%s] ", desDir);
	RUNLOG("localDir=[%s] ", localDir);
	RUNLOG("filename=[%s]\n", filename);
	
	param_->ip_ = ip;
	param_->port_ = port;
	param_->usr_ = usr;
	param_->pwd_ = pwd;
	param_->desDir_ = desDir;
	param_->localDir_ = localDir;
	param_->filename_ = filename;
	
	int r = NO_ERROR;
	// 登录并设置传输方式等
	r = pre_transfer();
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	r = get_file();
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	// 退出
	r = execute_cmd(QUIT);
	if (r)
	{
		sock_cmd_->s_close();
		return r;
	}
	
	return NO_ERROR;
}

/*
int main()
{
	FtpClient* ftpclient = new FtpClient();
	// ftpclient->upload("127.0.0.1", 21, "misery", "880622", "/home/misery/", "/home/misery/MyProject/MyCpp", "main.cpp");
	ftpclient->download("127.0.0.1", 21, "misery", "880622", "/home/misery/MyProject/MyCpp", "/home/misery/", "main.cpp");
	return 0;
}
*/

