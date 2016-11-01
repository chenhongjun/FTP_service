#include "session.h"
#include "privparent.h"
#include "privsock.h"
#include "ftpproto.h"

void begin_session(session_t* psess)
{
	
	priv_sock_init(psess);

	pid_t pid = fork();
	if (pid == -1)
		ERR_EXIT("fork");
	if (pid == 0)
	{//服务进程
		priv_sock_set_child_context(psess);
		handle_child(psess);
	}
	else if(pid > 0)
	{//nobody进程
		priv_sock_set_parent_context(psess);
		handle_parent(psess);
	}
}


