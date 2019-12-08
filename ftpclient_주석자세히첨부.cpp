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

// FTP 접속시 사용하는 포트
#define PORT 21

// 프로그램 내에서 쓰이는 여러 함수들
void ErrorPrint(char *msg);
int ExtractStatusCode(char *msg);
void GetPass(char *buf, int max);
int ReadLine(SOCKET sock, char *buf, int max);
void sendProtocol(int sock, char *protocol);
void Passive_mode(SOCKET sock, char *ip, int *port);

// 각 FTP 명령어에 대한 처리 함수
SOCKET Cmd_OPEN(char *host);			// 서버 접속 및 로그인 함수
void Cmd_DIR(SOCKET sock);				// 디렉토리 리스팅 함수
void Cmd_MKDIR(SOCKET sock, char *dir);	// 디렉토리 생성 함수
void Cmd_CD(SOCKET sock, char *dir);	// 현재 작업 디렉토리 변경 함수
void Cmd_DEL(SOCKET sock, char *file);	// 파일 삭제 함수
void Cmd_GET(SOCKET sock, char *file);	// 파일 다운로드 함수
void Cmd_PUT(SOCKET sock, char *file);	// 파일 업로드 함수
void Cmd_PWD(SOCKET sock);				// 현재 작업 디렉토리 출력 함수
void Cmd_LCD(char *dir);				// 로컬상에서 현재 작업 디렉토리 변경 함수
void Cmd_LDIR();						// 로컬상에서 현재 작업 디렉토리 리스팅 함수
void Cmd_LPWD();						// 로컬상에서 현재 작업 디렉토리 출력 함수
void Cmd_HELP();						// 도움말 출력



// 현재 접속 상태 관련 전역 변수들
char connectedHost[256] = { 0, };			// 현재 접속된 서버
int isConnected = 0, logged = 0;		    // 접속 되있는지 여부 저장(isConnected), 로그인된 상태인지 여부(logged)

#pragma comment(lib, "ws2_32.lib")

