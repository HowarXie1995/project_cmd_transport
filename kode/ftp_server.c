#include <stdio.h>
#include <errno.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/time.h>
#include <fcntl.h>





#include "protocol.h"


int terminate = 0;//进程退出标志，0表示不退出，1表示退出


int create_tcp_listen_socket(char *ip, short port)
{
	//1 .创建一个套接字
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket error:");
		return -1;
	}

	//2. bind一个地址(对tcp server来说，是必须要的)

	/*
		struct sockaddr_in
		{
			sa_family_t sin_family; //协议族， AF_INET
			u_int16_t	sin_port; //端口号， 
			struct in_addr sin_addr; //ipv4地址
			char sin_zero[8] ;// padding, 填充用的，为了保持和其他
							 //协议族地址结构体大小一致!
		};

			struct in_addr
			{
				in_addr_t  s_addr; //ipv4地址是unsigned 32bits number
			};
			
			typedef u_int32_t in_addr_t;
	*/
	
	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port =  htons(  port );

	// "192.168.31.128" -> 
	//sa.sin_addr  => struct in_addr
	//sa.sin_addr.s_addr	=> in_addr_t (u_int32_t)
	sa.sin_addr.s_addr = inet_addr(ip); //inet_addr("192.168.31.128");  (1)
	//inet_aton(const char *cp, struct in_addr *inp);
	//inet_aton(argv[1], &sa.sin_addr); (2)

	int r = bind(sock,	(struct sockaddr*)&sa, sizeof(sa));
	if (r == -1)
	{
		perror("bind error:");
		return -1;
	}


	//3. listen:让socket进入一个“监听模式”
	r = listen(sock, 5);
	if (r == -1)
	{
		perror("listen error:");
		return -1;
	}

	return sock;

}



void sig_handler(int signo)
{
	switch(signo)
	{
		case SIGINT:
		case SIGQUIT:
			terminate= 1;
			break;
		default:
			break;
				
	}
	
}

/*
	send_resp_to_client:
		resp中不包括包头和包尾，同时里面
		可能含有需要转义的字符(0xc0, 0xdd)
		0xc0 -> 0xdd 0xdc
		0xdd -> 0xdd 0xdb
*/

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


void send_resp_to_client(int sock, unsigned char resp[], int len)
{
	unsigned char *send_c = malloc(len * 2 + 2);
	int i = 0;
	int j = 0;
//	puts(resp+4);
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
/*
	for (j = 0; j < i; j++)
	{
		printf("%02x ", send_c[j]);
	}
	printf("\n");
	*/
	free(send_c);
}



void read_ftp_cmd(int sock, unsigned char *p, int *len)
{
	int i = 0;
	unsigned char ch = 0;
	int j = 0;
	// 0xc0 ..... 0xc0

	unsigned char cmd[512];

	printf("%s L_%d\n", __FUNCTION__, __LINE__);
	//读到第一个 c0为止
	do
	{
		read(sock,&ch, 1);
		//printf("ch = %02x\n", ch);
	
	}while (ch != 0xc0 && !terminate);

	printf("%s L_%d\n", __FUNCTION__, __LINE__);

	while (ch == 0xc0)
	{
		read(sock, &ch, 1);
	}

	//此时 ch保存的是命令中的第一个字符
	

	while (ch != 0xc0)
	{
		cmd[i++] = ch;
		read(sock, &ch,1);
	}


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
}


