#ifndef _COMMON_H_//公共宏
#define _COMMON_H_

/*linux head file*/
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <shadow.h>
#include <dirent.h>
#include <signal.h>
//#include <crypt.h>

/*c stand head file*/
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/*c++ STL head file*/
#include <iostream>

using namespace std;

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}\
	while (0)

#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024
#define MINIFTP_CONF "./miniftpd.conf"

#endif/*_COMMON_H_*/
