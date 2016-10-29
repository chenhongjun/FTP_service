#include "session.h"
#include "privparent.h"
#include "ftpproto.h"

void begin_session(session_t* psess)
{
	int sockfds[2] = {-1, -1};
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) == -1)//父子进程pipe通信
	{
		ERR_EXIT("socketpair");
	}
	

	pid_t pid = fork();
	if (pid == -1)
		ERR_EXIT("fork");
	if (pid == 0)
	{//服务进程
		close(sockfds[0]);
		psess->child_fd = sockfds[1];
		handle_child(psess);
	}
	else if(pid > 0)
	{//nobody进程
		struct passwd* pw = getpwnam("nobody");//获得nobody用户的信息
		if (pw == NULL)
			return;
		if (setegid(pw->pw_gid) < 0)//修改程序的用户和用户组
			ERR_EXIT("setegid");
		if (setuid(pw->pw_uid) < 0)
			ERR_EXIT("setuid");
	
		close(sockfds[1]);
		psess->parent_fd = sockfds[0];
		handle_parent(psess);
	}
}
