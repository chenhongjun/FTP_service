#include "privparent.h"


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
		read(psess->parent_fd, &cmd, sizeof(cmd));
		//解析和处理命令
	}
}
