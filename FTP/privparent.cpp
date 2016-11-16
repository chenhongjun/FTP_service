#include "privparent.h"
#include "privsock.h"
#include "sysutil.h"
#include "tunable.h"
static void privop_pasv_get_data_sock(session_t* psess);
static void privop_pasv_active(session_t* psess);
static void privop_pasv_listen(session_t* psess);
static void privop_pasv_accept(session_t* psess);
static int capset(struct __user_cap_header_struct*, struct __user_cap_data_struct*);//增加用户权限
static void minimize_privilege();//对用户权限的修改封装使之权限最小化

void handle_parent(session_t* psess)
{
	//minimize_privilege();//修改用户为nobody及权限最小化
	
	/*int ff = socket(AF_INET, SOCK_STREAM, 0);
	if (ff > 0)
	{
		struct sockaddr_in ad;
		bzero(&ad, sizeof(ad));
		ad.sin_family = AF_INET;
		ad.sin_port = htons(20);
		inet_pton(AF_INET, "127.0.0.1", &ad.sin_addr);
		if (bind(ff, (struct sockaddr*)&ad, sizeof(ad)) == -1)
			cerr << "我去" << endl;
	}
	else cout << "fddddd" << endl;
	cout << "OK" << endl;
	//*/
	int cmd;
	while (1)
	{
		//read(psess->parent_fd, &cmd, sizeof(cmd));
		cmd = priv_sock_get_cmd(psess->parent_fd);
		//解析和处理命令
		switch (cmd)
		{
			case PRIV_SOCK_GET_DATA_SOCK://想要数据链接通道
				privop_pasv_get_data_sock(psess);
				break;
			case PRIV_SOCK_PASV_ACTIVE://想要知道是否为被动模式
				privop_pasv_active(psess);
				break;
			case PRIV_SOCK_PASV_LISTEN://想要被动监听
				privop_pasv_listen(psess);
				break;
			case PRIV_SOCK_PASV_ACCEPT://想要被动接收
				privop_pasv_accept(psess);
				break;
		}
	}
}

static void privop_pasv_get_data_sock(session_t* psess)
{
	//接收命令,接收整数port,接收字符串ip
	unsigned short port = (unsigned short)priv_sock_get_int(psess->parent_fd);
	char ip[16] = {0};
	priv_sock_recv_buf(psess->parent_fd, ip, sizeof(ip));
	//bind(20);
	struct sockaddr_in addr;
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	//connect_client
	int fd = tcp_client(20);
	if (fd == -1)
	{
		priv_sock_send_result(psess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	if (connect_timeout(fd, psess->port_addr, tunable_connect_timeout) < 0)
	{
		close(fd);
		priv_sock_send_result(psess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	priv_sock_send_result(psess->parent_fd, PRIV_SOCK_RESULT_OK);
	//send_fd_to_son
	priv_sock_send_fd(psess->parent_fd, fd);
	close(fd);
}

static void privop_pasv_active(session_t* psess)
{
	int active;
	if (psess->pasv_listen_fd != -1)
		active = 1;
	else
		active = 0;
	priv_sock_send_int(psess->parent_fd, active);
}
static void privop_pasv_listen(session_t* psess)
{
	char ip[16] = {0};
	getlocalip(ip);
	psess->pasv_listen_fd = tcp_server(NULL, 20);//在本地创建套接口20并监听
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	bzero(&addr, addrlen);
	
	if (getsockname(psess->pasv_listen_fd, (struct sockaddr*)&addr, &addrlen) < 0)//获得新套接字信息
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(addr.sin_port);//端口号存入port
	priv_sock_send_int(psess->parent_fd, (int)port);//发送给子进程，由子进程通知客户端链接过来
	
}
static void privop_pasv_accept(session_t* psess)
{
	int fd = accept_timeout(psess->pasv_listen_fd, NULL, tunable_accept_timeout);
	close(psess->pasv_listen_fd);
	psess->pasv_listen_fd = -1;
	if (fd == -1)
	{
		priv_sock_send_result(psess->parent_fd, PRIV_SOCK_RESULT_BAD);
		cout << "send result is bad." << endl;
		return;
	}
	priv_sock_send_result(psess->parent_fd, PRIV_SOCK_RESULT_OK);
	
	priv_sock_send_fd(psess->parent_fd, fd);
	close(fd);
}

static void minimize_privilege()//对用户权限的修改封装使之权限最小化
{
	
	struct passwd* pw = getpwnam("nobody");//获得nobody用户的信息
	if (pw == NULL)
		return;
	if (setegid(pw->pw_gid) < 0)//修改程序的用户和用户组
		ERR_EXIT("setegid");
	if (setuid(pw->pw_uid) < 0)
		ERR_EXIT("setuid");
	
	struct __user_cap_header_struct cap_header;//增加用户权限预备工作
	struct __user_cap_data_struct cap_data;
	memset(&cap_header, 0, sizeof(cap_header));
	memset(&cap_data, 0, sizeof(cap_data));
	
	cap_header.version = _LINUX_CAPABILITY_VERSION_1;//32位系统
	cap_header.pid = 0;

	__u32 cap_mask = 0;
	cap_mask |= (1 << CAP_NET_BIND_SERVICE);//绑定<1024端口号的特权
	cap_data.effective = cap_data.permitted = cap_mask;
	cap_data.inheritable = 0;//exec时不继承

	if (capset(&cap_header, &cap_data) == -1)//增加用户权限,头文件中没有定义该接口,所以通过syscall函数调用系统调用实现该接口
		ERR_EXIT("syscall hdrp");
	
}
static int capset(struct __user_cap_header_struct* hdrp, struct __user_cap_data_struct* datap)//增加用户权限
{
	return syscall(__NR_capset, hdrp, datap);
}
