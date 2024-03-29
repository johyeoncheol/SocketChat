//이후로 다루는 대부분의 코드는 ITCOOKBOOK 윈도우네트워크프로그래밍 김선우저 소스에서 가져다 사용합니다.
#pragma warning(disable:4996)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

#define BUFSIZE 512 //HEADSIZE+DATASIZE
#define HEADSIZE 12
#define DATASIZE 500
////////////////////////////////////////////
SOCKET sockArray[100];
int sockCount=0;
////////////////////////////////////////////
// 소켓 함수 오류 출력 후 종료
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER| // 오류 메시지 저장 메모리를 내부에서 할당하라
		FORMAT_MESSAGE_FROM_SYSTEM, //운영체제로 부터 오류 메시지를 가져온다
		NULL, 
        WSAGetLastError(), //오류 코드
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //언어(제어판 설정 언어)
		(LPTSTR)&lpMsgBuf, // 오류 메시지 outparam
        0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf); // 오류 메시지 저장 메모리 반환 
	exit(-1);
}

// 소켓 함수 오류 출력
void err_display(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}
void AddSocket(SOCKET arr[], int& count, int data)
{
	arr[count++] = data;
}
void RemoveSocket(SOCKET arr[], int& count, int idx)
{
	for (int i = idx; i < count - 1; ++i)
		arr[i] = arr[i + 1];
	--count;
}
int FindSocket(SOCKET arr[], int count, int data)
{
	for (int i = 0; i < count; ++i)
		if (arr[i] == data)
			return i;
	return -1;
}
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}
DWORD WINAPI RWClient(LPVOID param)
{
	SOCKET client_sock = (SOCKET)param;

	int retval = 0;
	char buf[BUFSIZE];

	// 클라이언트와 데이터 통신
	while (1) {
		// 데이터 받기
		retval = recvn(
			client_sock, //통신소켓핸들
			buf, //받을 애플리케이션 버퍼
			BUFSIZE, //수신 버퍼의 최대 크기
			0 //대부분 0 or MSG_PEEK와 MSG_OOB를 사용 가능
		);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}
		else {
			// 받은 데이터 출력
			switch (*(int*)(buf + 4))
			{
			case 'S':
				printf("클라이언트 보낸 데이터 :: %s\n", buf+HEADSIZE);
				for (int i = 0; i < sockCount; ++i)
				{
					SOCKET sock = sockArray[i];
					send(sock, buf, retval, 0);
				}
				break;
			case 'O':
			{
				int x, y, f;
				x = *(int*)(buf + HEADSIZE + 1);
				y = *(int*)(buf + HEADSIZE + 5);
				f = x + y;
				printf("클라이언트 요청한 연산 %d + %d = %d\n", x, y, f);
				*(int*)(buf + HEADSIZE) = f;
				send(client_sock, buf, retval, 0);
			}
				break;
			default:
				break;
			}
		}
	}

	// closesocket()
	closesocket(client_sock);
	printf("%d 소켓 종료\n", client_sock);
	//printf("\n[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
	//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	RemoveSocket(sockArray, sockCount, 
		FindSocket(sockArray, sockCount, client_sock));


	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) 
		return -1;

	// socket()
	SOCKET listen_sock = socket(
        AF_INET, //주소체계: 통신 영역 설정, 인터넷 영역을 사용하며 리모트 컴퓨터 사이의 통신을 사용, IPv4
        SOCK_STREAM, //프로토콜유형: TCP/IP 기반 사용
        0 //앞 두 인자로 프로토콜 결정이 명확하면 0사용, IPPROTO_TCP, IPPROTO_UDP
        );
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");	
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET; //주소체계
	serveraddr.sin_port = htons(9000); //지역포트번호
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //지역IP 주소
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");
	
	// listen()
	retval = listen(
        listen_sock, 
        SOMAXCONN //접속대기 큐의 크기
        ); // TCP 상태를 LISTENING 변경
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFSIZE+1];

	while(1){
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(
            listen_sock, //대기 소켓
            (SOCKADDR *)&clientaddr, //클라이언트의 정보 out param
            &addrlen //주소구조체형식의크기, in(크기지정), out(초기화한크기반환) param
            ); //통신소켓 생성: 원격 IP, 원격 포트 결정
		if(client_sock == INVALID_SOCKET){
			err_display("accept()");
			continue;
		}
		AddSocket(sockArray, sockCount, client_sock);
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), //문자열로 IP주소 변환
            ntohs(clientaddr.sin_port) // 포트번호 network to host
            );

		DWORD id;
		CreateThread(NULL, 0, RWClient,(LPVOID)client_sock, 0, &id);

	}

	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}