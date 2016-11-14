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

void priv_sock_send_cmd(int fd, int cmd);//服务进程(发送)-->>nobody
int priv_sock_get_cmd(int fd);//服务进程-->>nobody(接收)

void priv_sock_send_result(int fd, char res);//nobody(发送)-->>服务进程
char priv_sock_get_result(int fd);//nobody-->>服务进程(接收)

void priv_sock_send_int(int fd, int the_int);//发送一个整数
int priv_sock_get_int(int fd);//接收一个整数

void priv_sock_send_buf(int fd, const char* buf, unsigned int len);//发送一个字符串
void priv_sock_recv_buf(int fd, char* buf, unsigned int len);//接收一个字符串

void priv_sock_send_fd(int sock_fd, int fd);//发送文件描述符
int priv_sock_recv_fd(int sock_fd);//接收文件描述符



#endif //_PRIV_SOCK_H_