/*
	反馈命令执行的状态(成功还是失败)
	0xc0（包头）
					回复包的长度(4bytes,小端模式存放)
					命令号(1byte,表示是对哪个命令的回复)
					执行状态(1bytes,
								0表示成功，
								其他值表示失败
							)
				
				0xc0(包尾)
*/
void resp_ftp_cmd_status(int sock, unsigned char cmdno, unsigned char error_code)
{
	unsigned char resp[512];
	int i = 0;

	//resp[i++] = 0xc0;

	//状态回复的包的长度:4 + 1 + 1 = 0x06
	resp[i++] = 0x06;
	resp[i++] = 0x00;
	resp[i++] = 0x00;
	resp[i++] = 0x00;

	//命令号
	resp[i++] = cmdno;

	//执行的状态
	resp[i++] = error_code;

	//包尾
	//resp[i++] = 0xc0;


	send_resp_to_client(sock, resp , i);

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
			chdir(TRAN_PATH);
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

void resp_ftp_cmd_ls(int sock, unsigned char *cmd)
{
	unsigned char error_code = FTP_ERR_NOERROR;
	unsigned char filenames[4096];
	int index = 0; //表示文件名列表的长度
	unsigned char *p = NULL;

	printf("%s L_%d\n", __FUNCTION__, __LINE__);

	DIR *dir = opendir(FTP_ROOT_DIR);
	if (dir == NULL)
	{
		error_code = FTP_ERR_DIR;
		goto resp_status;
	}

	printf("%s L_%d\n", __FUNCTION__, __LINE__);

	struct dirent *dirp = NULL;
	while (dirp = readdir(dir))
	{
		index += sprintf(filenames +index, "%s ", dirp->d_name);
	}
	closedir(dir);
	printf("%s L_%d\n", __FUNCTION__, __LINE__);



//回复命令执行的状态
resp_status:
	resp_ftp_cmd_status(sock, FTP_CMD_LS, error_code);

	if (error_code != FTP_ERR_NOERROR)
	{
		return ;
	}

	printf("%s L_%d\n", __FUNCTION__, __LINE__);
//回复ls命令的执行结果
resp_result:

	p = malloc(4 + index);

	// index -> p[0] p[1] p[2] p[3], 小端模式存放
	p[0] = index & 0xff;
	p[1] = (index >> 8) & 0xff;
	p[2] = (index >> 16) & 0xff;
	p[3] = (index >> 24) & 0xff;

	strcpy(p + 4, filenames);


	send_resp_to_client(sock, p, index + 4);

	free(p);
	
}

void resp_ftp_cmd_get(int sock, unsigned char *cmd)
{
	unsigned char error_code = FTP_ERR_NOERROR;
	unsigned char file_capacity[4096];
	int index = 0; //表示文件名列表的长度
	//1）解析文目录名字
	unsigned char *file_name = cmd + 7;
	unsigned int len = *(cmd + 6);
/*	//查错代码*********************************************************
	int j = 0;
	while(j < len)
	{
		printf("send_cmd = %02x  \n", file_name[j++]);
	}
	//******************************************************************
	*/
	int fd_src = 0;
	fd_src = handle_Catalog(TRAN_PATH, file_name);
	if (fd_src == 0)
	{
		error_code = FTP_ERR_NOEXIST;
		goto resp_status;
	}else if(fd_src < 0)
	{
		error_code = FTP_ERR_DIR;
		goto resp_status;
	}

	//2)回复命令执行的状态
resp_status:
	resp_ftp_cmd_status(sock, FTP_CMD_GET, error_code);

	if (error_code != FTP_ERR_NOERROR)
	{
		return ;
	}

	//4)发送文件的大小
	struct stat st_buf;
	int fst = fstat(fd_src, &st_buf);
	if( -1 == fst )
	{
		perror("fstat error");
		return ;
	}
	int fs_size = st_buf.st_size;
	printf("size = %d\n", fs_size);
	unsigned char f_size[256] = {0};
	int i = 0;
	f_size[i++] = st_buf.st_size & 0xff;
	f_size[i++] = (st_buf.st_size >> 8 ) & 0xff;
	f_size[i++] = (st_buf.st_size >> 16) & 0xff;
	f_size[i++] = (st_buf.st_size >> 24) & 0xff;
	len = i;
	//4)
	send_resp_to_client(sock, f_size, i);
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
			send_resp_to_client(sock, p, real_size + 4);
			sum_size += real_size;
		}
		printf("real_size = %d\n", real_size);

/*
		//这一帧数据的实际长度保存在 p[0] p[1] p[2] p[3], 小端模式存放
		 real_size	  =   (p[0] & 0xff) | 
					((p[1] & 0xff) << 8) |
					((p[2] & 0xff) << 16) |
					((p[3] & 0xff) << 24);
		write(dest_src, p + 4, real_size);
		sum_size += real_size;
		*/
	}
	close(fd_src);

	
}


void resp_ftp_cmd_put(int sock, unsigned char * cmd)
{
	unsigned char error_code = FTP_ERR_NOERROR;
	unsigned char file_capacity[4096];
	int index = 0; //表示文件名列表的长度
	//1）解析文件名字,并创建文件
	unsigned char *file_name = cmd + 7;
	unsigned int len = *(cmd + 6);
/*	//查错代码*********************************************************
	int j = 0;
	while(j < len)
	{
		printf("send_cmd = %02x  \n", file_name[j++]);
	}
	//******************************************************************
	*/		
	char filename[256] = {0};
	sprintf(filename, "/home/gec/Tcp_Service/%s", file_name);
	//printf("filename = %s\n", filename);
	int dest_src = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if( -1 == dest_src )
	{
		error_code = FTP_ERR_FILE;
		goto resp_status;
	}

	//2)回复命令执行的状态
resp_status:
	resp_ftp_cmd_status(sock, FTP_CMD_GET, error_code);

	if (error_code != FTP_ERR_NOERROR)
	{
		return ;
	}
	//接收客户端发来的文件大小
	int len_get = 0;
	char f_size[12] = {0};
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
		 real_size	  =   (p[0] & 0xff) | 
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


}

