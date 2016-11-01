#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "ftpcodes.h"
#include "tunable.h"

void ftp_reply(int ctrl_fd, int status, const char* text);//回应函数
void ftp_lreply(int ctrl_fd, int status, const char* text);//回应函数
int port_active(session_t* psess);//主动模式是否被激活
int pasv_active(session_t* psess);//被动模式被激活
int get_transfer_fd(session_t* psess);//获得data_fd

int list_common(session_t* psess);//文件列表

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
		return;
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
		return;
	}
	
	setegid(pw->pw_gid);//登录成功修改当前用户和用户组
	seteuid(pw->pw_uid);
	chdir(pw->pw_dir);//修改进程路径
	ftp_reply(psess->ctrl_fd, FTP_LOGINOK, "Login successful.");//登录成功
}
static void do_cwd(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_cdup(session_t* psess)
{
	cout << "OO" << endl;
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
	p[0] = v[0];//大端(网络)字节序存法
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
	getlocalip(ip);
	psess->pasv_listen_fd = tcp_server(ip, 0);//创建套接口20
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	if (getsockname(psess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen) < 0)//获得新套接字信息
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(addr.sin_port);//端口号存入port
	unsigned int v[4];//ip地址存入v
	sscanf(ip, "%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char text[1024] = {0};
	sprintf(text, "Entering Passive Mode (%u,%u,%u,%u,%u,%u).", v[0], v[1], v[2], v[3], port>>8, port&0xFF);
	ftp_reply(psess->ctrl_fd, FTP_PASVOK, text);
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
	cout << "OO" << endl;
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
	//创建数据链接
	if (get_transfer_fd(psess))
		return;
	//150
	ftp_reply(psess->ctrl_fd, FTP_DATACONN, "Here comes the directory listing.");
	
	//传输列表
	list_common(psess);
	//关闭数据套接字
	close(psess->data_fd);
	//226
	ftp_reply(psess->ctrl_fd, FTP_TRANSFEROK, "Directory send OK.");
}
static void do_nlst(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_rest(session_t* psess)
{
	cout << "OO" << endl;
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
	cout << "OO" << endl;
}
static void do_rmd(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_dele(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_rnfr(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_rnto(session_t* psess)
{
	cout << "OO" << endl;
}
static void do_site(session_t* psess)
{
	cout << "OO" << endl;
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
static void do_size(session_t* psess)
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
		cout << "cmd text:" << psess->cmdline << endl;
		//解析FTP命令与参数
		str_split(psess->cmdline, psess->cmd, psess->arg, ' ');
		str_upper(psess->cmd);//命令改为统一大写
		cout << psess->cmd << " : " << psess->arg << endl;
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


int list_common(session_t* psess)
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
		
		//if (dt->d_name[0] == '.')//过滤隐藏文件
		//	continue;
		
		char perms[] = "----------";
		perms[0] = '?';//文件类型
		mode_t mode = sbuf.st_mode;
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
		
		char buf[1024] = {0};
		int off = sprintf(buf, "%s ", perms);
		off += sprintf(buf+off, "%3d %-8d %-8d ", sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);//硬链接数,所有者ID,所属组ID
		off += sprintf(buf+off, "%8lu ", sbuf.st_size);//文件大小
		
		const char* p_date_format = "%b %e %H:%M";
		struct timeval tv;
		gettimeofday(&tv, NULL);//获得系统当前时间
		time_t local_time = tv.tv_sec;
		if (sbuf.st_mtime > local_time || (local_time - sbuf.st_mtime) > 60*60*24*182)//创建时间大于系统当前时间或创建时间超过半年
		{
			p_date_format = "%b %e %Y";
		}
		char datebuf[64] = {0};
		struct tm* p_tm = localtime(&sbuf.st_mtime);
		strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);
		off += sprintf(buf+off, "%s ", datebuf);

		
		if (S_ISLNK(mode))//是符号(软)链接文件
		{
			char tmp[1024+1] = {0};
			readlink(dt->d_name, tmp, sizeof(tmp));
			off += sprintf(buf+off, "%s -> %s\r\n", dt->d_name, tmp);
		}
		else 	
			off += sprintf(buf+off, "%s\r\n", dt->d_name);//非符号链接文件名
		
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
			cerr << "botn port an pasv are active.";
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}
//两个都开就是死循环
int pasv_active(session_t* psess)//被动模式被激活
{
	if (psess->pasv_listen_fd != -1)
	{
		if (port_active(psess))
		{
			cerr << "botn port an pasv are active.";
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	return 0;
}


int get_transfer_fd(session_t* psess)
{
	//检测是否收到PORT或者PASV命令
	if (!port_active(psess) && !pasv_active(psess))
	{
		ftp_reply(psess->ctrl_fd, FTP_BADSENDCONN, "Use PORT or PASV first.");
		return 0;
	}

	if (port_active(psess))
	{//主动模式,服务器主动去连接客户端
		int fd = tcp_client(0);//20端口
		if (connect_timeout(fd, psess->port_addr, tunable_connect_timeout) < 0)
		{
			close(fd);
			return 0;
		}
		psess->data_fd = fd;
		
	}
	else if (pasv_active(psess))
	{
		int fd = accept_timeout(psess->pasv_listen_fd, NULL, tunable_accept_timeout);
		close(psess->pasv_listen_fd);
		psess->pasv_listen_fd = -1;
		if (fd == -1)
		{
			return 0;
		}
		psess->data_fd = fd;
	}

	if (psess->port_addr)
	{
		free(psess->port_addr);
		psess->port_addr = NULL;
	}
	return 1;
}
