#ifndef _SYS_UTIL_H_//系统工具模块
#define _SYS_UTIL_H_

#include "common.h"

int tcp_server(const char* host, unsigned short port);//启动服务器(创建套接字并绑定,监听)

int getlocalip(char* ip);//获取本机IP地址

void activate_nonblock(int fd);//设置为非阻塞模式
void deactivate_nonblock(int fd);//设置为阻塞模式

int read_timeout(int fd, unsigned int wait_seconds);//读超时函数
int write_timeout(int fd, unsigned int wait_seconds);//写超时函数
int accept_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds);//接受超时
int connect_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds);//连接超时

ssize_t readn(int fd, void* buf, size_t count);
ssize_t writen(int fd, const void* buf, size_t count);
ssize_t recv_peek(int sockfd, void* buf, size_t len);//只读不移除套接口数据
ssize_t readline(int sockfd, void* buf, size_t maxline);//读到'\n'时返回

void send_fd(int sock_fd, int fd);//发送文件描述符
int recv_fd(int sock_fd, int fd);//接收文件描述符


#endif/*_SYS_UTIL_H_*/
