#include "privsock.h"

void priv_sock_init(session_t* psess)
{
	int sockfds[2] = {-1, -1};
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) == -1)//父子进程pipe通信
	{
		ERR_EXIT("socketpair");
	}
	psess->parent_fd = sockfds[0];
	psess->child_fd = sockfds[1];
}
void priv_sock_close(session_t* psess)
{
	if (psess->child_fd != -1)
	{
		close(psess->child_fd);
		psess->child_fd = -1;
	}

	if (psess->parent_fd != -1)
	{
		close(psess->parent_fd);
		psess->parent_fd = -1;
	}
}
void priv_sock_set_parent_context(session_t* psess)
{
	if (psess->child_fd != -1)
	{
		close(psess->child_fd);
		psess->child_fd = -1;
	}

}
void priv_sock_set_child_context(session_t* psess)
{
	if (psess->parent_fd != -1)
	{
		close(psess->parent_fd);
		psess->parent_fd = -1;
	}
}
