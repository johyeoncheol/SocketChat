#pragma warning(disable:4996)
#include <winsock2.h>
#include <windows.h>
#include "resource.h"

#define BUFSIZE 512 //HEADSIZE+DATASIZE
#define HEADSIZE 12
#define DATASIZE 500

//1-1. ����
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT iMsg,
	WPARAM wParam, LPARAM lParam);
int recvn(SOCKET s, char *buf, int len, int flags);
SOCKET ConnectServer();
DWORD WINAPI ReadServer(LPVOID param);

////////////////  �������� ���� ////////////////
SOCKET sock;
HWND hList;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine, int nCmdShow)
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return -1;

	sock = ConnectServer();
	if (sock != NULL)
	{
		DWORD id;
		CreateThread(NULL, 0, ReadServer, (LPVOID)sock, 0, &id);
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	}

	// ���� ����
	WSACleanup();
	return 0;
}

//1-2. ����
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT iMsg,
	WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
		//1-1. ���̾�α� �����
	case WM_INITDIALOG:
		hList = GetDlgItem(hwnd, IDC_LIST);

		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;

			//2-1. ����Ʈ�ڽ��� �о
		case IDC_SEND:
			{
				char sendBuf[BUFSIZE];
				*(int*)sendBuf = 500;	//����
				*(int*)(sendBuf + 4) = 'S'; //����
				*(int*)(sendBuf + 8) = 0; //����
				GetDlgItemText(hwnd, IDC_EDIT, sendBuf + HEADSIZE, 500);
				SetDlgItemText(hwnd, IDC_EDIT, "");

				//3-1. �������� �������ٴ� �����Ͽ�
				send(sock, sendBuf, BUFSIZE, 0);
				char msg[600];
				wsprintf(msg, "���� ������ : %s", sendBuf + HEADSIZE);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			break;
		}
		break;

	}
	return 0;
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

SOCKET ConnectServer()
{
	int retval = 0;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) return NULL;

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(
		sock, //�����ڵ�
		(SOCKADDR *)&serveraddr, //���� ���� �ּҰ�
		sizeof(serveraddr) //�ּҰ� ũ��
	); // ������ ���� ��û(�����ϸ� �ڵ����� ������Ʈ, �����ּҸ� �Ҵ�)
	if (retval == SOCKET_ERROR) return NULL;

	if (retval == SOCKET_ERROR)
		return NULL;
	else
		return sock;

}

DWORD WINAPI ReadServer(LPVOID param)
{
	while (1)
	{
		char recvBuf[BUFSIZE];
		int retval = recvn(sock, recvBuf, BUFSIZE, 0);
		if (retval <= 0)
			break;

		switch (*(int*)(recvBuf + 4))
		{
		case 'S':
			{
				char msg[500];
				wsprintf(msg, "���� ������ : %s", recvBuf+HEADSIZE);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
				break;
			}
		default:
			break;
		}
	}
	closesocket(sock);

	return 0;
}