/*************************************************************************
	> File Name: ftp.cpp
	> Author: Willow
	> Mail: 1769003060@qq.com 
	> Created Time: 2016年10月30日 星期日 20时47分57秒
 ************************************************************************/


#include "../common.h"
#include "../sysutil.h"
#include <iostream>
#include <sys/epoll.h>

using namespace std;

int to_connect_ser(const char* addr, short port);
void do_read_send(int conn);
void do_recv(int conn);

int main()
{
	int conn_fd = to_connect_ser("127.0.0.1", 21);
	
	int ep_fd = epoll_create(100);
	struct epoll_event ev[3];
	ev[0].events = EPOLLIN;
	ev[0].data.fd = 0;
	epoll_ctl(ep_fd,EPOLL_CTL_ADD, 0, &ev[0]);

	ev[1].events = EPOLLIN;
	ev[1].data.fd = conn_fd;
	epoll_ctl(ep_fd, EPOLL_CTL_ADD, conn_fd, &ev[1]);
	while (1)
	{
		int ret = epoll_wait(ep_fd, ev, 3, -1);
		for (int i = 0; i < ret; ++i)
		{
			int fd = ev[i].data.fd;
			if ((fd == 0) && (ev[i].events & EPOLLIN))
				do_read_send(conn_fd);
			else if ((fd == conn_fd) && (ev[i].events & EPOLLIN))
				do_recv(conn_fd);
		}
	}

	/*char recvbuf[128] = {0};
	
	readline(conn_fd, recvbuf, sizeof(recvbuf));
	cout << recvbuf << endl;
	
	while (1)
	{
		memset(recvbuf, 0, sizeof(recvbuf));
		cout << "输入需要发送的信息:" << endl;
		gets(recvbuf);

		cout << recvbuf << endl;
		recvbuf[strlen(recvbuf)] = '\r';
		recvbuf[strlen(recvbuf)] = '\n';

		writen(conn_fd, recvbuf, strlen(recvbuf));
	
		memset(recvbuf, 0, sizeof(recvbuf));
		readline(conn_fd, recvbuf, sizeof(recvbuf));
		cout << "收到服务器回复为:" << endl;
		cout << recvbuf << endl;
	}
	*/
	return 0;
}

int to_connect_ser(const char* addr, short port)//链接服务器
{
	int conn_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (conn_fd == -1)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_aton(addr, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);
	if (connect(conn_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		ERR_EXIT("connect");
	return conn_fd;
}

void do_read_send(int conn)
{
	char buf[1024] = {0};
	gets(buf);
	buf[strlen(buf)] = '\0';
	buf[strlen(buf)] = '\r';
	buf[strlen(buf)] = '\n';
	writen(conn, buf, strlen(buf));
}

void do_recv(int conn)
{
	char buf[1024] = {0};
	readline(conn, buf, sizeof(buf));
	cout << "收到数据:" << endl;
	cout << buf << endl;
}
