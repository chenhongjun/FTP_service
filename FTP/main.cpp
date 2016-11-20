
#include "tunable.h"
#include "sysutil.h"
#include "parseconf.h"
#include "str.h"
#include "session.h"
#include "ftpproto.h"

int main()
{
	//配置文件模块测试代码
	/*parseconf_load_file(MINIFTP_CONF);
	cout << tunable_pasv_enable << endl;
	cout << tunable_port_enable << endl;

	cout << tunable_listen_port << endl;
	cout << tunable_max_clients << endl;
	cout << tunable_max_per_ip << endl;
	cout << tunable_accept_timeout << endl;
	cout << tunable_connect_timeout << endl;
	cout << tunable_idle_session_timeout << endl;
	cout << tunable_data_connection_timeout << endl;
	cout << tunable_local_umask << endl;
	cout << tunable_upload_max_rate << endl;
	cout << tunable_download_max_rate << endl;
	cout << tunable_listen_address << endl;
	//*/
	//list_common();
	if (getuid() != 0)//只能由root用户启动
	{
		fprintf(stderr, "miniftpd: must be started as root\n");
		exit(EXIT_FAILURE);
	}
	
	session_t sess = {//会话结构体，用于通信
		//控制连接
		-1, -1, "", "", "",
		//数据连接
		NULL, -1, -1,
		//父子进程通信
		-1, -1,
		//FTP协议状态
		0, 0, NULL
	};

	signal(SIGCHLD, SIG_IGN);//忽略僵尸进程状态的回收
	
	int listenfd = tcp_server(NULL, 21);
	int conn;
	pid_t pid;
	while (1)
	{
		if ((conn = accept_timeout(listenfd, NULL, 0)) == -1)//永久阻塞等待接收连接
			ERR_EXIT("accept_timeout::select");
		pid = fork();
		if (pid == -1)
			ERR_EXIT("fork");
		if (pid == 0)
		{//子进程处理新用户
			close(listenfd);
			sess.ctrl_fd = conn;
			begin_session(&sess);
		}
		else if (pid > 0)
		{//父进程继续等待新连接
			close(conn);
		}
	}


	return 0;
}
