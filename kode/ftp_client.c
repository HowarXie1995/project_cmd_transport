#include <stdio.h>
#include <errno.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>


#include "protocol.h"

int terminate = 0;//进程退出标志，0表示不退出，1表示退出


int connect_tcp_server(char *servIP, short servPORT)
{
	//1.socket:创建一个套接字
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket error:");
		return -1;
	}


	//客户端也可以绑定。
	//绑定也好，不绑定也好，反正
	//是套接字，就一定会有一个地址(ip+port)
	//你不绑定，那么内核就会动态给你分配。

	#if 0
	struct sockaddr_in lo;
	memset(&lo, 0, sizeof(lo));
	lo.sin_family = AF_INET;
	lo.sin_port =  htons(  9999 );


	lo.sin_addr.s_addr = inet_addr("192.168.31.38"); //inet_addr("192.168.31.128");  (1)


	int r = bind(sock,	(struct sockaddr*)&lo, sizeof(lo));
	if (r == -1)
	{
		perror("bind error:");
		return -1;
	}
	#endif



	

	//2. connect：连接服务器
	struct sockaddr_in server;
	memset(&server,0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons( servPORT );
	server.sin_addr.s_addr = inet_addr(servIP);
	
	int   r =  connect(sock, (struct sockaddr*)&server, sizeof(server));
	if (r == -1)
	{
		perror("connect error:");
		return -1;
	}

	return sock;

}




void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGQUIT:
		case SIGINT:
			terminate = 1;
			break;
		default:
			break;
	}

}

int handle_Catalog(char *pathname, char *destname)
{
	//1）打开目录
	int er;					//用来查错
	DIR* dir;
	dir = opendir(pathname);
	if( NULL == dir )
	{
		perror("opendir error");
		return -1;
	}
	//严谨操作：判读是什么文件，目录文件出错？？？

	//2)解析目录名字
	struct dirent *dire;
	//char *path_buf[PATHNAME];
	while(dire = readdir(dir))
	{
		//跳过当前与上一级目录
		if(	!strcmp(dire->d_name, ".") ||
			!strcmp(dire->d_name, ".."))
		{
			continue;
		}
		//相等传回文件描述符
		if(!strcmp(dire->d_name, destname))
		{
			chdir(pathname);
			int fd = open(destname, O_RDONLY);
			printf("fd = %d\n", fd);
			if( -1 == fd )
			{
				perror("open error");
				closedir(dir);
				return -1;
			}
			closedir(dir);
			return fd;
		}
		
	}
	closedir(dir);
	return 0;
	
	
}


void read_ftp_resp(int sock, unsigned char *p, int *len)
{
	int i = 0;
	unsigned char ch = 0;
	int j = 0;
	// 0xc0 ..... 0xc0

	unsigned char cmd[MAX];
	
	//读到第一个 c0为止
	do
	{
		read(sock,&ch, 1);
		
	}while (ch != 0xc0 && !terminate);
	//printf("----%d------\n", __LINE__);


	while (ch == 0xc0)
	{
		read(sock, &ch, 1);
	}

	//此时 ch保存的是命令中的第一个字符
	
	//printf("----%d------\n", __LINE__);

	while (ch != 0xc0)
	{
	//	printf("1,i = %d\n", i);
		cmd[i++] = ch;		
		//printf("2,i = %d\n", i);
		read(sock, &ch,1);
	}

	//printf("----%d------\n", __LINE__);

	//去掉包头，包尾的完整的命令就保存在cmd[0] ~ cmd[i-1]
	//	0xdd 0xdc -> 0xc0
	//  0xdd 0xdb -> 0xdd

	int k = 0;
	while (j < i)
	{
		if (cmd[j] == 0xdd)
		{
			if (cmd[j+1] == 0xdc)
			{
				p[k++] = 0xc0;
			}
			else if (cmd[j+1] == 0xdb)
			{
				p[k++] = 0xdd;
			}
			
			j++;
		}
		else
		{
			p[k++] = cmd[j];
		}

		j++;
	}


	*len = k;
/*
	printf("recv:");
	for (j = 0; j < k; j++)
	{
		printf("%02x ", p[j]);
	}
	printf("\n");
	*/
}


