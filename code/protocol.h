#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define FTP_ROOT_DIR  "/home/gec/Tcp_Service"
#define TRAN_PATH "/home/gec/Tcp_Service"
#define DEST_PATH "/home/gec/receve"
#define MAX_CAPACITY 4096
#define MAX 4096*2+2


/*
		�������ʽ:
		 
		(1) "��ͷ" ....... "��β"
		 
			0xc0 ....... 0xc0
		������в�������� 0xc0,
			���ȷʵ��0xc0 ת��:
					0xc0  -> 0xdd 0xdc
					0xdd  -> 0xdd 0xdb
					
			�յ������ʱ��
			
				0xc0 ...... 0xc0
					
					0xdd 0xdc -> 0xc0
					0xdd 0xdb -> 0xdd


�������ʽ :
	��ͷ(0xc0)  
		�������������(4bytes,��С��ģʽ���)
		�����(1bytes)
		��������(1bytes)
		����1����(1bytes)
		����1������
		����2����(1bytes)
		����2������
		...
		����n�ĳ���(1bytes)
		����n�ĳ���
	��β(0xc0)



*/

enum cmd_no
{
	FTP_CMD_LS = 1,
	FTP_CMD_GET,
	FTP_CMD_PUT,
	FTP_CMD_BYE,
};

enum err_no
{
	FTP_ERR_NOERROR = 0,// ok, success
	FTP_ERR_DIR, //1��ʾxxx����,Ŀ¼��صĴ���
	FTP_ERR_NOEXIST, //��ʾ�ļ�������
	FTP_ERR_FILE,  //��ʾ�ļ�����ʧ��
};


#endif
