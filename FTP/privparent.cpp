#include "privparent.h"


void handle_parent(session_t* psess)
{
	char cmd;
	while (1)
	{
		read(psess->parent_fd, &cmd, sizeof(cmd));
		//解析和处理命令
	}
}
