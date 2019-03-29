#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define FTP_ROOT_DIR  "/home/gec/Tcp_Service"
#define TRAN_PATH "/home/gec/Tcp_Service"
#define DEST_PATH "/home/gec/receve"
#define MAX_CAPACITY 4096
#define MAX 4096*2+2


/*
		命令包格式:
		 
		(1) "包头" ....... "包尾"
		 
			0xc0 ....... 0xc0
		命令包中不允许出现 0xc0,
			如果确实有0xc0 转义:
					0xc0  -> 0xdd 0xdc
					0xdd  -> 0xdd 0xdb
					
			收到命令的时候
			
				0xc0 ...... 0xc0
					
					0xdd 0xdc -> 0xc0
					0xdd 0xdb -> 0xdd


命令包格式 :
	包头(0xc0)  
		整个命令包长度(4bytes,以小端模式存放)
		命令号(1bytes)
		参数个数(1bytes)
		参数1长度(1bytes)
		参数1的内容
		参数2长度(1bytes)
		参数2的内容
		...
		参数n的长度(1bytes)
		参数n的长度
	包尾(0xc0)



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
	FTP_ERR_DIR, //1表示xxx错误,目录相关的错误
	FTP_ERR_NOEXIST, //表示文件不存在
	FTP_ERR_FILE,  //表示文件创建失败
};


#endif
