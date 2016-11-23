#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "ftpcodes.h"
#include "tunable.h"
#include "privsock.h"
void ftp_reply(int ctrl_fd, int status, const char* text);//回应函数
void ftp_lreply(int ctrl_fd, int status, const char* text);//回应函数
int port_active(session_t* psess);//主动模式是否被激活
int pasv_active(session_t* psess);//被动模式被激活
int get_transfer_fd(session_t* psess);//获得data_fd
int get_port_fd(session_t* psess);//向父进程接收套接字
int get_pasv_fd(session_t* psess);//
const char* statbuf_get_perms(struct stat* sbuf);//文件列表访问属性
const char* statbuf_get_date(struct stat* sbuf);//文件列表时间属性


int list_common(session_t* psess, int detail);//文件列表

struct ftpcmd_t {
	const char* cmd;
	void (*cmd_handler)(session_t* psess);
};

static void do_user(session_t* psess)//cmd = USER
{
	struct passwd *pw = getpwnam(psess->arg);//获取用户信息
	if (pw == NULL)//用户不存在
	{
		ftp_reply(psess->ctrl_fd, FTP_LOGINERR, "Login incorrect.");
		exit(0);
	}
	
	psess->uid = pw->pw_uid;//用户存在
	ftp_reply(psess->ctrl_fd, FTP_GIVEPWORD, "Please specify the password.");
}
static void do_pass(session_t* psess)//cmd = PASS
{
	struct passwd *pw = getpwuid(psess->uid);//获取用户信息
	if (pw == NULL)
	{
		ftp_reply(psess->ctrl_fd, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	
	struct spwd* sp = getspnam(pw->pw_name);//获取用户的影子文件信息
	if (sp == NULL)
	{
		ftp_reply(psess->ctrl_fd, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	
	//明文密码加密后,与影子文件中的密码对比相不相等
	char* encrypted_pass = crypt(psess->arg, sp->sp_pwdp);	
	if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)//如果密码错误
	{
		ftp_reply(psess->ctrl_fd, FTP_LOGINERR, "Login incorrect.");
		exit(0);
	}
	
	umask(tunable_local_umask);//设置掩码
	setegid(pw->pw_gid);//登录成功修改当前用户和用户组
	seteuid(pw->pw_uid);
	chdir(pw->pw_dir);//修改进程路径
	ftp_reply(psess->ctrl_fd, FTP_LOGINOK, "Login successful.");//登录成功
}
static void do_cwd(session_t* psess)//改变当前路径
{
	if (chdir(psess->arg) < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Failed to change directory.");
		return;
	}
	ftp_reply(psess->ctrl_fd, FTP_CWDOK, "Directory successfully changed.");
}
static void do_cdup(session_t* psess)//返回上一级目录
{
	if (chdir("..") < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Failed to change directory.");
		return;
	}
	ftp_reply(psess->ctrl_fd, FTP_CWDOK, "Directory successfully changed.");
}
static void do_quit(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_port(session_t* psess)
{
	//PORT 1,50,190,100,0,21
	unsigned int v[6];
	sscanf(psess->arg, "%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);
	psess->port_addr = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	bzero(psess->port_addr, sizeof(struct sockaddr));
	psess->port_addr->sin_family = AF_INET;
	unsigned char* p = (unsigned char*) psess->port_addr->sin_port;
	p[0] = v[0];//大端(网络)字节序存法存入psess->port_addr
	p[1] = v[1];
	p = (unsigned char*)psess->port_addr->sin_addr.s_addr;
	p[0] = v[2];
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];
	
	ftp_reply(psess->ctrl_fd, FTP_PORTOK, "PORT command successful. Consider using PASV.");
	
}
static void do_pasv(session_t* psess)
{
	char ip[16] = {0};
	//getlocalip(ip);

	/*psess->pasv_listen_fd = tcp_server(ip, 20);//创建套接口20
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getsockname(psess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen) < 0)//获得新套接字信息
	{
		ERR_EXIT("getsockname");
	}*/

	priv_sock_send_cmd(psess->child_fd, PRIV_SOCK_PASV_LISTEN);
	unsigned short port = (unsigned short)priv_sock_get_int(psess->child_fd);
	cout << "port=" << port << endl;
	
	unsigned int v[4];//ip地址存入v
	sscanf("0.0.0.0", "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char text[1024] = {0};
	sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).", v[0], v[1], v[2], v[3], port>>8, port&0xFF);
	cout << text << endl;
	ftp_reply(psess->ctrl_fd, FTP_PASVOK, text);//通知客户端自己已经准备好
	//free(psess->port_addr);psess->port_addr = -1;
}
static void do_type(session_t* psess)
{
	if (strcmp(psess->arg, "A") == 0)//要求以ascii码传输
	{
		psess->is_ascii = 1;
		ftp_reply(psess->ctrl_fd, FTP_TYPEOK, "Switching to ASCII mode.");		
	}
	else if (strcmp(psess->arg, "I") == 0)//要求以二进制传输
	{
		psess->is_ascii = 0;
		ftp_reply(psess->ctrl_fd, FTP_TYPEOK, "Switching to Binary mode.");
	}
	else
	{
		ftp_reply(psess->ctrl_fd, FTP_BADCMD, "Unrecognised TYPE command.");
	}
}
static void do_stru(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_mode(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_retr(session_t* psess)
{
	//下载文件
	//断点续传
	if (get_transfer_fd(psess) == 0)//获得数据传输通道
	{
		return;
	}
	
	int fd = open(psess->arg, O_RDONLY);//打开文件
	if (fd == -1)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Failed to open file.");
		return;
	}
	int ret;
	//加读锁
	ret = lock_file_read(fd);
	if (ret == -1)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Failed to open file.");
		return;
	}
	//判断文件是否为普通文件
	struct stat sbuf;
	ret = fstat(fd, &sbuf);
	if (!S_ISREG(sbuf.st_mode))
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Failed to open file.");
		return;
	}
	//150应答
	char text[4096] = {0};
	if (psess->is_ascii)//ascii模式传输
	{
		sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes).", psess->arg, (long long)sbuf.st_size);
	}
	else//二进制模式传输
	{
		sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes).", psess->arg, (long long)sbuf.st_size);
	}

	ftp_reply(psess->ctrl_fd, FTP_DATACONN, text);

	//下载文件
	bzero(&text, 4096);
	int flag;
	while (1)
	{
		ret = read(fd, text, sizeof(text));
		if (ret == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				flag = 1;
				break;
			}
		}
		else if (ret == 0)
		{
			flag = 0;
			break;
		}
		if (writen(psess->data_fd, text, ret) != ret)
		{
			flag = 2;
			break;
		}
	}
	close(psess->data_fd);
	psess->data_fd = -1;
	if (flag == 0)
	{
		//传输完成226
		ftp_reply(psess->ctrl_fd, FTP_TRANSFEROK, "Transfer complete.");
	}
	else if (flag == 1)
	{//426
		ftp_reply(psess->ctrl_fd, FTP_BADSENDFILE, "Failure reading from local file.");
	}
	else if (flag == 2)
	{//451
		ftp_reply(psess->ctrl_fd, FTP_BADSENDNET, "Failure writting to network stream.");
	}


}
static void do_stor(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_appe(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_list(session_t* psess)
{
	//根据之前的主被动模式 创建数据链接
	if (!get_transfer_fd(psess))
		return;
	//150
	ftp_reply(psess->ctrl_fd, FTP_DATACONN, "Here comes the directory listing.");
	cout << "begin list" << endl;
	//传输列表
	list_common(psess, 1);//详细清单
	//关闭数据套接字
	close(psess->data_fd);
	psess->data_fd = -1;
	//226
	ftp_reply(psess->ctrl_fd, FTP_TRANSFEROK, "Directory send OK.");
}
static void do_nlst(session_t* psess)
{
	if (get_transfer_fd(psess) == 0)
		return;
	ftp_reply(psess->ctrl_fd, FTP_DATACONN, "Here comes the directory listing.");
	list_common(psess, 0);//简略清单
	close(psess->data_fd);
	psess->data_fd = -1;
	ftp_reply(psess->ctrl_fd, FTP_TRANSFEROK, "Directory send OK.");
}
static void do_rest(session_t* psess)//保存断点位置
{
	psess->restart_pos = str_to_longlong(psess->arg);//断点位置
	char text[1024] = {0};
	sprintf(text, "Restart position accepted (%lld).", psess->restart_pos);
	ftp_reply(psess->ctrl_fd, FTP_RESTOK, text);
}
static void do_abor(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_pwd(session_t* psess)
{
	char dir[1024+1] = {0};
	getcwd(dir, 1024);
	char text[1024+3] = {0};
	sprintf(text, "\"%s\"", dir);
	ftp_reply(psess->ctrl_fd, FTP_PWDOK, text);
}
static void do_mkd(session_t* psess)
{
	if (mkdir(psess->arg, 0777) < 0)//创建目录(文件夹),权限为0777&umask
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Create directory operation failed.");
		return;
	}
	char text[4096+1] = {0};//返回文本内容
	if (psess->arg[0] == '/')//绝对路径
	{
		sprintf(text, "%s created.", psess->arg);
	}
	else//相对路径
	{
		char dir[4096+1] = {0};
		getcwd(dir, 4097);//获得当前路径
		if (dir[strlen(dir)-1] == '/')//有后缀
		{
			sprintf(text, "%s%s created.", dir, psess->arg);
		}
		else//无后缀
		{
			sprintf(text, "%s/%s created.", dir, psess->arg);
		}
	}
	ftp_reply(psess->ctrl_fd, FTP_MKDIROK, text);
}
static void do_rmd(session_t* psess)//删除路径(文件夹)
{
	if (rmdir(psess->arg) < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Remove directory operation failed.");
		return;
	}
	ftp_reply(psess->ctrl_fd, FTP_RMDIROK, "Remove directory operation successful.");
}
static void do_dele(session_t* psess)//删除文件
{
	if (unlink(psess->arg) < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Delete operation failed.");
		return;
	}
	ftp_reply(psess->ctrl_fd, FTP_DELEOK, "Delete operation successful.");
}
static void do_rnfr(session_t* psess)//接收需要重命名的文件的原文件名
{
	psess->rnfr_name = (char*)malloc(strlen(psess->arg) + 1);
	memset(psess->rnfr_name, 0, strlen(psess->arg) + 1);
	strcpy(psess->rnfr_name, psess->arg);
	ftp_reply(psess->ctrl_fd, FTP_RNFROK, "Ready for RNTO.");
}
static void do_rnto(session_t* psess)//接收需要重命名的文件的新名称
{
	if (psess->rnfr_name == NULL)
	{
		ftp_reply(psess->ctrl_fd, FTP_NEEDRNFR, "RNFR required first.");
		return;
	}
	if (rename(psess->rnfr_name, psess->arg) < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Rename fail.");
		free(psess->rnfr_name);
		psess->rnfr_name = NULL;
		return;
	}
	ftp_reply(psess->ctrl_fd, FTP_RENAMEOK, "Rename successful.");
	free(psess->rnfr_name);
	psess->rnfr_name = NULL;
}
static void do_size(session_t* psess)//查看文件大小(不支持文件夹大小的查看)
{
	struct stat buf;
	if (stat(psess->arg, &buf) < 0)
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "SIZE operation failed.");
		return;
	}
	if (!S_ISREG(buf.st_mode))
	{
		ftp_reply(psess->ctrl_fd, FTP_FILEFAIL, "Could not get file size.");
		return;
	}
	char text[1024] = {0};
	sprintf(text, "%lld", (long long)buf.st_size);
	ftp_reply(psess->ctrl_fd, FTP_SIZEOK, text);
}
static void do_syst(session_t* psess)
{
	ftp_reply(psess->ctrl_fd, FTP_SYSTOK, "UNIX Type : L8");
}
static void do_feat(session_t* psess)
{
	ftp_lreply(psess->ctrl_fd, FTP_FEAT, "Features:");
	writen(psess->ctrl_fd, "EPRT\r\n", strlen("EPRT\r\n"));
	writen(psess->ctrl_fd, "EPSV\r\n", strlen("EPSV\r\n"));
	writen(psess->ctrl_fd, "MDTM\r\n", strlen("MDTM\r\n"));
	writen(psess->ctrl_fd, "PASV\r\n", strlen("PASV\r\n"));
	writen(psess->ctrl_fd, "REST STREAM\r\n", strlen("REST STREAM\r\n"));
	writen(psess->ctrl_fd, "SIZE\r\n", strlen("SIZE\r\n"));
	writen(psess->ctrl_fd, "TVFS\r\n", strlen("TVFS\r\n"));
	writen(psess->ctrl_fd, "UTF-8\r\n", strlen("UTF-8\r\n"));
	ftp_reply(psess->ctrl_fd, FTP_FEAT, "End");
}
static void do_site(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_stat(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_noop(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_help(session_t* psess)
{
	cout << "OO" << endl;
}

ftpcmd_t ctrl_cmds[] = {
	/*访问控制命令*/
	{"USER", &do_user},
	{"PASS", &do_pass},
	{"CWD", &do_cwd},
	{"XCWD", &do_cwd},
	{"CDUP", &do_cdup},
	{"XCUP", &do_cdup},
	{"QUIT", &do_quit},
	{"ACCT", NULL},
	{"SMNT", NULL},
	{"REIN", NULL},
/*传输参数命令*/
	{"PORT", &do_port},
	{"PASV", &do_pasv},
	{"TYPE", &do_type},
	{"STRU", &do_stru},
	{"MODE", &do_mode},
/*服务命令*/
	{"RETR", &do_retr},
	{"STOR", &do_stor},
	{"APPE", &do_appe},
	{"LIST", &do_list},
	{"NLST", &do_nlst},
	{"REST", &do_rest},
	{"ABOR", &do_abor},
	{"\377\364\377\362ABOR", &do_abor},
	{"PWD", &do_pwd},
	{"XPWD", &do_pwd},
	{"MKD", &do_mkd},
	{"XMKD", &do_mkd},
	{"RMD", &do_rmd},
	{"XRMD", &do_rmd},
	{"DELE", &do_dele},
	{"RNFR", &do_rnfr},
	{"RNTO", &do_rnto},
	{"SITE", &do_site},
	{"SYST", &do_syst},
	{"FEAT", &do_feat},
	{"SIZE", &do_size},
	{"STAT", &do_stat},
	{"NOOP", &do_noop},
	{"HELP", &do_help},
	{"STOU", NULL},
	{"ALLO", NULL}
};


void handle_child(session_t* psess)
{
	ftp_reply(psess->ctrl_fd, FTP_GREET, "(miniftp 0.1)");//连接(connect)成功响应
	int ret;
	while (1)
	{
		bzero(psess->cmdline, MAX_COMMAND_LINE);
		bzero(psess->cmd, MAX_COMMAND);
		bzero(psess->arg, MAX_ARG);
		ret = readline(psess->ctrl_fd, psess->cmdline, MAX_COMMAND_LINE);
		if (ret == -1)
		{
			ERR_EXIT("readline");
		}
		else if (ret == 0)
		{
			exit(EXIT_SUCCESS);//对方断开连接,服务进程退出
		}
		//去除\r\n
		str_trim_crlf(psess->cmdline);
		//cout << "cmd text:" << psess->cmdline << endl;
		//解析FTP命令与参数
		str_split(psess->cmdline, psess->cmd, psess->arg, ' ');
		str_upper(psess->cmd);//命令改为统一大写
		//cout << psess->cmd << " : " << psess->arg << endl;
		//处理FTP命令与参数
		/*if (strcmp(psess->cmd, "USER") == 0)
		{
			do_user(psess);
		}
		else if (strcmp(psess->cmd, "PASS") == 0)
		{
			do_pass(psess);
		}*/
		int size = sizeof(ctrl_cmds) / sizeof(ftpcmd_t);
		int i = 0;
		for (i = 0; i < size; ++i)
		{
			if (strcmp(psess->cmd, ctrl_cmds[i].cmd) == 0)
			{
				if (ctrl_cmds[i].cmd_handler != NULL)
					ctrl_cmds[i].cmd_handler(psess);
				else //无对应的处理函数,处理函数为NULL
					ftp_reply(psess->ctrl_fd, FTP_COMMANDNOTIMPL, "Unimplement command.");
				break;
			}
		}
		if (i == size)//找不到命令
			ftp_reply(psess->ctrl_fd, FTP_BADCMD, "Unknown command.");
	}

}

void ftp_reply(int ctrl_fd, int status, const char* text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d %s\r\n", status, text);
	writen(ctrl_fd, buf, strlen(buf));
}
void ftp_lreply(int ctrl_fd, int status, const char* text)//回应函数
{
	char buf[1024] = {0};
	sprintf(buf, "%d-%s\r\n", status, text);
	writen(ctrl_fd, buf, strlen(buf));
}


int list_common(session_t* psess, int detail)
{
	DIR* dir = opendir(".");
	if (dir == NULL)
		return 0;
	struct dirent *dt;
	struct stat sbuf;
	while ((dt = readdir(dir)) != NULL)
	{
		if (lstat(dt->d_name, &sbuf) < 0)//文件本身信息,如果是软链接文件也是该软链接文件的信息
			continue;
		
		if (dt->d_name[0] == '.')//过滤隐藏文件
			continue;
		
		char buf[1024] = {0};
		int off = 0;
		if (detail)//长清单list
		{
		const char* perms = statbuf_get_perms(&sbuf);

		off = sprintf(buf, "%s ", perms);
		off += sprintf(buf+off, "%3d %-8d %-8d ", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);//硬链接数,所有者ID,所属组ID
		off += sprintf(buf+off, "%8lu ", sbuf.st_size);//文件大小
		

		const char* datebuf = statbuf_get_date(&sbuf);
		off += sprintf(buf+off, "%s ", datebuf);

		
		if (S_ISLNK(sbuf.st_mode))//是符号(软)链接文件
		{
			char tmp[1024+1] = {0};
			readlink(dt->d_name, tmp, sizeof(tmp));
			off += sprintf(buf+off, "%s -> %s\r\n", dt->d_name, tmp);
		}
		else 	
			off += sprintf(buf+off, "%s\r\n", dt->d_name);//非符号链接文件名
		}
		else//短清单
		{
			sprintf(buf+off, "%s", dt->d_name);
		}
		
		//cout << buf << endl;
		writen(psess->data_fd, buf, strlen(buf));
	}
	closedir(dir);
	return 1;
}

int port_active(session_t* psess)//主动模式是否被激活
{
	if (psess->port_addr)
	{
		if (pasv_active(psess))
		{
			cerr << "both port an pasv are active.";
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}
//两个都开就是死循环
int pasv_active(session_t* psess)//被动模式被激活
{
	/*
	if (psess->pasv_listen_fd != -1)
	{
		if (port_active(psess))
		{
			cerr << "botn port an pasv are active.";
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	//*/
	priv_sock_send_cmd(psess->child_fd, PRIV_SOCK_PASV_ACTIVE);
	int active = priv_sock_get_int(psess->child_fd);
	if (active)
	{
		if (port_active(psess))
		{
				cerr << "both port an pasv are active.";
				exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}

int get_port_fd(session_t* psess)//向父进程接收套接字存入psess->data_fd
{
		priv_sock_send_cmd(psess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
		unsigned short port = ntohs(psess->port_addr->sin_port);
		char* ip = inet_ntoa(psess->port_addr->sin_addr);
		priv_sock_send_int(psess->child_fd, (int)port);
		char buf[16] = {0};
		strcpy(buf, ip);
		priv_sock_send_buf(psess->child_fd, buf, strlen(buf));
		int ret = priv_sock_get_result(psess->child_fd);
		if (ret == PRIV_SOCK_RESULT_BAD)
			return 0;
		else if (ret == PRIV_SOCK_RESULT_OK)
		{
			psess->data_fd = priv_sock_recv_fd(psess->child_fd);
		}
		return 1;
}
int get_pasv_fd(session_t* psess)//被动模式创建数据链接
{
	priv_sock_send_cmd(psess->child_fd, PRIV_SOCK_PASV_ACCEPT);
	int res = priv_sock_get_result(psess->child_fd);
	if (res == PRIV_SOCK_RESULT_BAD)
		return 0;
	else if (res == PRIV_SOCK_RESULT_OK)
	{
		psess->data_fd = priv_sock_recv_fd(psess->child_fd);
	}
		return 1;
}

int get_transfer_fd(session_t* psess)
{
	int ret = 1;
	//检测是否收到PORT或者PASV命令
	if (!port_active(psess) && !pasv_active(psess))
	{
		ftp_reply(psess->ctrl_fd, FTP_BADSENDCONN, "Use PORT or PASV first.");
		return 0;
	}

	if (port_active(psess))
	{//主动模式,服务器主动去连接客户端
		/*int fd = tcp_client(0);//20端口
		if (connect_timeout(fd, psess->port_addr, tunable_connect_timeout) < 0)
		{
			close(fd);
			return 0;
		}
		psess->data_fd = fd;*/
		ret = get_port_fd(psess);
	}
	else if (pasv_active(psess))
	{
		/*int fd = accept_timeout(psess->pasv_listen_fd, NULL, tunable_accept_timeout);
		close(psess->pasv_listen_fd);
		psess->pasv_listen_fd = -1;
		if (fd == -1)
		{
			return 0;
		}
		psess->data_fd = fd;*/
		ret = get_pasv_fd(psess);
		cout << "fd = " << psess->data_fd << endl;
	}

	if (psess->port_addr)
	{
		free(psess->port_addr);
		psess->port_addr = NULL;
	}
	return ret;
}

const char* statbuf_get_perms(struct stat* sbuf)//文件列表属性
{
		static char perms[] = "----------";
		perms[0] = '?';//文件类型
		mode_t mode = sbuf->st_mode;
		switch (mode & S_IFMT)
		{
			case S_IFREG:
				perms[0] = '-';
				break;
			case S_IFDIR:
				perms[0] = 'd';
				break;
			case S_IFLNK:
				perms[0] = 'l';
				break;
			case S_IFIFO:
				perms[0] = 'p';
				break;
			case S_IFSOCK:
				perms[0] = 's';
				break;
			case S_IFCHR:
				perms[0] = 'c';
				break;
			case S_IFBLK:
				perms[0] = 'b';
				break;
			
		}

		if (mode & S_IRUSR)//用户访问权限
			perms[1] = 'r';
		if (mode & S_IWUSR)
			perms[2] = 'w';
		if (mode & S_IXUSR)
			perms[3] = 'x';

		if (mode & S_IRGRP)
			perms[4] = 'r';
		if (mode & S_IWGRP)
			perms[5] = 'w';
		if (mode & S_IXGRP)
			perms[6] = 'x';
		
		if (mode & S_IROTH)
			perms[7] = 'r';
		if (mode & S_IWOTH)
			perms[8] = 'w';
		if (mode & S_IXOTH)
			perms[9] = 'x';
		
		if (mode & S_ISUID)//特殊权限位
			perms[3] = (perms[3] == 'x' ? 's' : 'S');
		if (mode & S_ISGID)
			perms[6] = (perms[6] == 'x' ? 's' : 'S');
		if (mode & S_ISVTX)
			perms[9] = (perms[9] == 'x' ? 't' : 'T');
		
		return perms;
}
const char* statbuf_get_date(struct stat* sbuf)//文件列表时间属性
{
		static char datebuf[64] = {0};
		const char* p_date_format = "%b %e %H:%M";
		struct timeval tv;
		gettimeofday(&tv, NULL);//获得系统当前时间
		time_t local_time = tv.tv_sec;
		if (sbuf->st_mtime > local_time || (local_time - sbuf->st_mtime) > 60*60*24*182)//创建时间大于系统当前时间或创建时间超过半年
		{
			p_date_format = "%b %e %Y";
		}
		struct tm* p_tm = localtime(&sbuf->st_mtime);
		strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
		return datebuf;
}
