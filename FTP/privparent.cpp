#include "privparent.h"

static void privop_pasv_get_data_sock(session_t* psess);
static void privop_pasv_active(session_t* psess);
static void privop_pasv_listen(session_t* psess);
static void privop_pasv_accept(session_t* psess);

void handle_parent(session_t* psess)
{
	struct passwd* pw = getpwnam("nobody");//获得nobody用户的信息
	if (pw == NULL)
		return;
	if (setegid(pw->pw_gid) < 0)//修改程序的用户和用户组
		ERR_EXIT("setegid");
	if (setuid(pw->pw_uid) < 0)
		ERR_EXIT("setuid");

	char cmd;
	while (1)
	{
		//read(psess->parent_fd, &cmd, sizeof(cmd));
		cmd = priv_sock_get_cmd(psess->parent_fd);
		//解析和处理命令
		switch (cmd)
		{
			case PRIV_SOCK_GET_DATA_SOCK:
				break;
			case PRIV_SOCK_PASV_ACTIVE:
				break;
			case PRIV_SOCK_PASV_LISTEN:
				break;
			case PRIV_SOCK_PASV_ACCEPT:
				break;
		}
	}
}

static void privop_pasv_get_data_sock(session_t* psess)
{
	//接收命令,接收整数port,接收字符串ip
	//bind(20);
	//connect_client
	//send_fd
}
static void privop_pasv_active(session_t* psess)
{}
static void privop_pasv_listen(session_t* psess)
{}
static void privop_pasv_accept(session_t* psess)
{}
