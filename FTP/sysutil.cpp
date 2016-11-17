#include "sysutil.h"

int tcp_client(unsigned short port)
{
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("tcp_client");
	
	if (port > 0)
	{
		int on = 1;
		if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))) < 0)
			ERR_EXIT("setsockopt");
	
		struct sockaddr_in localaddr;
		bzero(&localaddr, sizeof(localaddr));
		localaddr.sin_family = AF_INET;
		localaddr.sin_port = htons(port);
		char ip[16] = {0};
		getlocalip(ip);
		inet_aton(ip, &localaddr.sin_addr);
		if (bind(sock, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0)
			ERR_EXIT("bind");
	}
	return sock;
}



/*启动tcp服务器
 * host:服务器ip地址或主机名
 * port:服务器端口号
 * 成功则返回监听套接字
 */
int tcp_server(const char* host, unsigned short port)
{
	int listenfd;
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ERR_EXIT("socket");

	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));
	
	servaddr.sin_family = AF_INET;
	
	if (host != NULL)//地址不为空
	{
		if (inet_aton(host, &servaddr.sin_addr)/*host为IP地址*/ == 0)
		{//==0 则host不为IP而为主机名
			struct hostent *hp;
			if ((hp = gethostbyname(host)) == NULL)
				ERR_EXIT("gethostbyname");
			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
		}
	}
	else//host为空则绑定本主机所有IP地址
	{
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	servaddr.sin_port = htons(port);
	
	//inet_aton("127.0.0.1", &servaddr.sin_addr);

	int on = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) < 0)//设置套接字重复利用
		ERR_EXIT("setsockopt");
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
	{
		cerr << listenfd << endl;
		cerr << ntohs(servaddr.sin_port) << endl;
		cerr << inet_ntoa(servaddr.sin_addr) << endl;
		ERR_EXIT("bind");
	}
	if (listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");
	return listenfd;
}

int getlocalip(char* ip)//获得本机IP地址
{
	char host[100] = {0};
	if (gethostname(host, sizeof(host)) < 0)
		return -1;
	struct hostent *hp;
	if ((hp = gethostbyname(host)) == NULL)
		return -1;
	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
	return 0;
}

void activate_nonblock(int fd)//设置为非阻塞模式
{
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		ERR_EXIT("fcntl");
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		ERR_EXIT("fcntl");
}
void deactivate_nonblock(int fd)//设置为阻塞模式
{
	int ret;
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		ERR_EXIT("fcntl");
	flags ^= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		ERR_EXIT("fcntl");
}

int read_timeout(int fd, unsigned int wait_seconds)//读超时函数
{
	int ret = 0;
	if (wait_seconds > 0)
	{
		fd_set read_fdset;
		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			FD_ZERO(&read_fdset);
			FD_SET(fd, &read_fdset);
			ret = select(fd+1, &read_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else if (ret == 1)
			ret = 0;
	}
	return ret;
}
int write_timeout(int fd, unsigned int wait_seconds)//写超时函数
{
	int ret = 0;

	if (wait_seconds > 0)
	{
		fd_set write_fdset;
		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			FD_ZERO(&write_fdset);
			FD_SET(fd, &write_fdset);
			ret = select(fd+1, NULL, &write_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);

		if (ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else if (ret == 1)
			ret = 0;
	}
	return ret;
}
int accept_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds)//接受超时
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	
	if (wait_seconds > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		bzero(&timeout, sizeof(timeout));
		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do 
		{
			ret = select(fd+1, &accept_fdset, NULL, NULL, &timeout);
		} while ((ret < 0) && (errno == EINTR));
		if (ret == -1)
			return -1;
		else if (ret == 0)
		{
			errno = ETIMEDOUT;
			return -1;
		}
	}
	
	if (addr != NULL)
		ret = accept(fd, (struct sockaddr*)addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);
	if (ret == -1)
		ERR_EXIT("accept");
	return ret;
}

