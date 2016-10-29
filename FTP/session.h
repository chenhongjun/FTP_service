#ifndef _SESSION_H_
#define _SESSION_H_
#include "common.h"

typedef struct session {	
	//控制连接
	uid_t uid;
	int ctrl_fd;//已接入的用户套接字
	char cmdline[MAX_COMMAND_LINE];
	char cmd[MAX_COMMAND];
	char arg[MAX_ARG];
	//父子进程通信
	int parent_fd;
	int child_fd;
	//FTP协议状态
	int is_ascii;
}session_t;

void begin_session(session_t* psess);

#endif/*_SESSION_H_*/