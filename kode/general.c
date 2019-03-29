#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

//�ⲿ��������
extern int terminate;


/*
 *	read_check:��һ���ļ��������ж�����
 *	@fd���ļ�������
 *	@buf����������ȥ
 *	@len:�����ٸ��ֽ�
 *	����ֵ��
 *			> 0 ����ʵ�ʶ�����ʵ����
 *			= 0	��ʾ�ļ�����
 *			< 0 ��ʾ�ļ�����
 */

int read_check(int fd, char *buf, int len)
{
	int rbytes = 0;//�Ѿ��������ֽ�����
	int r = 0;
	int cur_bytes ;//һ��Ҫ�����ֽ���
	
	while (rbytes < len && !terminate)
	{
		//���÷�����
		//select ��׼����
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
			//������
			r = read(fd, buf + rbytes, cur_bytes);
			//д���ļ�����
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
 *	read_check:��һ���ļ���������д����
 *	@fd���ļ�������
 *	@buf��д������ȥ
 *	@len:д���ٸ��ֽ�
 *	����ֵ��
 *			> 0 ����ʵ��д����ʵ����
 *			= 0	��ʾ�ļ�����
 *			< 0 ��ʾ�ļ�����
 */

int write_check(int fd, char *buf, int len)
{
	int rbytes = 0;//�Ѿ��������ֽ�����
	int r = 0;
	int cur_bytes ;//һ��Ҫ�����ֽ���
	
	while (rbytes < len && !terminate)
	{
		//���÷�����
		//select ��׼����
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
			//д����
			r = write(fd, buf + rbytes, cur_bytes);
			//д���ļ�����
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

