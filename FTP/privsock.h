#ifndef _PRIV_SOCK_H_
#define _PRIV_SOCK_H_
#include "session.h"

//进程间通信命令
//FTP ser -->> nobody ctrl
#define PRIV_SOCK_GET_DATA_SOCK     1
#define PRIV_SOCK_PASV_ACTIVE       2
#define PRIV_SOCK_PASV_LISTEN       3
#define PRIV_SOCK_PASV_ACCEPT       4
//nobody ctrl -->> FTP ser
#define PRIV_SOCK_RESULT_OK         1
#define PRIV_SOCK_RESULT_BAD        2

void priv_sock_init(session_t* psess);//初始化内部进程间通信通道
void priv_sock_close(session_t* psess);//关闭进程间通信通道
void priv_sock_set_parent_context(session_t* psess);//设置父进程环境
void priv_sock_set_child_context(session_t* psess);//设置子进程环境





#endif //_PRIV_SOCK_H_