// 명령어 집합 
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

	// 프로그램 실행시 넘어온 인자가 2개라면 바로 argv[1]에서 지정된 서버로 접속
	if (argc == 2) {
		shost = argv[1];
		sock = Cmd_OPEN(shost);
	}

	// 명령을 입력받기 위해 무한 루프
	while (1) {
		printf("ftp> ");
		fgets(buf, sizeof(buf), stdin);
		buf[strlen(buf) - 1] = '\0';

		memset(cmd, 0, sizeof(cmd));
		memset(arg, 0, sizeof(arg));

		// "명령어"와 "인자"를 빈칸을 기준으로 구분해주는 작업
		ptr = strchr(buf, ' ');
		if (ptr) {
			*ptr = '\0';
			ptr++;
			strncpy(arg, ptr, sizeof(arg));
		}
		strncpy(cmd, buf, sizeof(cmd));


		//
		// 각 명령어에 대한 처리(함수로 인자를 넘겨줌)
		//
		if (!strcmp(cmd, "open")) {
			if (isConnected) // 이미 접속되있는지를 체크
				printf("Already connected to %s, use disconnect first.\n");
			else
				sock = Cmd_OPEN(arg);
		}
		else if (!strcmp(cmd, "dir") || !strcmp(cmd, "ls")) {
			if (logged == 0) { printf("You must be logged\n"); continue; }		// 로그인 되있는 상태인지 체크, 서버상에서 작업되는 다른 명령어에서도 동일하게 체크됨
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
		closesocket(sock);		// 소켓 닫기
	WSACleanup();
	return 0;
}

// Error를 출력하고 종료시킴
void ErrorPrint(char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

// 문자열을 입력받아 맨 앞의 Status Code를 추출하여 int형태로 반환
int ExtractStatusCode(char *msg)
{
	char *ptr;
	int code;

	ptr = strchr(msg, ' ');	// ' '(빈칸)을 기준으로 앞쪽의 숫자를 추출하기 위해 빈칸 검색
	*ptr = '\0';				// ' '(빈칸)부분을 '\0'(널문자)로 치환함으로써 문자열 분리
	code = atoi(msg);		// 문자열에서 숫자 추출(integer)

	return code;
}


// 패스워드 입력시 '*'(별표)로 표시되게 만듬
void GetPass(char *buf, int max)
{
	int c, m = 0;

	// getch 함수는 한글자를 입력받되, 화면상에 표시되지 않음
	while ((c = getch()) != EOF && m < (max - 1) && c != '\n' && c != '\r') {		// 무한루프를 돌며 문자 입력받음. 루프 탈출 조건 : EOF, max(최대입력길이) 초과, '\n'(라인피드), '\r'(캐리지리턴) 
		if (m > 0 && c == '\b') {			// '\b' (백스페이스) 입력시  													
			printf("\b \b");						// 화면상에서 글자 지움
			fflush(stdout);					
			m--;									// 카운팅을 줄임
			buf[m] = '\0';
		}
		else if (isalnum(c)) {					// 알파벳 또는 숫자이면 정상 처리
			buf[m++] = (char)c;
			putchar('*');							// 화면상에 별표출력
		}
	}

	printf("\n");
	buf[m] = '\0';
}

// 소켓에서 '\r\n' 가 나올때까지만 Recv하여 반환
int ReadLine(SOCKET sock, char *buf, int max)
{
	int total = 0;
	char c = 0, pc = 0;

	while (recv(sock, (char *)&c, 1, 0) > 0) {			// 1바이트(한글자)씩 소켓으로부터 recv
		total += 1;
		if (total > max)
			break;

		*buf++ = c;
		if (pc == '\r' && c == '\n')			// 바로 이전문자가 '\r', 현재 문자가 '\n' 이면 recv 종료
			break;

		pc = c;
	}

	return total;
}


// 지정한 FTP서버로 접속
SOCKET Cmd_OPEN(char *host)
{
	SOCKET sock;
	SOCKADDR_IN serv_addr;
	struct hostent *phost;
	char buf[1024], tmphost[256], cmd[1024];
	int len, code;

	// "open" 명령 실행시 서버주소가 인자로 넘어오지 않았다면
	// 직접 입력 받음
	if (host[0] == '\0') {
		printf("To: ");
		fgets(tmphost, sizeof(tmphost), stdin);
		tmphost[strlen(tmphost) - 1] = '\0';
		host = tmphost;
	}

	// host(서버) 도메인 주소를 hostent 구조체에 맞게 변환(도메인 주소를 아이피로 변환)
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

	// ftp서버에 접속
	if (connect(sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	// 현재 ftp서버에 접속중인 상태인지를 표시하기 위해 관련 변수 셋팅
	isConnected = 1;
	strncpy(connectedHost, host, sizeof(connectedHost));

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// 사용자명 입력 후 전송
	printf("User (%s)): ", host);
	fgets(buf, sizeof(buf), stdin);
	buf[strlen(buf) - 1] = '\0';

	sprintf(cmd, "USER %s\r\n", buf);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// 상태코드가 331(정상 사용자임)가 아니면 즉시 리턴
	code = ExtractStatusCode(buf);
	if (code != 331) {
		return sock;
	}

	// 패스워드 입력 후 전송
	printf("Password: ");
	GetPass(buf, sizeof(buf));		// 별표로 표시하기 위해 직접 만든 GetPass 함수 호출

	sprintf(cmd, "PASS %s\r\n", buf);
	send(sock, cmd, strlen(cmd), 0);

	// 로그인 성공 여부 확인
	while ((len = ReadLine(sock, (char *)buf, sizeof(buf))) > 0) {
		buf[len] = '\0';
		// 230 blahblah~~ 메시지가 오면 로그인 성공, 로그인 과정 종료
		if (!strncmp(buf, "230 ", 4)) {
			logged = 1;
			break;
		}
		// 530 blahblah~~ 메시지가 오면 로그인 실패
		else if (!strncmp(buf, "530 ", 4)) {
			printf("Login failed.\n");
			break;
		}
	}

	return sock;
}

// Directory Listing 기능 구현 함수
void Cmd_DIR(SOCKET sock)
{
	char buf[1024], cmd[1024], ip[256];
	int len;
	struct hostent *phost;
	SOCKADDR_IN serv_addr;
	SOCKET nsock;
	int port;

	// Passive Mode 활성화
	Passive_mode(sock, ip, &port);

	if ((nsock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		ErrorPrint("socket() Error");

	// host(서버) 도메인 주소를 hostent 구조체에 맞게 변환(도메인 주소를 아이피로 변환)
	if ((phost = gethostbyname(ip)) == NULL) {
		ErrorPrint("gethostbyname() Error");
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr = *(struct in_addr *)phost->h_addr_list[0];
	serv_addr.sin_port = htons(port);

	// LIST 명령 전송
	sprintf(cmd, "LIST\r\n");
	send(sock, cmd, strlen(cmd), 0);

	// passive mode로 open된 서버의 포트로 접속
	if (connect(nsock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);

	// 더이상 받을게 없을때까지 recv로 받아 출력
	while ((len = recv(nsock, (char *)buf, sizeof(buf) - 1, 0)) > 0) {
		buf[len] = '\0';
		printf("%s", buf);
	}

	closesocket(nsock);

	len = recv(sock, (char *)buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';
	printf("%s", buf);
}

// FTP 서버상에서의 현재 작업 디렉토리 변경(Change Directory) 함수
void Cmd_CD(SOCKET sock, char *dir)
{
	char cmd[1024];
	char buf[1024];
	int len;

	// CWD 명령 전송
	sprintf(cmd, "CWD %s\r\n", dir);
	send(sock, cmd, strlen(cmd), 0);

	len = recv(sock, buf, sizeof(buf) - 1, 0);
	buf[len] = '\0';

	printf("%s", buf);
}

// 파일 삭제(Delete) 함수
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

// 파일 다운로드 함수
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

	// Passive 모드 활성화
	Passive_mode(sock, ip, &port);

	if ((dtp_Sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		ErrorPrint("socket() error");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(ip);
	servAddr.sin_port = htons(port);

	// 바이너리 모드로 변경
	sprintf(sBuf, "TYPE I%s", "\r\n");

	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// passive mode로 open된 서버의 포트로 접속
	if (connect(dtp_Sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
		ErrorPrint("connect() error");


	// 파일 전송 요청(RETR)
	sprintf(sBuf, "RETR %s%s", file, "\r\n");

	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// 상태코드가 550이면 파일 전송 요청 실패로 판단
	if (ExtractStatusCode(rBuf) == 550) {
		closesocket(dtp_Sock);
		return;
	}
	
	// 파일 다운로드 처리 부분
	r_Size = 0;
	totalSize = 0;
	fp = fopen(filePath, "wb");		// binary 모드로 file 생성 및 open
	int i = 0;

	// 더이상 받을데이터가 없을때까지 받아 파일로 write
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

// 파일 업로드 함수
void Cmd_PUT(SOCKET sock, char *file)
{
	char sBuf[1024], rBuf[1024];
	char ip[256];
	int port, len, f_size;
	SOCKET datasock;
	SOCKADDR_IN dataaddr;
	FILE *fStream;

	// Passive 모드 활성화
	Passive_mode(sock, ip, &port);

	if ((datasock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		ErrorPrint("socket() error");

	memset(&dataaddr, 0, sizeof(dataaddr));
	dataaddr.sin_family = AF_INET;
	dataaddr.sin_addr.s_addr = inet_addr(ip);
	dataaddr.sin_port = htons(port);

	// 바이너리 모드로 변경
	sprintf(sBuf, "TYPE I\r\n");
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	// passive mode로 open된 서버의 포트로 접속
	if (connect(datasock, (SOCKADDR *)&dataaddr, sizeof(dataaddr)) == SOCKET_ERROR)
		ErrorPrint("connect() Error");

	// STOR 명령 전송
	sprintf(sBuf, "STOR %s\r\n", file);
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	if ((fStream = fopen(file, "rb")) == NULL) {	// binary 모드로 file open
		printf("fopen error()\n");
		closesocket(datasock);
		return;
	}

	// 더이상 읽을 파일 내용이 없을때까지 읽어 서버로 전송
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

// FTP 서버상에서 위치한 현재 작업디렉토리 출력
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

// 로컬상에서 현재 작업디렉토리 변경 함수
void Cmd_LCD(char *dir)
{
	char *buf = NULL;

	if (_chdir(dir) == 0){		// chdir = change directory
		if (strcmp(dir, "..")) {
			_chdir(dir);
			printf("%s\n", getcwd(buf, sizeof(buf)));		// 현재 작업디렉토리 경로 출력
		}
		else
			printf("%s\n", getcwd(buf, sizeof(buf)));		// 현재 작업디렉토리 경로 출력
	}
	else
		printf("not exist directory");
}

// 로컬상에서 현재 작업디렉토리 리스팅 기능 함수
void Cmd_LDIR()
{
	long hd;
	struct _finddata_t fd;
	char path[100];
	int res = 1;

	getcwd(path, sizeof(path));
	printf("%s\n", path);

	strcat(path, "\\*.*");

	if ((hd = _findfirst(path, &fd)) == -1)		// 디렉토리 open & 첫 항목(파일수도 있고 디렉토리일수도 있음) 정보 얻기
		return;

	while (res != -1) {							// 디렉토리의 모든 항목을 가져옴
		printf("%s\n", fd.name);				// 항목 name 출력
		res = _findnext(hd, &fd);				// 다음 항목을 가져옴
	}

	_findclose(hd);
}

// 로컬상에서 현재 작업 디렉토리 출력 함수
void Cmd_LPWD()
{
	char path[256];

	getcwd(path, sizeof(path));
	printf("%s\n", path);
}


// Passive Mode 활성화 함수
void Passive_mode(SOCKET sock, char *ip, int *port)
{
	char sBuf[1024], rBuf[1024];
	char *temp;
	int len;
	int h0, h1, h2, h3;
	int p0, p1;


	// PASV 명령 전송
	sprintf(sBuf, "PASV\r\n");
	if (send(sock, sBuf, strlen(sBuf), 0) != strlen(sBuf))
		ErrorPrint("send() error");

	if ((len = recv(sock, (char *)rBuf, sizeof(rBuf) - 1, 0)) <= 0)
		ErrorPrint("recv() error");

	rBuf[len] = '\0';
	printf("%s", rBuf);

	temp = strchr(rBuf, '('); // '(' 검색, '(' 이후에 아이피주소, 포트번호가 나옴
	temp++;

	// 아이피주소, 포트번호에 해당하는 자리를 읽어 각 변수에 저장
	sscanf(temp, "%d,%d,%d,%d,%d,%d", &h0, &h1, &h2, &h3, &p0, &p1);
	// 아이피주소 형성
	sprintf(ip, "%d.%d.%d.%d", h0, h1, h2, h3);

	// 포트번호 형성
	*port = (p0 * 256) + p1;
}

// 도움말(명령어 리스트) 출력
void Cmd_HELP()
{
	int i;

	printf("[COMMANDS] : ");
	for (i = 0; i < 16; i++) {
		printf("%s ", cmds[i]);
	}
	printf("\n");

}