int connect_timeout(int fd, struct sockaddr_in* addr, unsigned int wait_seconds)//连接超时
{
	int ret = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if (wait_seconds > 0)
		activate_nonblock(fd);
	ret = connect(fd, (struct sockaddr*)addr, addrlen);
	if (ret < 0 && errno == EINPROGRESS)
	{
		fd_set connect_fdset;
		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			FD_ZERO(&connect_fdset);
			FD_SET(fd, &connect_fdset);
			ret = select(fd+1, NULL, &connect_fdset, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0)
		{
			errno = ETIMEDOUT;
			ret = -1;
		}
		else if (ret == 1)
		{//套接口错误或者连接成功
			int err;
			socklen_t socklen = sizeof(err);
			int sockptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);//获得fd的错误码
			if (sockptret  == -1)
				return -1;
			if (err == 0)
				ret = 0;
			else
			{
				errno = err;
				ret = -1;
			}
		}
		else if (ret < 0)
			return -1;
	}
	if (wait_seconds > 0)
		deactivate_nonblock(fd);
	return ret;
}

ssize_t readn(int fd, void* buf, size_t count)
{
	size_t nleft = count;
	ssize_t nread;
	char* bufp = (char*)buf;
	while (nleft > 0)
	{
		if ((nread = read(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (nread == 0)
			return 0;
		
		bufp += nread;
		nleft -= nread;
	}
	return count;
}
ssize_t writen(int fd, const void* buf, size_t count)
{
	size_t nleft = count;
	ssize_t nwritten;
	const char* bufp = (const char*)buf;
	while (nleft > 0)
	{
		if ((nwritten = write(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR)
				continue;
			return -1;
		}
		else if (nwritten == 0)
			continue;
		bufp += nwritten;
		nleft -= nwritten;
	}
	return count;
}
ssize_t recv_peek(int sockfd, void* buf, size_t len)//recv函数只能用于网络套接字
{
	while (1)
	{
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if (ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}
ssize_t readline(int sockfd, void* buf, size_t maxline)//只能用于套接口
{
	int ret;
	int nread;
	char* bufp = (char*)buf;
	int nleft = maxline;
	while (1)
	{
		ret = recv_peek(sockfd, bufp, nleft);
		if (ret < 0)//出错失败
			return ret;
		else if (ret == 0)//对方关闭了套接口
			return ret;
		nread = ret;
		for (int i = 0; i < nread; ++i)
		{
			if (bufp[i] == '\n')
			{
				ret = readn(sockfd, bufp, i+1);
				bufp[ret] = '\0';
				if (ret != i+1)
					exit(EXIT_FAILURE);
				return ret;
			}
		}
		if (nread > nleft)
			exit(EXIT_FAILURE);
		nleft -= nread;
		ret = readn(sockfd, bufp, nread);//真正的读数据
		if (ret != nread)
			exit(EXIT_FAILURE);
		bufp += nread;
	}
	return -1;
}

void send_fd(int sock_fd, int fd)//发送文件描述符
{
	int ret;
	struct msghdr msg;
	struct cmsghdr* p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(fd))];
	int* p_fds;
	char sendchar = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	p_fds = (int*)CMSG_DATA(p_cmsg);
	*p_fds = fd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);
	ret = sendmsg(sock_fd, &msg, 0);
	if (ret == -1)
		ERR_EXIT("sendmsg");
}

int recv_fd(int sock_fd)//接收文件描述符
{
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr* p_cmsg;
	int* p_fd;
	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	p_fd = (int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;
	ret = recvmsg(sock_fd, &msg, 0);
	if (ret == -1)
		ERR_EXIT("recvmsg");

	p_cmsg = CMSG_FIRSTHDR(&msg);
	if (p_cmsg == NULL)
		ERR_EXIT("no passed fd");

	p_fd = (int*)CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if (recv_fd == -1)
		ERR_EXIT("no passed fd");

	return recv_fd;
}
