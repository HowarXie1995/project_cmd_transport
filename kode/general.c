#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

//外部变量声明
extern int terminate;


/*
 *	read_check:从一个文件描述符中读数据
 *	@fd：文件描述符
 *	@buf：读到哪里去
 *	@len:读多少个字节
 *	返回值：
 *			> 0 返回实际读到的实际数
 *			= 0	表示文件结束
 *			< 0 表示文件出错
 */

int read_check(int fd, char *buf, int len)
{
	int rbytes = 0;//已经读到的字节数字
	int r = 0;
	int cur_bytes ;//一次要读的字节数
	
	while (rbytes < len && !terminate)
	{
		//设置非阻塞
		//select 标准流程
		int maxfd = fd + 1;

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd);

		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		r = select(maxfd, &rfds, NULL, NULL, &timeout);
		if (r <= 0)
		{
			usleep(100000);
			continue;
		}
		if(FD_ISSET(fd, &rfds))
		{
			cur_bytes = (len - rbytes) > 1024 ? 1024 : (len - rbytes);
			//读操作
			r = read(fd, buf + rbytes, cur_bytes);
			//写入文件操作
			if ( r > 0 )
			{
				rbytes += r;
			}else
			{
				break;
			}
		}
	}

	return rbytes;

}


/*
 *	read_check:从一个文件描述符中写数据
 *	@fd：文件描述符
 *	@buf：写到哪里去
 *	@len:写多少个字节
 *	返回值：
 *			> 0 返回实际写到的实际数
 *			= 0	表示文件结束
 *			< 0 表示文件出错
 */

int write_check(int fd, char *buf, int len)
{
	int rbytes = 0;//已经读到的字节数字
	int r = 0;
	int cur_bytes ;//一次要读的字节数
	
	while (rbytes < len && !terminate)
	{
		//设置非阻塞
		//select 标准流程
		int maxfd = fd + 1;

		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(fd);

		struct timeval timeout;
		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		r = select(maxfd, NULL, &rfds, NULL, &timeout);
		if (r <= 0)
		{
			usleep(100000);
			continue;
		}
		if(FD_ISSET(fd, &rfds))
		{
			cur_bytes = (len - rbytes) > 1024 ? 1024 : (len - rbytes);
			//写操作
			r = write(fd, buf + rbytes, cur_bytes);
			//写入文件操作
			if ( r > 0 )
			{
				rbytes += r;
			}else
			{
				break;
			}
		}
	}

	return rbytes;

}