/*
	send_cmd:
		cmd中不包括包头和包尾，同时里面
		可能含有需要转义的字符(0xc0, 0xdd)
		0xc0 -> 0xdd 0xdc
		0xdd -> 0xdd 0xdb
*/
void send_cmd_to_server(int sock, unsigned char cmd[], int len)
{
	unsigned char send_c[512];
	int i = 0;
	int j = 0;

	send_c[i++] = 0xc0; //包头

	while (j < len)
	{
		if (cmd[j] == 0xc0)
		{
			send_c[i++] = 0xdd;
			send_c[i++] = 0xdc;
		}
		else if (cmd[j] == 0xdd)
		{
			send_c[i++] = 0xdd;
			send_c[i++] = 0xdb;
		}
		else
		{
			send_c[i++] = cmd[j];
		}

		j++;
	}

	send_c[i++] = 0xc0;//包尾

	//only for test

	for (j = 0; j < i; j++)
	{
		printf("%02x ", send_c[j]);
	}
	printf("\n");


	send(sock, send_c, i, 0);
	
	
}

void send_resp_to_service(int sock, unsigned char resp[], int len)
{
	unsigned char *send_c = malloc(len * 2 + 2);
	int i = 0;
	int j = 0;
	puts(resp+4);
	send_c[i++] = 0xc0; //包头

	while (j < len)
	{
		if (resp[j] == 0xc0)
		{
			send_c[i++] = 0xdd;
			send_c[i++] = 0xdc;
		}
		else if (resp[j] == 0xdd)
		{
			send_c[i++] = 0xdd;
			send_c[i++] = 0xdb;
		}
		else
		{
			send_c[i++] = resp[j];
		}

		j++;
	}

	send_c[i++] = 0xc0;//包尾

	send(sock, send_c, i, 0);

	for (j = 0; j < i; j++)
	{
		printf("%02x ", send_c[j]);
	}
	printf("\n");

	free(send_c);
}


void ftp_cmd_ls(int sock, char *cmd)
{
	unsigned char send_cmd[512];
	int i = 0;
	
	//  长度字段(4) + 命令号(1) +参数个数(1) + 参数1(1) + 参数1实际长度 + .... +
	int packet_len = 4 + 1 + 1;
	

	//1.组成完整的命令包
	//send_cmd[i++]= 0xc0;


	//packet_len: 0x00000006
	//packet_len => send_cmd[1] send_cmd[2] send_cmd[3] send_cmd[4]
	//					0x06		0x00		0x00	 0x00
	//命令长度 ，小端模式存放
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;

	//命令号,1byte
	send_cmd[i++] = FTP_CMD_LS;

	//命令的参数个数，1byte
	send_cmd[i++] = 0;


	//send_cmd[i++] = 0xc0;


	//2.发送
	send_cmd_to_server(sock, send_cmd, i);
		
	printf("%s L_%d\n", __FUNCTION__, __LINE__);


	//等待ftp server的回复
	//读 服务器执行状态
	unsigned char resp_status[12];
	int len;
	read_ftp_resp(sock, resp_status, &len);
	if (len != 6)
	{
		printf("len != 6\n");
		return ;
	}

	if (resp_status[5] != FTP_ERR_NOERROR)
	{
		printf("ftp exec eror: %d\n", resp_status[5]);
		return ;
	}
	
	//读返回结果
	unsigned char resp[4096];
	read_ftp_resp(sock, resp, &len);
	if (len < 4096)
	{
		resp[len] = '\0';
		printf("%s\n", resp + 4);
	}

}


