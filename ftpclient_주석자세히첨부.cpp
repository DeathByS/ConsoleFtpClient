#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#include <sys/stat.h>
#include <conio.h> 
#include <ctype.h> 
#include <direct.h>
#include <io.h>

// FTP ���ӽ� ����ϴ� ��Ʈ
#define PORT 21

// ���α׷� ������ ���̴� ���� �Լ���
void ErrorPrint(char *msg);
int ExtractStatusCode(char *msg);
void GetPass(char *buf, int max);
int ReadLine(SOCKET sock, char *buf, int max);
void sendProtocol(int sock, char *protocol);
void Passive_mode(SOCKET sock, char *ip, int *port);

// �� FTP ��ɾ ���� ó�� �Լ�
SOCKET Cmd_OPEN(char *host);			// ���� ���� �� �α��� �Լ�
void Cmd_DIR(SOCKET sock);				// ���丮 ������ �Լ�
void Cmd_MKDIR(SOCKET sock, char *dir);	// ���丮 ���� �Լ�
void Cmd_CD(SOCKET sock, char *dir);	// ���� �۾� ���丮 ���� �Լ�
void Cmd_DEL(SOCKET sock, char *file);	// ���� ���� �Լ�
void Cmd_GET(SOCKET sock, char *file);	// ���� �ٿ�ε� �Լ�
void Cmd_PUT(SOCKET sock, char *file);	// ���� ���ε� �Լ�
void Cmd_PWD(SOCKET sock);				// ���� �۾� ���丮 ��� �Լ�
void Cmd_LCD(char *dir);				// ���û󿡼� ���� �۾� ���丮 ���� �Լ�
void Cmd_LDIR();						// ���û󿡼� ���� �۾� ���丮 ������ �Լ�
void Cmd_LPWD();						// ���û󿡼� ���� �۾� ���丮 ��� �Լ�
void Cmd_HELP();						// ���� ���



// ���� ���� ���� ���� ���� ������
char connectedHost[256] = { 0, };			// ���� ���ӵ� ����
int isConnected = 0, logged = 0;		    // ���� ���ִ��� ���� ����(isConnected), �α��ε� �������� ����(logged)

#pragma comment(lib, "ws2_32.lib")

// ��ɾ� ���� 
char *cmds[] = {	"open",
					"dir", "ls",
					"cd",
					"mkdir",
					"del", "delete",
					"get",
					"put",
					"pwd",
					"lcd",
					"ldir", "lls",
					"lpwd",
					"help",
					"quit"
				};

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET sock;
	char *shost;
	char buf[1024], cmd[1024], arg[1024], *ptr;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorPrint("WSAStartup() Error");

	// ���α׷� ����� �Ѿ�� ���ڰ� 2����� �ٷ� argv[1]���� ������ ������ ����
	if (argc == 2) {
		shost = argv[1];
		sock = Cmd_OPEN(shost);
	}

	// ����� �Է¹ޱ� ���� ���� ����
	while (1) {
		printf("ftp> ");
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf) - 1] = '\0';

		memset(cmd, 0, sizeof(cmd));
		memset(arg, 0, sizeof(arg));

		// "��ɾ�"�� "����"�� ��ĭ�� �������� �������ִ� �۾�
		ptr = strchr(buf, ' ');
		if (ptr) {
			*ptr = '\0';
			ptr++;
			strncpy(arg, ptr, sizeof(arg));
		}
		strncpy(cmd, buf, sizeof(cmd));


		//
		// �� ��ɾ ���� ó��(�Լ��� ���ڸ� �Ѱ���)
		//
		if (!strcmp(cmd, "open")) {
			if (isConnected) // �̹� ���ӵ��ִ����� üũ
				printf("Already connected to %s, use disconnect first.\n");
			else
				sock = Cmd_OPEN(arg);
		}
		else if (!strcmp(cmd, "dir") || !strcmp(cmd, "ls")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }		// �α��� ���ִ� �������� üũ, �����󿡼� �۾��Ǵ� �ٸ� ��ɾ���� �����ϰ� üũ��
			Cmd_DIR(sock);
		}
		else if (!strcmp(cmd, "cd")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			Cmd_CD(sock, arg);
		}
		else if (!strcmp(cmd, "mkdir")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			if (*arg == '\0') { printf("Usage: mkdir dirname\n"); continue; }
			Cmd_MKDIR(sock, arg);
		}
		else if (!strcmp(cmd, "del") || !strcmp(cmd, "delete")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			if (*arg == '\0') { printf("Usage: del filename\n"); continue; }
			Cmd_DEL(sock, arg);
		}
		else if (!strcmp(cmd, "get")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			if (*arg == '\0') { printf("Usage: get filename\n"); continue; }
			Cmd_GET(sock, arg);
		}
		else if (!strcmp(cmd, "put")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			if (*arg == '\0') { printf("Usage: put filename\n"); continue; }
			Cmd_PUT(sock, arg);
		}
		else if (!strcmp(cmd, "pwd")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }
			Cmd_PWD(sock);
		}
		else if (!strcmp(cmd, "lcd"))
			Cmd_LCD(arg);
		else if (!strcmp(cmd, "ldir") || !strcmp(cmd, "lls"))
			Cmd_LDIR();
		else if (!strcmp(cmd, "lpwd"))
			Cmd_LPWD();
		else if (!strcmp(cmd, "help"))
			Cmd_HELP();
		else if (!strcmp(cmd, "quit")) {
			break;
		}
		else
			printf("Invalid Command.\n");
	}

	if (sock != -1)
		closesocket(sock);		// ���� �ݱ�
	WSACleanup();
	return 0;
}

