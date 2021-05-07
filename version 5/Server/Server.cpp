//���ķ� �ٷ�� ��κ��� �ڵ�� ITCOOKBOOK �������Ʈ��ũ���α׷��� �輱���� �ҽ����� ������ ����մϴ�.
#pragma warning(disable:4996)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>

#define BUFSIZE 512 //HEADSIZE+DATASIZE
#define HEADSIZE 12
#define DATASIZE 500

struct SocketInfo
{
	SOCKET sock;
	char clientIP[50];
	unsigned short clientPort;
};
////////////////////////////////////////////
SocketInfo sockArray[100];
int sockCount=0;
////////////////////////////////////////////
// ���� �Լ� ���� ��� �� ����
void err_quit(const char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER| // ���� �޽��� ���� �޸𸮸� ���ο��� �Ҵ��϶�
		FORMAT_MESSAGE_FROM_SYSTEM, //�ü���� ���� ���� �޽����� �����´�
		NULL, 
        WSAGetLastError(), //���� �ڵ�
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //���(������ ���� ���)
		(LPTSTR)&lpMsgBuf, // ���� �޽��� outparam
        0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf); // ���� �޽��� ���� �޸� ��ȯ 
	exit(-1);
}

// ���� �Լ� ���� ���
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
void AddSocket(SocketInfo arr[], int& count, SOCKET sock)
{
	SOCKADDR_IN sockin;
	int socklen = sizeof(sockin);
	char ip[50];
	unsigned short port;
	getpeername(sock, (SOCKADDR*)& sockin, &socklen); //���� ��Ʈ��ȣ�� ip �ּҸ� �����Ѵ�.
	//sock ->�ּҹ޾ƿ��� ���� ���� sockin -> ������ ���� ����ü�� ������ 
	//socklen -> ���� ������ ũ�⸦ ���� ������
	strcpy_s(ip, 50, inet_ntoa(sockin.sin_addr));
	port = ntohs(sockin.sin_port);

	arr[count].sock = sock;
	strcpy_s(arr[count].clientIP , 50,  ip);
	arr[count++].clientPort = port;
}
void RemoveSocket(SocketInfo arr[], int& count, int idx)
{
	for (int i = idx; i < count - 1; ++i)
		arr[i] = arr[i + 1];
	--count;
}
int FindSocket(SocketInfo arr[], int count, SOCKET sock)
{
	for (int i = 0; i < count; ++i)
		if (arr[i].sock == sock)
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

	// Ŭ���̾�Ʈ�� ������ ���
	while (1) {
		// ������ �ޱ�
		retval = recvn(
			client_sock, //��ż����ڵ�
			buf, //���� ���ø����̼� ����
			BUFSIZE, //���� ������ �ִ� ũ��
			0 //��κ� 0 or MSG_PEEK�� MSG_OOB�� ��� ����
		);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}
		else {
			// ���� ������ ���
			switch (*(int*)(buf + 4))
			{
			case 'S':
				printf("Ŭ���̾�Ʈ ���� ������ :: %s\n", buf+HEADSIZE);
				for (int i = 0; i < sockCount; ++i)
				{
					SOCKET sock = sockArray[i].sock;
					char sendBuf[BUFSIZE];			

					int idx = FindSocket(sockArray, sockCount, client_sock);
					memcpy(sendBuf, buf, HEADSIZE);
					wsprintf(sendBuf+HEADSIZE, "[%s,%d]:",
						sockArray[idx].clientIP, sockArray[idx].clientPort);
					wsprintf(sendBuf + HEADSIZE + 30, "%s", buf + HEADSIZE);
					send(sock, sendBuf, BUFSIZE, 0);
				}
				break;
			case 'O':
			{
				int x, y, f;
				x = *(int*)(buf + HEADSIZE + 1);
				y = *(int*)(buf + HEADSIZE + 5);
				f = x + y;
				printf("Ŭ���̾�Ʈ ��û�� ���� %d + %d = %d\n", x, y, f);
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
	printf("%d ���� ����\n", client_sock);
	//printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
	//	inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	RemoveSocket(sockArray, sockCount, 
		FindSocket(sockArray, sockCount, client_sock));


	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) 
		return -1;

	// socket()
	SOCKET listen_sock = socket(
        AF_INET, //�ּ�ü��: ��� ���� ����, ���ͳ� ������ ����ϸ� ����Ʈ ��ǻ�� ������ ����� ���, IPv4
        SOCK_STREAM, //������������: TCP/IP ��� ���
        0 //�� �� ���ڷ� �������� ������ ��Ȯ�ϸ� 0���, IPPROTO_TCP, IPPROTO_UDP
        );
	if(listen_sock == INVALID_SOCKET) err_quit("socket()");	
	
	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET; //�ּ�ü��
	serveraddr.sin_port = htons(9000); //������Ʈ��ȣ
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); //����IP �ּ�
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("bind()");
	
	// listen()
	retval = listen(
        listen_sock, 
        SOMAXCONN //���Ӵ�� ť�� ũ��
        ); // TCP ���¸� LISTENING ����
	if(retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
//	char buf[BUFSIZE+1];

	while(1){
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(
            listen_sock, //��� ����
            (SOCKADDR *)&clientaddr, //Ŭ���̾�Ʈ�� ���� out param
            &addrlen //�ּұ���ü������ũ��, in(ũ������), out(�ʱ�ȭ��ũ���ȯ) param
            ); //��ż��� ����: ���� IP, ���� ��Ʈ ����
		if(client_sock == INVALID_SOCKET){
			err_display("accept()");
			continue;
		}
		AddSocket(sockArray, sockCount, client_sock);
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			inet_ntoa(clientaddr.sin_addr), //���ڿ��� IP�ּ� ��ȯ
            ntohs(clientaddr.sin_port) // ��Ʈ��ȣ network to host
            );

		DWORD id;
		CreateThread(NULL, 0, RWClient,(LPVOID)client_sock, 0, &id);

	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}