void ftp_cmd_get(int sock, char *cmd)
{
	
	unsigned char send_cmd[512];
	int i = 0;
	int sum_len = strlen(cmd);
	int len = 0;
	cmd = cmd + 4;
	while(cmd[0] == ' ')   //去掉空格
	{
		cmd++;
	}
	unsigned int contain_len = strlen(cmd) - 1;//文件内容大小 去掉最后的换行符
	
	//查错重点*******************************************************************
	//头部结构：  长度       命令号 参数个数 参数长度 参数内容
	int packet_len = 4 + 1 + 1 + 1 + contain_len;

	//解析长度
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;
	
	//命令号,1byte
	send_cmd[i++] = FTP_CMD_GET;

	//命令的参数个数，1byte
	send_cmd[i++] = 1;

	//命令参数长度
	send_cmd[i++] = contain_len;
	//命令内容
	strncpy(send_cmd + 7, cmd, contain_len);
	
	//查错代码*********************************************************
/*
	int j = 0;
	while(j < i + contain_len)
	{
		printf("send_cmd = %02x  \n", send_cmd[j++]);
	}
	*/
	//******************************************************************
	send_cmd_to_server(sock, send_cmd, i + contain_len);

	//等待ftp server的回复
	//读 服务器执行状态
	unsigned char resp_status[12];
	int len_get;
	unsigned char f_size[256] = {0};
	read_ftp_resp(sock, resp_status, &len_get);
	if (len_get != 6)
	{
		printf("len != 6\n");
		return ;
	}

	if (resp_status[5] != FTP_ERR_NOERROR)
	{
		printf("ftp exec eror: %d\n", resp_status[5]);
		return ;
	}
	
	//接收服务器发来的文件大小
	len_get = 0;
	read_ftp_resp(sock, f_size, &len_get);
	if(len_get != 4)
	{
		printf("ftp resp file size ERROR\n");
		return ;
	}
	// p[0] p[1] p[2] p[3]
	//小端模式，p[0]低字节 p[3] 高字节
	len = (f_size[0] & 0xff) |
		 ((f_size[1] & 0xff) << 8) |
		 ((f_size[2] & 0xff) << 16) |
		((f_size[3] & 0xff) <<  24);

	printf("len_get = %d\n", len_get);
	printf("len = %d\n", len);
	//接收服务器发来的文件内容

	char filename[256] = {0};
	cmd[contain_len] = '\0';
	sprintf(filename, "/home/gec/receve/%s", cmd);
	//printf("filename = %s\n", filename);
	int dest_src = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if( -1 == dest_src )
	{
		perror("open error");
		return ;
	}
	int real_size = 0;					//真实接收的字节数
	int sum_size = 0;					//总的接收到的字节数
	int size = 0;
	unsigned char p[MAX_CAPACITY] = {0};
	while(sum_size < len)
	{
		//接收
		read_ftp_resp(sock, p, &size);
		printf("*********len = %d\n", size);
	//	puts(p + 4);
		//这一帧数据的实际长度保存在 p[0] p[1] p[2] p[3], 小端模式存放
		 real_size    =   (p[0] & 0xff) | 
					((p[1] & 0xff) << 8) |
					((p[2] & 0xff) << 16) |
					((p[3] & 0xff) << 24);
		printf("*************real_size = %d\n", real_size);
		int realsize = write(dest_src, p + 4, real_size);
		if(realsize > 0)
		{
			sum_size += real_size;
		}
		
	}
	close(dest_src);
/*	while()
	{

	}*/

/*	//读返回结果
	unsigned char resp[4096];
	read_ftp_resp(sock, resp, &len);
	if (len_get < 4096)
	{
		resp[len_get] = '\0';
		printf("%s\n", resp + 4);
	}*/
	
}


void ftp_cmd_put(int sock, char * cmd)
{
	unsigned char send_cmd[512];
	int i = 0;
	int sum_len = strlen(cmd);
	int len = 0;
	cmd = cmd + 4;
	while(cmd[0] == ' ')   //去掉空格
	{
		cmd++;
	}
	unsigned int contain_len = strlen(cmd) - 1;//文件内容大小 去掉最后的换行符
	
	//查错重点*******************************************************************
	//头部结构：  长度		命令号 参数个数 参数长度 参数内容
	int packet_len = 4 + 1 + 1 + 1 + contain_len;

	//解析长度
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;
	
	//命令号,1byte
	send_cmd[i++] = FTP_CMD_PUT;

	//命令的参数个数，1byte
	send_cmd[i++] = 1;

	//命令参数长度
	send_cmd[i++] = contain_len;
	//命令内容
	strncpy(send_cmd + 7, cmd, contain_len);
	
	//查错代码*********************************************************
/*
	int j = 0;
	while(j < i + contain_len)
	{
		printf("send_cmd = %02x  \n", send_cmd[j++]);
	}
	*/
	//******************************************************************
	send_cmd_to_server(sock, send_cmd, i + contain_len);


	//等待ftp server的回复
	//读 服务器执行状态
	unsigned char resp_status[12];
	int len_get;
	unsigned char f_size[256] = {0};
	read_ftp_resp(sock, resp_status, &len_get);	
	if (len_get != 6)
	{
		printf("len != 6\n");
		return ;
	}

	if (resp_status[5] != FTP_ERR_NOERROR)
	{
		printf("ftp exec eror: %d\n", resp_status[5]);
		return ;
	}
	
	char filename[256] = {0};
	strncpy(filename, cmd, contain_len);
	int fd_src = 0;
	
	fd_src = handle_Catalog(DEST_PATH, filename);
	if(fd_src == 0)
	{
		printf("no find\n");
		return ;
	}else if(fd_src < 0)
	{
		return ;
	}
	printf("ready to send \n");
	
	//4)发送文件的大小
	struct stat st_buf;
	int fst = fstat(fd_src, &st_buf);
	if( -1 == fst )
	{
		perror("fstat error");
		return ;
	}
	int len_put = st_buf.st_size;
	printf("size = %d\n", len_put);
	unsigned char fi_size[256] = {0};
	i = 0;
	fi_size[i++] = st_buf.st_size & 0xff;
	fi_size[i++] = (st_buf.st_size >> 8 ) & 0xff;
	fi_size[i++] = (st_buf.st_size >> 16) & 0xff;
	fi_size[i++] = (st_buf.st_size >> 24) & 0xff;
	len = i;
	//4)
	send_resp_to_service(sock, fi_size, i);

	
	//5)打印文件
		int sum_size = 0;
		int real_size = 0;
		int size = 0;
		unsigned char p[MAX_CAPACITY] = {0} ;
		//发送
		while(sum_size < st_buf.st_size)
		{
			real_size = read(fd_src, p + 4, MAX_CAPACITY - 4);
			//puts(p + 4);
			if(real_size < 0)
			{
				perror("read file error");
				exit(0);
			}
			else if(real_size == 0)
			{
				printf("read null\n");
				continue;
			}
			else
			{
				p[0] = real_size & 0xff;
				p[1] = (real_size >> 8) & 0xff;
				p[2] = (real_size >> 16) & 0xff;
				p[3] = (real_size >> 24) & 0xff;
				//发送
				send_resp_to_service(sock, p, real_size + 4);
				sum_size += real_size;
			}
			printf("real_size = %d\n", real_size);
	
		}
		close(fd_src);
}