// Error�� ����ϰ� �����Ŵ
void ErrorPrint(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

// ���ڿ��� �Է¹޾� �� ���� Status Code�� �����Ͽ� int���·� ��ȯ
int ExtractStatusCode(char *msg)
{
	char *ptr;
	int code;

	ptr = strchr(msg, ' ');	// ' '(��ĭ)�� �������� ������ ���ڸ� �����ϱ� ���� ��ĭ �˻�
	*ptr = '\0';				// ' '(��ĭ)�κ��� '\0'(�ι���)�� ġȯ�����ν� ���ڿ� �и�
	code = atoi(msg);		// ���ڿ����� ���� ����(integer)

	return code;
}


// �н����� �Է½� '*'(��ǥ)�� ǥ�õǰ� ����
void GetPass(char *buf, int max)
{
	int c, m = 0;

	// getch �Լ��� �ѱ��ڸ� �Է¹޵�, ȭ��� ǥ�õ��� ����
	while ((c = getch()) != EOF && m < (max - 1) && c != '\n' && c != '\r') {		// ���ѷ����� ���� ���� �Է¹���. ���� Ż�� ���� : EOF, max(�ִ��Է±���) �ʰ�, '\n'(�����ǵ�), '\r'(ĳ��������) 
		if (m > 0 && c == '\b') {			// '\b' (�齺���̽�) �Է½�  													
			printf("\b \b");						// ȭ��󿡼� ���� ����
			fflush(stdout);					
			m--;									// ī������ ����
			buf[m] = '\0';
		}
		else if (isalnum(c)) {					// ���ĺ� �Ǵ� �����̸� ���� ó��
			buf[m++] = (char)c;
			putchar('*');							// ȭ��� ��ǥ���
		}
	}

	printf("\n");
	buf[m] = '\0';
}

// ���Ͽ��� '\r\n' �� ���ö������� Recv�Ͽ� ��ȯ
int ReadLine(SOCKET sock, char *buf, int max)
{
	int total = 0;
	char c = 0, pc = 0;

	while (recv(sock, (char *)&c, 1, 0) > 0) {			// 1����Ʈ(�ѱ���)�� �������κ��� recv
		total += 1;
		if (total > max)
			break;

		*buf++ = c;
		if (pc == '\r' && c == '\n')			// �ٷ� �������ڰ� '\r', ���� ���ڰ� '\n' �̸� recv ����
			break;

		pc = c;
	}

	return total;
}


// ������ FTP������ ����
SOCKET Cmd_OPEN(char *host)
{
	SOCKET sock;
	SOCKADDR_IN serv_addr;
	struct hostent *phost;
	char buf[1024], tmphost[256], cmd[1024];
	int len, code;

	// "open" ��� ����� �����ּҰ� ���ڷ� �Ѿ���� �ʾҴٸ�
	// ���� �Է� ����
	if (host[0] == '\0') {
		printf("To: ");
		fgets(tmphost, sizeof(tmphost), stdin);
		tmphost[strlen(tmphost) - 1] = '\0';
		host = tmphost;
	}

	// host(����) ������ �ּҸ� hostent ����ü�� �°� ��ȯ(������ �ּҸ� �����Ƿ� ��ȯ)
	if ((phost = gethostbyname(host)) == NULL) {
		ErrorPrint("gethostbyname() Error");
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
		ErrorPrint("socket() Error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *(struct in_addr *)phost->h_addr_list[0];
	serv_addr.sin_port = htons(PORT);

	// ftp������ ����
	if (connect(sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	// ���� ftp������ �������� ���������� ǥ���ϱ� ���� ���� ���� ����
	isConnected = 1;
	strncpy(connectedHost, host, sizeof(connectedHost));

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// ����ڸ� �Է� �� ����
	printf("User (%s)): ", host);
	fgets(buf, sizeof(buf), stdin);
	buf[strlen(buf) - 1] = '\0';

	sprintf(cmd, "USER %s\r\n", buf);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// �����ڵ尡 331(���� �������)�� �ƴϸ� ��� ����
	code = ExtractStatusCode(buf);
	if (code != 331) {
		return sock;
	}

	// �н����� �Է� �� ����
	printf("Password: ");
	GetPass(buf, sizeof(buf));		// ��ǥ�� ǥ���ϱ� ���� ���� ���� GetPass �Լ� ȣ��

	sprintf(cmd, "PASS %s\r\n", buf);
	send(sock, cmd, strlen(cmd), 0);

	// �α��� ���� ���� Ȯ��
	while ((len = ReadLine(sock, (char *)buf, sizeof(buf))) > 0) {
		buf[len] = '\0';
		// 230 blahblah~~ �޽����� ���� �α��� ����, �α��� ���� ����
		if (!strncmp(buf, "230 ", 4)) {
			logged = 1;
			break;
		}
		// 530 blahblah~~ �޽����� ���� �α��� ����
		else if (!strncmp(buf, "530 ", 4)) {
			printf("Login failed.\n");
			break;
		}
	}

	return sock;
}

// Directory Listing ��� ���� �Լ�
void Cmd_DIR(SOCKET sock)
{
	char buf[1024], cmd[1024], ip[256];
	int len;
	struct hostent *phost;
	SOCKADDR_IN serv_addr;
	SOCKET nsock;
	int port;

	// Passive Mode Ȱ��ȭ
	Passive_mode(sock, ip, &port);

	if ((nsock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		ErrorPrint("socket() Error");

	// host(����) ������ �ּҸ� hostent ����ü�� �°� ��ȯ(������ �ּҸ� �����Ƿ� ��ȯ)
	if ((phost = gethostbyname(ip)) == NULL) {
		ErrorPrint("gethostbyname() Error");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *(struct in_addr *)phost->h_addr_list[0];
	serv_addr.sin_port = htons(port);

	// LIST ��� ����
	sprintf(cmd, "LIST\r\n");
	send(sock, cmd, strlen(cmd), 0);

	// passive mode�� open�� ������ ��Ʈ�� ����
	if (connect(nsock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// ���̻� ������ ���������� recv�� �޾� ���
	while ((len = recv(nsock, (char *)buf, sizeof(buf) - 1, 0)) > 0) {
		buf[len] = '\0';
		printf("%s", buf);
	}

	closesocket(nsock);

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);
}

// FTP �����󿡼��� ���� �۾� ���丮 ����(Change Directory) �Լ�
void Cmd_CD(SOCKET sock, char *dir)
{
	char cmd[1024];
	char buf[1024];
	int len;

	// CWD ��� ����
	sprintf(cmd, "CWD %s\r\n", dir);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';

	printf("%s", buf);
}

// ���� ����(Delete) �Լ�
void Cmd_DEL(SOCKET sock, char *file)
{
	char cmd[1024];
	char buf[1024];
	int len;

	sprintf(cmd, "DELE %s\r\n", file);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';

	printf("%s", buf);
}

void Cmd_MKDIR(SOCKET sock, char *dir)
{
	char cmd[1024];
	char buf[1024];
	int len;

	sprintf(cmd, "MKD %s\r\n", dir);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';

	printf("%s", buf);
}

// ���� �ٿ�ε� �Լ�
void Cmd_GET(SOCKET sock, char *file)
{
	char ip[256], filePath[256];
	char sBuf[1024], rBuf[1024];
	char readBuf[1024];
	int port;
	int len;
	int f_Size = 0;
	int r_Size, totalSize;

	FILE *fp;

	SOCKET dtp_Sock;
	SOCKADDR_IN servAddr;

	getcwd(filePath, 256);
	sprintf(filePath, "%s\\%s", filePath, file);

	printf("%s\n", filePath);

	// Passive ��� Ȱ��ȭ
	Passive_mode(sock, ip, &port);

	if ((dtp_Sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ErrorPrint("socket() error");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(ip);
	servAddr.sin_port = htons(port);

	// ���̳ʸ� ���� ����
	sprintf(sBuf, "TYPE I%s", "\r\n");

	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// passive mode�� open�� ������ ��Ʈ�� ����
	if (connect(dtp_Sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		ErrorPrint("connect() error");


	// ���� ���� ��û(RETR)
	sprintf(sBuf, "RETR %s%s", file, "\r\n");

	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// �����ڵ尡 550�̸� ���� ���� ��û ���з� �Ǵ�
	if (ExtractStatusCode(rBuf) == 550) {
		closesocket(dtp_Sock);
		return;
	}
	
	// ���� �ٿ�ε� ó�� �κ�
	r_Size = 0;
	totalSize = 0;
	fp = fopen(filePath, "wb");		// binary ���� file ���� �� open
	int i = 0;

	// ���̻� ���������Ͱ� ���������� �޾� ���Ϸ� write
	while ((r_Size = recv(dtp_Sock, readBuf, sizeof(readBuf) - 1, 0)) > 0) {
		fwrite(readBuf, 1, r_Size, fp);
	}

	closesocket(dtp_Sock);

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	fclose(fp);
}

// ���� ���ε� �Լ�
void Cmd_PUT(SOCKET sock, char *file)
{
	char sBuf[1024], rBuf[1024];
	char ip[256];
	int port, len, f_size;
	SOCKET datasock;
	SOCKADDR_IN dataaddr;
	FILE *fStream;

	// Passive ��� Ȱ��ȭ
	Passive_mode(sock, ip, &port);

	if ((datasock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		ErrorPrint("socket() error");

	memset(&dataaddr, 0, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = inet_addr(ip);
	dataaddr.sin_port = htons(port);

	// ���̳ʸ� ���� ����
	sprintf(sBuf, "TYPE I\r\n");
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// passive mode�� open�� ������ ��Ʈ�� ����
	if (connect(datasock, (SOCKADDR *)&dataaddr, sizeof(dataaddr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	// STOR ��� ����
	sprintf(sBuf, "STOR %s\r\n", file);
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	if ((fStream = fopen(file, "rb")) == NULL) {	// binary ���� file open
		printf("fopen error()\n");
		closesocket(datasock);
		return;
	}

	// ���̻� ���� ���� ������ ���������� �о� ������ ����
	while ((f_size = fread(sBuf, 1, sizeof(sBuf), fStream)) > 0){
		if (send(datasock, sBuf, f_size, 0) != f_size)
			ErrorPrint("send() error");
	}
	fclose(fStream);
	closesocket(datasock);

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);
}

// FTP �����󿡼� ��ġ�� ���� �۾����丮 ���
void Cmd_PWD(SOCKET sock)
{
	char cmd[1024];
	char buf[1024];
	int len;

	sprintf(cmd, "PWD\r\n");
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';

	printf("%s", buf);
}

// ���û󿡼� ���� �۾����丮 ���� �Լ�
void Cmd_LCD(char *dir)
{
	char *buf = NULL;

	if (_chdir(dir) == 0){		// chdir = change directory
		if (strcmp(dir, "..")) {
			_chdir(dir);
			printf("%s\n", getcwd(buf, sizeof(buf)));		// ���� �۾����丮 ��� ���
		}
		else
			printf("%s\n", getcwd(buf, sizeof(buf)));		// ���� �۾����丮 ��� ���
	}
	else
		printf("not exist directory");
}

// ���û󿡼� ���� �۾����丮 ������ ��� �Լ�
void Cmd_LDIR()
{
	long hd;
	struct _finddata_t fd;
	char path[100];
	int res = 1;

	getcwd(path, sizeof(path));
	printf("%s\n", path);

	strcat(path, "\\*.*");

	if ((hd = _findfirst(path, &fd)) == -1)		// ���丮 open & ù �׸�(���ϼ��� �ְ� ���丮�ϼ��� ����) ���� ���
		return;

	while (res != -1) {							// ���丮�� ��� �׸��� ������
		printf("%s\n", fd.name);				// �׸� name ���
		res = _findnext(hd, &fd);				// ���� �׸��� ������
	}

	_findclose(hd);
}

// ���û󿡼� ���� �۾� ���丮 ��� �Լ�
void Cmd_LPWD()
{
	char path[256];

	getcwd(path, sizeof(path));
	printf("%s\n", path);
}


// Passive Mode Ȱ��ȭ �Լ�
void Passive_mode(SOCKET sock, char *ip, int *port)
{
	char sBuf[1024], rBuf[1024];
	char *temp;
	int len;
	int h0, h1, h2, h3;
	int p0, p1;


	// PASV ��� ����
	sprintf(sBuf, "PASV\r\n");
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	temp = strchr(rBuf, '('); // '(' �˻�, '(' ���Ŀ� �������ּ�, ��Ʈ��ȣ�� ����
	temp++;

	// �������ּ�, ��Ʈ��ȣ�� �ش��ϴ� �ڸ��� �о� �� ������ ����
	sscanf(temp, "%d,%d,%d,%d,%d,%d", &h0, &h1, &h2, &h3, &p0, &p1);
	// �������ּ� ����
	sprintf(ip, "%d.%d.%d.%d", h0, h1, h2, h3);

	// ��Ʈ��ȣ ����
	*port = (p0 * 256) + p1;
}

// ����(��ɾ� ����Ʈ) ���
void Cmd_HELP()
{
	int i;

	printf("[COMMANDS] : ");
	for (i = 0; i < 16; i++) {
		printf("%s ", cmds[i]);
	}
	printf("\n");

}