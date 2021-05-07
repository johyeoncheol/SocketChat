#pragma warning(disable:4996)
#include <winsock2.h>
#include <windows.h>
#include "resource.h"

#define BUFSIZE 512 //HEADSIZE+DATASIZE
#define HEADSIZE 12
#define DATASIZE 500

struct SocketInfo
{
	SOCKET sock;
	char clientIP[50];
	unsigned short clientPort;
};

//1-1. ����
INT_PTR CALLBACK DlgProc(HWND hwnd, UINT iMsg,
	WPARAM wParam, LPARAM lParam);
int recvn(SOCKET s, char *buf, int len, int flags);
SOCKET ConnectServer();
DWORD WINAPI ReadServer(LPVOID param);
void CheckRadioOp(HWND hwnd, int opType);

////////////////  �������� ���� ////////////////
SOCKET sock;
HWND hDlg;

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
	static HWND hList;
	static int opType = 1; //{0:Add, 1:Sub, 2: Mul, 3: Div};
	switch (iMsg)
	{
		//1-1. ���̾�α� �����
	case WM_INITDIALOG:
	{
		hDlg = hwnd;
		hList = GetDlgItem(hwnd, IDC_LIST);

		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)"AAA");
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)"BBB");
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)"CCC");
		///////////////////////////////////////////////
		SetDlgItemText(hwnd, IDC_X, "0");
		SetDlgItemText(hwnd, IDC_Y, "0");
		SetDlgItemText(hwnd, IDC_F, "0");

		CheckRadioButton(hwnd, IDC_RADIO1, IDC_RADIO4, IDC_RADIO1 + opType);
		CheckRadioOp(hwnd, opType);

		char buf[BUFSIZE];
		*(int*)(buf + 4) = 'L';
		send(sock, buf, BUFSIZE, 0);
	}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;

		case IDC_ROOMJOIN:
		{
			HWND hList = GetDlgItem(hDlg, IDC_LISTROOMINFO);
			int idx = SendMessage(hList, LB_GETCURSEL, 0, 0);
			if (idx >= 0)
			{
				char buf[BUFSIZE];
				*(int*)(buf + 4) = 'J';
				*(int*)(buf + HEADSIZE) = idx+1;
				send(sock, buf, BUFSIZE, 0);
			}
		}
		break;
		case IDC_REFRESH:
		{
			char buf[BUFSIZE];
			*(int*)(buf + 4) = 'L';
			send(sock, buf, BUFSIZE, 0);
		}
		break;
		case IDC_ROOMREFRESH:
		{
			char buf[BUFSIZE];
			*(int*)(buf + 4) = 'R';
			send(sock, buf, BUFSIZE, 0);
		}
		break;
		case IDC_ROOMMAKE:
		{
			char buf[BUFSIZE];
			*(int*)(buf + 4) = 'M';
			send(sock, buf, BUFSIZE, 0);
		}
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
		case IDC_OPERATOR:
			{
			int x, y;
			x = GetDlgItemInt(hwnd, IDC_X, NULL, TRUE);
			y = GetDlgItemInt(hwnd, IDC_Y, NULL, TRUE);

			char sendBuf[BUFSIZE];
			*(int*)sendBuf = 500;	//����
			*(int*)(sendBuf + 4) = 'O'; //����
			*(int*)(sendBuf + 8) = 0; //����

			*(char*)(sendBuf + HEADSIZE) = 'A';
			*(int*)(sendBuf + HEADSIZE + 1) = x;
			*(int*)(sendBuf + HEADSIZE + 5) = y;

			send(sock, sendBuf, BUFSIZE, 0);
			}
			break;
		case IDC_RADIO1:
			opType = 0;
			CheckRadioOp(hwnd, opType);
			break;
		case IDC_RADIO2:
			opType = 1;
			CheckRadioOp(hwnd, opType);
			break;
		case IDC_RADIO3:
			opType = 2;
			CheckRadioOp(hwnd, opType);
			break;
		case IDC_RADIO4:
			opType = 3;
			CheckRadioOp(hwnd, opType);
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
		case 'J':
		{
			int roomLen = *(int*)(recvBuf + HEADSIZE);
			char msg[200];
			wsprintf(msg, "room : %d ����", roomLen);
			SetDlgItemText(hDlg, IDC_SOCKINFO, msg);
		}
			break;
		case 'L':
		{
			int clientLen = *(int*)(recvBuf + 8);
			char* ptr = recvBuf + HEADSIZE;

			HWND hList = GetDlgItem(hDlg, IDC_LISTSOCKINFO);
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			SetDlgItemText(hDlg, IDC_SOCKINFO, "��� ���� ����");
			for (int i = 0; i < clientLen; ++i, ptr+=sizeof(SocketInfo))
			{
				SocketInfo sockInfo;
				memcpy(&sockInfo,ptr, sizeof(SocketInfo));
				char msg[200];
				wsprintf(msg, "ip:%s, port:%d",
					sockInfo.clientIP, sockInfo.clientPort);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
			}				
		}
			break;
		case 'R':
		{
			int roomLen = *(int*)(recvBuf + HEADSIZE);
			HWND hList = GetDlgItem(hDlg, IDC_LISTROOMINFO);
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			for (int i = 1; i <= roomLen; ++i)
			{
				char msg[200];
				wsprintf(msg, "room : %d", i);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
		}
		break;
		case 'M':
		{
			int roomLen = *(int*)(recvBuf + HEADSIZE);
			char msg[200];
			wsprintf(msg, "room : %d ����", roomLen);
			SetDlgItemText(hDlg, IDC_SOCKINFO, msg);
			
			HWND hList = GetDlgItem(hDlg, IDC_LISTROOMINFO);
			SendMessage(hList, LB_RESETCONTENT, 0, 0);
			for (int i = 1; i <= roomLen; ++i)
			{
				char msg[200];
				wsprintf(msg, "room : %d", i);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
			}
		}
			break;
		case 'S':
			{
				char msg[500];

				wsprintf(msg, "���� ������ : %s%s",
					recvBuf+HEADSIZE,
					recvBuf+HEADSIZE+30);
				HWND hList = GetDlgItem(hDlg, IDC_LIST);
				SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)msg);
				break;
			}
		case 'O':
		{
			int f = *(int*)(recvBuf+HEADSIZE);
			SetDlgItemInt(hDlg, IDC_F, f, TRUE);
		}
			break;
		default:
			break;
		}
	}
	closesocket(sock);

	return 0;
}
void CheckRadioOp(HWND hwnd, int opType)
{
	switch (opType)
	{
	case 0:
		SetDlgItemText(hwnd, IDC_DISPLAY, "Add");
		break;
	case 1:
		SetDlgItemText(hwnd, IDC_DISPLAY, "Sub");
		break;
	case 2:
		SetDlgItemText(hwnd, IDC_DISPLAY, "Mul");
		break;
	case 3:
		SetDlgItemText(hwnd, IDC_DISPLAY, "Div");
		break;
	}
}