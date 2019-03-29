#ifndef __GENERAL_H__
#define __GENERAL_H__


/*
	read_check:从一个文件描述符中读数据
	@fd:文件描述符
	@buf:读到哪里去
	@len:读多少个字节
	返回值:
		 >0 返回实际读到的实际数
		 =0 表示文件结束(或连接断开)
		 <0 表示出错啦
	作者:
		
*/
extern int read_check(int fd, char *buf, int len);


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
extern int write_check(int fd, char *buf, int len);




#endif
