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

int terminate = 0;//�����˳���־��0��ʾ���˳���1��ʾ�˳�


int connect_tcp_server(char *servIP, short servPORT)
{
	//1.socket:����һ���׽���
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket error:");
		return -1;
	}


	//�ͻ���Ҳ���԰󶨡�
	//��Ҳ�ã�����Ҳ�ã�����
	//���׽��֣���һ������һ����ַ(ip+port)
	//�㲻�󶨣���ô�ں˾ͻᶯ̬������䡣

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



	

	//2. connect�����ӷ�����
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
	//1����Ŀ¼
	int er;					//�������
	DIR* dir;
	dir = opendir(pathname);
	if( NULL == dir )
	{
		perror("opendir error");
		return -1;
	}
	//�Ͻ��������ж���ʲô�ļ���Ŀ¼�ļ���������

	//2)����Ŀ¼����
	struct dirent *dire;
	//char *path_buf[PATHNAME];
	while(dire = readdir(dir))
	{
		//������ǰ����һ��Ŀ¼
		if(	!strcmp(dire->d_name, ".") ||
			!strcmp(dire->d_name, ".."))
		{
			continue;
		}
		//��ȴ����ļ�������
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
	
	//������һ�� c0Ϊֹ
	do
	{
		read(sock,&ch, 1);
		
	}while (ch != 0xc0 && !terminate);
	//printf("----%d------\n", __LINE__);


	while (ch == 0xc0)
	{
		read(sock, &ch, 1);
	}

	//��ʱ ch������������еĵ�һ���ַ�
	
	//printf("----%d------\n", __LINE__);

	while (ch != 0xc0)
	{
	//	printf("1,i = %d\n", i);
		cmd[i++] = ch;		
		//printf("2,i = %d\n", i);
		read(sock, &ch,1);
	}

	//printf("----%d------\n", __LINE__);

	//ȥ����ͷ����β������������ͱ�����cmd[0] ~ cmd[i-1]
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
		cmd�в�������ͷ�Ͱ�β��ͬʱ����
		���ܺ�����Ҫת����ַ�(0xc0, 0xdd)
		0xc0 -> 0xdd 0xdc
		0xdd -> 0xdd 0xdb
*/
void send_cmd_to_server(int sock, unsigned char cmd[], int len)
{
	unsigned char send_c[512];
	int i = 0;
	int j = 0;

	send_c[i++] = 0xc0; //��ͷ

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

	send_c[i++] = 0xc0;//��β

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
	send_c[i++] = 0xc0; //��ͷ

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

	send_c[i++] = 0xc0;//��β

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
	
	//  �����ֶ�(4) + �����(1) +��������(1) + ����1(1) + ����1ʵ�ʳ��� + .... +
	int packet_len = 4 + 1 + 1;
	

	//1.��������������
	//send_cmd[i++]= 0xc0;


	//packet_len: 0x00000006
	//packet_len => send_cmd[1] send_cmd[2] send_cmd[3] send_cmd[4]
	//					0x06		0x00		0x00	 0x00
	//����� ��С��ģʽ���
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;

	//�����,1byte
	send_cmd[i++] = FTP_CMD_LS;

	//����Ĳ���������1byte
	send_cmd[i++] = 0;


	//send_cmd[i++] = 0xc0;


	//2.����
	send_cmd_to_server(sock, send_cmd, i);
		
	printf("%s L_%d\n", __FUNCTION__, __LINE__);


	//�ȴ�ftp server�Ļظ�
	//�� ������ִ��״̬
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
	
	//�����ؽ��
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
	while(cmd[0] == ' ')   //ȥ���ո�
	{
		cmd++;
	}
	unsigned int contain_len = strlen(cmd) - 1;//�ļ����ݴ�С ȥ�����Ļ��з�
	
	//����ص�*******************************************************************
	//ͷ���ṹ��  ����       ����� �������� �������� ��������
	int packet_len = 4 + 1 + 1 + 1 + contain_len;

	//��������
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;
	
	//�����,1byte
	send_cmd[i++] = FTP_CMD_GET;

	//����Ĳ���������1byte
	send_cmd[i++] = 1;

	//�����������
	send_cmd[i++] = contain_len;
	//��������
	strncpy(send_cmd + 7, cmd, contain_len);
	
	//������*********************************************************
/*
	int j = 0;
	while(j < i + contain_len)
	{
		printf("send_cmd = %02x  \n", send_cmd[j++]);
	}
	*/
	//******************************************************************
	send_cmd_to_server(sock, send_cmd, i + contain_len);

	//�ȴ�ftp server�Ļظ�
	//�� ������ִ��״̬
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
	
	//���շ������������ļ���С
	len_get = 0;
	read_ftp_resp(sock, f_size, &len_get);
	if(len_get != 4)
	{
		printf("ftp resp file size ERROR\n");
		return ;
	}
	// p[0] p[1] p[2] p[3]
	//С��ģʽ��p[0]���ֽ� p[3] ���ֽ�
	len = (f_size[0] & 0xff) |
		 ((f_size[1] & 0xff) << 8) |
		 ((f_size[2] & 0xff) << 16) |
		((f_size[3] & 0xff) <<  24);

	printf("len_get = %d\n", len_get);
	printf("len = %d\n", len);
	//���շ������������ļ�����

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
	int real_size = 0;					//��ʵ���յ��ֽ���
	int sum_size = 0;					//�ܵĽ��յ����ֽ���
	int size = 0;
	unsigned char p[MAX_CAPACITY] = {0};
	while(sum_size < len)
	{
		//����
		read_ftp_resp(sock, p, &size);
		printf("*********len = %d\n", size);
	//	puts(p + 4);
		//��һ֡���ݵ�ʵ�ʳ��ȱ����� p[0] p[1] p[2] p[3], С��ģʽ���
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

/*	//�����ؽ��
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
	while(cmd[0] == ' ')   //ȥ���ո�
	{
		cmd++;
	}
	unsigned int contain_len = strlen(cmd) - 1;//�ļ����ݴ�С ȥ�����Ļ��з�
	
	//����ص�*******************************************************************
	//ͷ���ṹ��  ����		����� �������� �������� ��������
	int packet_len = 4 + 1 + 1 + 1 + contain_len;

	//��������
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;
	
	//�����,1byte
	send_cmd[i++] = FTP_CMD_PUT;

	//����Ĳ���������1byte
	send_cmd[i++] = 1;

	//�����������
	send_cmd[i++] = contain_len;
	//��������
	strncpy(send_cmd + 7, cmd, contain_len);
	
	//������*********************************************************
/*
	int j = 0;
	while(j < i + contain_len)
	{
		printf("send_cmd = %02x  \n", send_cmd[j++]);
	}
	*/
	//******************************************************************
	send_cmd_to_server(sock, send_cmd, i + contain_len);


	//�ȴ�ftp server�Ļظ�
	//�� ������ִ��״̬
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
	
	//4)�����ļ��Ĵ�С
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

	
	//5)��ӡ�ļ�
		int sum_size = 0;
		int real_size = 0;
		int size = 0;
		unsigned char p[MAX_CAPACITY] = {0} ;
		//����
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
				//����
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
	
	//	�����ֶ�(4) + �����(1) +��������(1) + ����1(1) + ����1ʵ�ʳ��� + .... +
	int packet_len = 4 + 1 + 1;
	

	//1.��������������
	//send_cmd[i++]= 0xc0;


	//packet_len: 0x00000006
	//packet_len => send_cmd[1] send_cmd[2] send_cmd[3] send_cmd[4]
	//					0x06		0x00		0x00	 0x00
	//����� ��С��ģʽ���
	send_cmd[i++] = packet_len & 0xff;
	send_cmd[i++] = (packet_len >> 8 ) & 0xff;
	send_cmd[i++] = (packet_len >> 16) & 0xff;
	send_cmd[i++] = (packet_len >> 24) & 0xff;

	//�����,1byte
	send_cmd[i++] = FTP_CMD_BYE;

	//����Ĳ���������1byte
	send_cmd[i++] = 0;


	//send_cmd[i++] = 0xc0;


	//2.����
	send_cmd_to_server(sock, send_cmd, i);
		
	printf("%s L_%d\n", __FUNCTION__, __LINE__);


	//�ȴ�ftp server�Ļظ�
	//�� ������ִ��״̬
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
		//���������ʾ�� ftp >
		char *ftp_indi = "ftp >";
		fwrite(ftp_indi, 1, strlen(ftp_indi), stdout);

		//1.�Ӽ��̽����û�������
		unsigned char cmd[512];
		fgets(cmd, 511, stdin);


		//2.��������ɡ����������ʽ����������
		if (strncmp(cmd, "ls", 2) == 0)
		{
			//�û�������ls��������

			//printf("%s L_%d\n", __FUNCTION__, __LINE__);

			ftp_cmd_ls(sock, cmd);
			printf("%s L_%d\n", __FUNCTION__, __LINE__);
			
		}
		else if (strncmp(cmd, "get", 3) == 0)
		{
			//�û�������get��������
			ftp_cmd_get(sock, cmd);
		}
		else if (strncmp(cmd, "put", 3) == 0)
		{
			//�û�������put��������
			ftp_cmd_put(sock, cmd);
		}
		else if (strncmp(cmd, "bye", 3) == 0)
		{
			//�û�������bye��������
			ftp_cmd_bye(sock, cmd);
		}



	
	}

	//�����˳���������
	close(sock);



}

