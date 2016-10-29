/*************************************************************************
	> File Name: main.cpp
	> Author: Willow
	> Mail: 1769003060@qq.com 
	> Created Time: 2016年10月24日 星期一 21时30分21秒
 ************************************************************************/

#include "../common.h"
#include "../sysutil.h"
#include<iostream>

using namespace std;

int main()
{
	int conn_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (conn_fd == -1)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &servaddr.sin_addr);
	servaddr.sin_port = htons(21);
	if (connect(conn_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		ERR_EXIT("connect");
	
	char recvbuf[128] = {0};
	
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

	return 0;
}
