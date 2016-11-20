#ifndef _SESSION_H_
#define _SESSION_H_
#include "common.h"

typedef struct session {	
	//控制连接
	uid_t uid;//当前用户ID
	int ctrl_fd;//已接入的用户套接字
	char cmdline[MAX_COMMAND_LINE];//命令端口接到的命令行
	char cmd[MAX_COMMAND];//存命令
	char arg[MAX_ARG];//存命令的参数
	//数据连接
	struct sockaddr_in* port_addr;//主动模式时存客户数据端口地址
	int pasv_listen_fd;//被动模式监听数据链接的监听套接字
	int data_fd;//数据链接套接字
	//父子进程通信
	int parent_fd;//nobody进程使用的通信口
	int child_fd;//子进程使用的通信口
	//FTP协议状态
	int is_ascii;//目前的数据传输格式
	long long restart_pos;//用于断点续传
	char* rnfr_name;//需要重命名的文件的原名称
} session_t;

void begin_session(session_t* psess);

#endif/*_SESSION_H_*/