resp_ftp_cmd_bye(int sock, unsigned char * cmd)
{
	
	unsigned char error_code = FTP_ERR_NOERROR;
	unsigned char filenames[4096];
	int index = 0; //表示文件名列表的长度
	unsigned char *p = NULL;

	printf("%s L_%d\n", __FUNCTION__, __LINE__);

	DIR *dir = opendir(FTP_ROOT_DIR);
	if (dir == NULL)
	{
		error_code = FTP_ERR_DIR;
		goto resp_status;
	}

	printf("%s L_%d\n", __FUNCTION__, __LINE__);

	struct dirent *dirp = NULL;
	while (dirp = readdir(dir))
	{
		index += sprintf(filenames +index, "%s ", dirp->d_name);
	}
	closedir(dir);
	printf("%s L_%d\n", __FUNCTION__, __LINE__);



//回复命令执行的状态
resp_status:
	resp_ftp_cmd_status(sock, FTP_CMD_LS, error_code);

	if (error_code != FTP_ERR_NOERROR)
	{
		return ;
	}
	close(sock);
}

void handle_connection(int conn)
{
	unsigned char cmd[1024];
	int len;
	while (!terminate)
	{
		printf("%s L_%d\n", __FUNCTION__, __LINE__);
		//1.阻塞直到收到ftp_client命令包
		read_ftp_cmd(conn, cmd, &len);

		printf("%s L_%d\n", __FUNCTION__, __LINE__);
		//2. 解析这个命令包
		// cmd[0] cmd[1] cmd[2] cmd[3] => 命令包的长度
		int packet_len = (cmd[0] & 0xff) |
						 ((cmd[1] & 0xff) << 8) |
						 ((cmd[2] & 0xff) << 16) |
						((cmd[3] & 0xff) <<  24);
		if (packet_len != len)
		{
			printf("packet_len != len\n");
			continue;
		}
		printf("%s L_%d\n", __FUNCTION__, __LINE__);
		printf("cmd[4] = %d  %d\n",cmd[4], FTP_CMD_PUT);
		//3.根据不同的命令包，作相应的处理		
		if (cmd[4] == FTP_CMD_LS)
		{
			printf("%s L_%d\n", __FUNCTION__, __LINE__);
			resp_ftp_cmd_ls(conn, cmd);
		}
		if (cmd[4] == FTP_CMD_GET)
		{
			resp_ftp_cmd_get(conn, cmd);
		}
		if (cmd[4] == FTP_CMD_PUT)
		{
			resp_ftp_cmd_put(conn, cmd);
		}
		if (cmd[4] == FTP_CMD_BYE)
		{
			resp_ftp_cmd_bye(conn, cmd);
			break;
		}

	}


}



//./tcp_server ip port
int main(int argc, char *argv[])
{
	signal(SIGINT, sig_handler);
	signal(SIGQUIT,sig_handler);

	int sock = create_tcp_listen_socket(argv[1], atoi(argv[2]));
	if (sock == -1)
	{
		printf("failed to create_tcp_listen_socket\n");
		return -1;
	}


	// 接收客户端的连接请求

	while (!terminate)
	{
		int r = 0;
		struct sockaddr_in remote; //
		socklen_t len = sizeof(remote); //NOTE !!!! 
									//在调用前，保存地址结构体的长度，防止越界
		int maxfd = sock + 1;
		fd_set rfds;
		FD_ZERO(&rfds);
		
		FD_SET(sock, &rfds);
		
		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		
		r = select(maxfd, &rfds, NULL, NULL, &timeout);
		if (r == 0)
		{
			continue;
		}
		else if (r < 0)
		{
			perror("select error:");
			break;
		}	
		if (FD_ISSET(sock, &rfds))
		{
			
			int conn = accept(sock, (struct sockaddr *)&remote, &len);
			if (conn == -1)
			{
				perror("accept error:");
				break;
			}

			//客户端的地址信息，保存在remote
			printf("receive from %s port :%d\n",
			inet_ntoa(remote.sin_addr), ntohs(remote.sin_port) );

			pid_t pid = fork();
			if (pid > 0)
			{
				close(conn);
				continue;
			}
			else if (pid == 0)
			{
				handle_connection(conn);
				exit(0);
				
			}
		}
	}
}

