#include "privsock.h"
#include "sysutil.h"

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


void priv_sock_send_cmd(int fd, int cmd)
{
	int ret = writen(fd, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
	{
		fprintf(stderr, "priv_sock_send_cmd\n");
		exit(EXIT_FAILURE);
	}
}
int priv_sock_get_cmd(int fd)
{
	int res;
	int ret = readn(fd, &res, sizeof(res));
	if (ret == 0)
		exit(EXIT_SUCCESS);
	if (ret != sizeof(res))
	{
		fprintf(stderr, "priv_sock_get_cmd\n");
		exit(EXIT_FAILURE);
	}
	return res;
}
void priv_sock_send_result(int fd, char res)
{
	int ret = writen(fd, &res, sizeof(res));
	if (ret != sizeof(res))
	{
		fprintf(stderr, "priv_sock_send_result\n");
		exit(EXIT_FAILURE);
	}
}
char priv_sock_get_result(int fd)
{
	char cmd;
	int ret = readn(fd, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
	{
		fprintf(stderr, "priv_sock_get_result\n");
		exit(EXIT_FAILURE);
	}
	return cmd;
}

void priv_sock_send_int(int fd, int the_int)//发送一个整数
{
	int ret = writen(fd, &the_int, sizeof(the_int));
	if (ret != sizeof(the_int))
	{
		fprintf(stderr, "priv_sock_send_int");
		exit(EXIT_FAILURE);
	}
}

int priv_sock_get_int(int fd)//接收一个整数
{
	int the_int;
	int ret = readn(fd, &the_int, sizeof(the_int));
	if (ret != sizeof(the_int))
	{
		if (ret == 0)
		{
			cerr << "he close!" << endl;
			exit(EXIT_FAILURE);
		}
		fprintf(stderr, "priv_sock_get_int\n");
		exit(EXIT_FAILURE);
	}
	return the_int;
}
void priv_sock_send_buf(int fd, const char* buf, unsigned int len)//发送一个字符串
{
	priv_sock_send_int(fd, len);
	int ret = writen(fd, buf, len);
	if ((unsigned int)ret != len)
	{
		fprintf(stderr, "priv_sock_send_buf");
		exit(EXIT_FAILURE);
	}
}

void priv_sock_recv_buf(int fd, char* buf, unsigned int len)//接收一个字符串
{
	unsigned int recv_len = priv_sock_get_int(fd);
	if (recv_len >= len)
	{
		fprintf(stderr, "priv_sock_recv_buf");
		exit(EXIT_FAILURE);
	}
	unsigned int ret = readn(fd, buf, recv_len);
	if (ret != recv_len)
	{
		fprintf(stderr, "priv_sock_recv_buf");
		exit(EXIT_FAILURE);
	}
}

void priv_sock_send_fd(int sock_fd, int fd)//发送文件描述符
{
	send_fd(sock_fd, fd);
}

int priv_sock_recv_fd(int sock_fd)//接收文件描述符
{
	return recv_fd(sock_fd);
}
