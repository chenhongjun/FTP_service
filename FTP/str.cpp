/*************************************************************************
	> File Name: str.cpp
	> Author: Willow
	> Mail: 1769003060@qq.com 
	> Created Time: 2016年10月25日 星期二 17时50分38秒
 ************************************************************************/
#include "str.h"
#include "common.h"

void str_trim_crlf(char* str)//删除字符串末尾的回车与换行"\r\n"
{
	char* p = str+strlen(str)-1;
	while (*p == '\r' || *p == '\n')
		*p-- = '\0';
}

void str_split(const char* str, char* left, char* right, char c)//分割字符串为命令加参数
{
	const char* p = strchr(str, c);
	if (p == NULL)//str中无c字符
	{
		strcpy(left, str);
	}
	else
	{
		strncpy(left, str, p-str);
		strcpy(right, p+1);
	}
}

int str_all_space(const char* str)//判断字符串是不是全为空白字符(不可显示字符/控制字符)
{
	while (*str)
	{
		if (!isspace(*str++))
			return 0;
	}
	return 1;
}
void str_upper(char* str)
{
	while (*str)
	{
		*str++ = toupper(*str);
	}
}
long long str_to_longlong(const char* str)//<stdlib>:atoll
{
	//return atoll(str);
	if (str == NULL || strlen(str) > 15)
		return 0;
	long long result = 0;
	long long mult = 1;
	const char* ch = str+strlen(str)-1;
	while (ch >= str)
	{
		if (*ch > '9' || *ch < '0')
			return 0;
		result += (*ch-- - '0') * mult;
		mult *= 10;
	}
	return result;
}
unsigned int str_octal_to_uint(const char* str)//8进str(0711)--->>>10进unsigned int
{
	if (str == NULL || strlen(str) > 12)
		return 0;
	const char* ch = str;
	if (*ch == '0')
		++ch;
	unsigned int result = 0;
	while (*ch)
	{
		if (*ch > '7' || *ch < '0')
			return 0;
		result <<= 3;
		result += *ch++ - '0';
	}
	return result;
}
