#ifndef __GENERAL_H__
#define __GENERAL_H__


/*
	read_check:��һ���ļ��������ж�����
	@fd:�ļ�������
	@buf:��������ȥ
	@len:�����ٸ��ֽ�
	����ֵ:
		 >0 ����ʵ�ʶ�����ʵ����
		 =0 ��ʾ�ļ�����(�����ӶϿ�)
		 <0 ��ʾ������
	����:
		
*/
extern int read_check(int fd, char *buf, int len);


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
extern int write_check(int fd, char *buf, int len);




#endif
