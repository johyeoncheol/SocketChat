//���ķ� �ٷ�� ��κ��� �ڵ�� ITCOOKBOOK �������Ʈ��ũ���α׷��� �輱���� �ҽ����� ������ ����մϴ�.
#pragma warning(disable:4996)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <Windows.h>
#include <vector>
using namespace std;

#define BUFSIZE 512 //HEADSIZE+DATASIZE
#define HEADSIZE 12
#define DATASIZE 500

struct SocketInfo
{
	SOCKET sock;
	char clientIP[50];
	unsigned short clientPort;
};
struct Room
{
	int roomNum;
	vector<SocketInfo> sockList;
};
////////////////////////////////////////////
SocketInfo sockArray[100];
int sockCount=0;
vector<Room> roomGroup;
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
void PrintLogRoomGroup(vector<Room>& roomGroup)
{
	printf(" =================== \n");
	printf(" Ŭ���̾�Ʈ ���� ���� : \n");
	vector<SocketInfo>& sockList = roomGroup[0].sockList;
	for (unsigned i = 0; i < sockList.size(); ++i)
	{
		printf("\n[TCP ����[%d]: Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			i,sockList[i].clientIP, sockList[i].clientPort);
	}
	///////////////////////////////////
	////////////ä�� �濡 ���� ����
	printf("  ä�� ���� ����  : \n");
	printf("==========================\n");
}
void AddSocket(vector<Room>& roomGroup, SOCKET sock)
{
	SOCKADDR_IN sockin;
	int socklen = sizeof(sockin);
	char ip[50];
	unsigned short port;
	getpeername(sock, (SOCKADDR*)& sockin, &socklen);
	strcpy_s(ip, 50, inet_ntoa(sockin.sin_addr));
	port = ntohs(sockin.sin_port);

	SocketInfo si;
	si.sock = sock; strcpy_s(si.clientIP, 50, ip); si.clientPort = port;
	roomGroup[0].sockList.push_back(si);
	PrintLogRoomGroup(roomGroup);
}
void RemoveSocket(vector<Room>& roomGroup, int idx)
{
	roomGroup[0].sockList.erase(idx + roomGroup[0].sockList.begin());
	PrintLogRoomGroup(roomGroup);
}
int FindSocket(vector<SocketInfo>& sockList, SOCKET sock)
{
	for (unsigned i = 0; i < sockList.size(); ++i)
		if (sockList[i].sock == sock)
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
			case 'L':
			{
				vector<SocketInfo>& sockList = roomGroup[0].sockList;
				char* ptr = buf + 12;
				for (unsigned i = 0; i < sockList.size(); ++i, ptr+=sizeof(SocketInfo))
					memcpy(ptr, &sockList[i], sizeof(SocketInfo));
				*(int*)(buf+8) = sockList.size();
				send(client_sock, buf, BUFSIZE, 0);
			}
				break;
			case 'S':
			{
				printf("Ŭ���̾�Ʈ ���� ������ :: %s\n", buf + HEADSIZE);
				vector<SocketInfo> & sockList = roomGroup[0].sockList;
				for (unsigned i = 0; i < sockList.size(); ++i)
				{
					SOCKET sock = sockList[i].sock;
					char sendBuf[BUFSIZE];

					int idx = FindSocket(sockList, client_sock);
					memcpy(sendBuf, buf, HEADSIZE);
					wsprintf(sendBuf + HEADSIZE, "[%s,%d]:",
						sockList[idx].clientIP, sockList[idx].clientPort);
					wsprintf(sendBuf + HEADSIZE + 30, "%s", buf + HEADSIZE);
					send(sock, sendBuf, BUFSIZE, 0);
				}
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
	RemoveSocket(roomGroup, 
		FindSocket(roomGroup[0].sockList, client_sock));


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

	//0 �� ���� Ŭ���̾�Ʈ ���� ��� ������ ���� 
	Room r = { 0, vector<SocketInfo>() };
	roomGroup.push_back(r);

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
		AddSocket(roomGroup, client_sock);		

		DWORD id;
		CreateThread(NULL, 0, RWClient,(LPVOID)client_sock, 0, &id);

	}

	// closesocket()
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}