void ftp_cmd_bye(int sock, char * cmd)
{
	unsigned char send_cmd[512];
	int i = 0;
	
	//	长度字段(4) + 命令号(1) +参数个数(1) + 参数1(1) + 参数1实际长度 + .... +
	int packet_len = 4 + 1 + 1;
	

	//1.组成完整的命令包
	//send_cmd[i++]= 0xc0;


	//packet_len: 0x00000006
	//packet_len => send_cmd[1] send_cmd[2] send_cmd[3] send_cmd[4]
	//					0x06		0x00		0x00	 0x00
	//命令长度 ，小端模式存放
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;

	//命令号,1byte
	send_cmd[i++] = FTP_CMD_BYE;

	//命令的参数个数，1byte
	send_cmd[i++] = 0;


	//send_cmd[i++] = 0xc0;


	//2.发送
	send_cmd_to_server(sock, send_cmd, i);
		
	printf("%s L_%d\n", __FUNCTION__, __LINE__);


	//等待ftp server的回复
	//读 服务器执行状态
	unsigned char resp_status[12];
	int len;
	read_ftp_resp(sock, resp_status, &len);
	if (len != 6)
	{
		printf("len != 6\n");
		return ;
	}

	if (resp_status[5] != FTP_ERR_NOERROR)
	{
		printf("ftp exec eror: %d\n", resp_status[5]);
		return ;
	}
	close(sock);
	exit(0);
		

}

//./ftp_client serverIP serverPort
int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);

	int sock = connect_tcp_server(argv[1], atoi(argv[2]));
	if (sock == -1)
	{
		printf("failed to connect_tcp_server\n");
		return -1;
	}

	while (!terminate)
	{
		//输出命令提示符 ftp >
		char *ftp_indi = "ftp >";
		fwrite(ftp_indi, 1, strlen(ftp_indi), stdout);

		//1.从键盘接收用户的命令
		unsigned char cmd[512];
		fgets(cmd, 511, stdin);


		//2.把命令组成“命令包”格式发给服务器
		if (strncmp(cmd, "ls", 2) == 0)
		{
			//用户输入了ls这条命令

			//printf("%s L_%d\n", __FUNCTION__, __LINE__);

			ftp_cmd_ls(sock, cmd);
			printf("%s L_%d\n", __FUNCTION__, __LINE__);
			
		}
		else if (strncmp(cmd, "get", 3) == 0)
		{
			//用户输入了get这条命令
			ftp_cmd_get(sock, cmd);
		}
		else if (strncmp(cmd, "put", 3) == 0)
		{
			//用户输入了put这条命令
			ftp_cmd_put(sock, cmd);
		}
		else if (strncmp(cmd, "bye", 3) == 0)
		{
			//用户输入了bye这条命令
			ftp_cmd_bye(sock, cmd);
		}



	
	}

	//进程退出的清理工作
	close(sock);



}

