#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//�Զ�����Ϣ���
#define MY_MSG_ID WM_USER+1
//������socket
SOCKET SerVerSocket;
//�ص�����
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam);
//����ÿ�δ�ӡ�ļ��
int n;

//��1��ʵ�����(����Ҫ�������ڵ�ID) ����2�������ģ������ã�Ϊ�����¼����Բ����ģ���һ�����ڵľ�������Ǵ��������ڣ���Ѳ�1�����ݸ��Ƶ��´��ڣ�
//��3���������д���������4����ʾ��ʽ������򿪳�������С��״̬���֣�
//ת����WINAPI ������CALLBACK��һ������ __stdcall�ĺ�
int WINAPI WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hprehinstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	//1.�������ڽṹ�� WNDCLASSEX
	WNDCLASSEX woYy;					//���ڽṹ��
	woYy.cbClsExtra = 0;				//�ṹ����չ��С������չ���ṹ�����Ŀռ�
	woYy.cbSize = sizeof(WNDCLASSEX);	//�ṹ���С
	woYy.cbWndExtra = 0;				//������չ��С�����ڶ���Ŀռ�
	woYy.hbrBackground = NULL;			//������ɫ��NULLΪĬ��
	woYy.hCursor = NULL;				//�����ʽ
	woYy.hIcon = NULL;					//���Ͻ�ͼ��
	woYy.hIconSm = NULL;				//������ͼ��
	woYy.hInstance = hinstance;			//����������1��������ľ��
	woYy.lpfnWndProc = WinbackProc;		//�ص�����
	woYy.lpszClassName = "Yyasyncselect";//����ࣨ�ṹ�壩������
	woYy.lpszMenuName = NULL;			//�˵�������
	woYy.style = CS_HREDRAW | CS_VREDRAW;//���ڷ��ˮƽˢ���봹ֱˢ�£���������ˮƽ�仯ʱ��Ҫ�ػ洰�ڣ���ֱҲһ����
	//2.ע�ᴰ�ڽṹ�� RegisterClassEx
	RegisterClassEx(&woYy);				//ע����ϵͳ����֪����
	//3.��������		CreateWindowEx
	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "Yyasyncselect", "Yy����", WS_OVERLAPPEDWINDOW, 200, 200, 700, 500, NULL, NULL, hinstance, NULL);
	//4.��ʾ����		ShowWindow
	ShowWindow(hwnd, nShowCmd);
	//5.���´���
	UpdateWindow(hwnd);

	//����ͨ�Ŵ���
	//�������
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nrse = WSAStartup(WSVersion, &wsaData);
	if (0 != nrse)
	{
		int a = WSAGetLastError();
		printf("WSAStratup error: %d\n", a);
		CloseHandle(hwnd);
		system("pause>0");
		return 0;
	}
	//У��汾
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("�����汾����ȷ error: %d\n", a);
		CloseHandle(hwnd);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//����SOCKET ��UDP�ģ��������ŵ�ȫ�֣��������˳��������ã�
	SerVerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		printf("����SOCKET error: %d\n", a);
		CloseHandle(hwnd);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//�󶨶˿ں����ַ
	struct sockaddr_in sd;
	sd.sin_family = AF_INET;
	sd.sin_port = htons(22258);
	sd.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (SOCKET_ERROR == bind(SerVerSocket, (SOCKADDR*)&sd, sizeof(sd)))
	{
		int a = WSAGetLastError();
		printf("bind error: %d\n", a);
		CloseHandle(hwnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//����Ϣ��socket����Ͷ�ݸ�����ϵͳ
	int wsaasync = WSAAsyncSelect(SerVerSocket, hwnd, MY_MSG_ID, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaasync)
	{
		int a = WSAGetLastError();
		printf("����Ϣ���� error: %d\n", a);
		CloseHandle(hwnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//6.��Ϣѭ��		ѭ���в��ϴ���Ϣ����ȡ��Ϣ����GetMessage��ȡ��Ϣ���رմ��ڷ���0
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))//��1����Ϣ�ṹ�塢��2����Ҫ���յ���Ϣ�Ķ�Ӧ���ڣ���NULL����ȫ�����ڶ��ܽ��յ�����3��4�����յķ�Χ����0���������з�Χ��
	{
		TranslateMessage(&msg);//ȡ����Ϣ�����Ϣ���з���
		DispatchMessage(&msg); //����ַ����ص��������ڴ˺����б�����
	}
		

	//�ͷ������
	//CloseHandle(hwnd);
	//ReleaseDC(hwnd);
	closesocket(SerVerSocket);
	WSACleanup();
	
	return 0;
}

//6.�ص�����		����4��ĩβ�������봴�����ں�����ĩβ��������ͨ�ģ�����Ǳ���д�ص���������Ὣ���ݴ��͵�����
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
{	//�ص�������DispatchMessage�б�����ִ��
	switch (msgID)
	{
	case MY_MSG_ID:
	{
		//��ȡsocket
		SOCKET sock = (SOCKET)wparam;
	
		//lparam�ĸ�λ��Ŵ����룬Ϊ0��û�д���
		if (0 == HIWORD(lparam))
		{
			//��ȡ�����Ϣ���ڴ��ڵĿͻ��������ID������������Ҫ�ͷţ������Ҫ�ͷ�
			HDC hdc = GetDC(hwnd);
			//��Ϣ���ͷ��ദ��
			if (FD_WRITE == LOWORD(lparam))
			{
				TextOut(hdc, 0, n, "FD_WRITE", strlen("FD_WRITE"));
				n += 20;
			}
			if (FD_READ == LOWORD(lparam))
			{	//lparam�ĵ�λ��ž�����Ϣ����
				
				struct sockaddr saClient;
				int nlen = sizeof(saClient);
				char strbuf[548] = { 0 };
				int num = recvfrom(sock, strbuf, 548, 0, &saClient, &nlen);
				if (SOCKET_ERROR == num)
				{
					int a = WSAGetLastError();
					//��������a,��itoaת�����ַ�����Ȼ�����str�У��ٴ�ӡ�������ϣ�����3��10 ��ʾ10���Ƶ�ת��
					char str[20] = { 0 };
					TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
					n += 20;
				}
				TextOut(hdc, 0, n, strbuf, strlen(strbuf));
				n += 20;
				//sendto������Ϣ
				int snum = sendto(sock, "Server �յ�\n", strlen("Server �յ�\n"), 0, (const struct sockaddr*)&saClient, sizeof(saClient));
				if (SOCKET_ERROR == snum)
				{
					int a = WSAGetLastError();
					char str[20] = { 0 };
					TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
					n += 20;
				}
			}
			//�ͷ�HDC���ͻ��������,�����´ν���ص�����ʹ�ã���Ϊÿ��ִֻ��һ��
			ReleaseDC(hwnd, hdc);
		}	
	}
		break;
	case WM_DESTROY:			//��������ڵ��������֮ǰTCPû��
		PostQuitMessage(0);		//��������������ǵ����͹رճ��������,�����˳���Ϣѭ��
		break;
	}
	//��һ����Ҫ���ϣ����򴰿��޷���ʾ
	return DefWindowProc(hwnd, msgID, wparam, lparam);
}




//ѧϰ�ʼ�
void studybook14()
{
	//5.1 �Խ�����������
	{
		/*
			Win32�첽ѡ��ģ���Խ��������̣�
				1.�������ڽṹ�� WNDCLASSEX
				2.ע�ᴰ�ڽṹ�� RegisterClassEx
				3.��������		CreateWindowEx
				4.��ʾ����		ShowWindow
				5.��Ϣѭ��		GetMessage��TranslateMessage��DispatchMessage
				6.�ص�����

			��������
				��1��ʵ�����(����Ҫ�������ڵ�ID) ����2�������ģ������ã�Ϊ�����¼����Բ����ģ���һ�����ڵľ�������Ǵ��������ڣ���Ѳ�1�����ݸ��Ƶ��´��ڣ�
				��3���������д���������4����ʾ��ʽ������򿪳�������С��״̬���֣�
				ת����WINAPI ������CALLBACK��һ������ __stdcall�ĺ�

				WinMain��Сд�ϸ����ײ㱾�ʻ���main�����ǵ���main��ʱ�򴫵���һЩ�����������WinMain����ʾ

			ʵ�������
				���������ܿ����Ķ��о����ID��

		
		*/
	}
	//5.2 �Խ�����
	{
		/*
			�ַ��������⣺
				���ַ�ʽ��
					1.��Ŀ-����-�ַ��� Ȼ���޸�
					2.�ַ���ǰ�� L 
					3.�ַ���ǰ�� _T  ��Ҫͷ�ļ� tchar.h
					4.�ַ���ǰ�� TEXT

			�ܽ᣺

				�ص�������Ҫ�ӵ���˳����������ǹرմ��ڣ����ǳ�����Ȼ������
				��Ϣѭ���Ǵ���Ϣ��������Ϣ��Ȼ��ַ�
				�������ڵĲ���1���Ƿ�񣬿��Զ������ð�λ�� | �������Ϳ���
				����	C28251	��WinMain������ע��һ��: ��ʵ������ ����ע��������������ǰ��_In_ �ڶ�����һ�������ǻ���û���


				�����Ĳ���̫���ˣ����鿴TCP���첽ѡ��ıʼ�
		*/
	};
	//5.3 �첽ѡ��ģ�ͷ��ദ��
	{
		/*
			�Ͻڿ�֪ʶ�㲹�䣺
				1.ѭ���в��ϴ���Ϣ����ȡ��Ϣ����GetMessage��ȡ��Ϣ���رմ��ڷ���0
				2.TranslateMessage������ȡ����Ϣ�����Ϣ���з���
				3.�ص�������DispatchMessage�б�����ִ��
				4.windows.h �� winsock2.h �г�ͻ��ɾ��windows.h

			���ڿ�֪ʶ�㣺
				1.�����Ǵ����õĴ��ڽṹ�����UDP��ͨ�Ŵ��룬��ӵ�λ������Ϣѭ��ǰ���������ں󣨲������ھ����
				2.����Ϣ���¼�ʱ��ϵͳռ�õķ�Χ��WM_USER,����Ҫʹ�ã���WM_USER+1 WM_USER�Ժ�������ǳ���Աʹ�õķ�Χ��
				3.�ص������Ĳ���WPARAM wparam�����������ղ�����Ϣ���¼���SOCKET
				4.�ص������Ĳ���LPARAM lparam����Ϊ�ߵ�λ����λ�洢�����룬��λ�洢��Ӧ����Ϣ���࣬��FD_READ�ȣ�
					��HIWORD �� LOWORD��ȡ����ΪLPARAM��int��4�ֽڣ�֮ǰѧ��HIBYTE��Ӧshort 2�ֽ�
				5.���飺���������ŵ� == �ĺ��棬��ֹ����
				6.printf�ǿ���̨��ӡ��TextOut�Ǵ��ڴ�ӡ
				7.�������к󣬲�������Ϣѭ���л�ȡ��Ϣ�������ǹرմ���ʱ���ͻ��˳�ѭ������������Ҫ����������β�ͷž����socket���ر������
				8.�����зֿͻ��������Ǵ�ӡ�ľ������򣩡��ǿͻ����ȣ��Ҳ�ͬ������������С���������Լ��ľ����ID����ID��ϵͳ֪��˭��˭�����ĸ�ID�ϲ���
				9.GetDC��ȡ�����DeleaseDC�ͷž������ȡ�˾ͱ����ͷţ�����������
				10.��ǩ�У�switch���в����������������������1.switch����{}����{}������������2.��switch�⣨�Ϸ�����������
				11._itoa(a, str, 10);�����������������a int���� 10����ת�����ַ�����ŵ�str�ַ���������
				12.TextOut ���ڴ�ӡ�Ὣ�ַ����Ľ�β��\0Ҳ��ӡ����������ĩβ������sizeof(a-1),����strlen����
				13.�����ID�������У���WRITE��REDA

			���¼���socket����Ͷ�ݳ�ȥ:
				����ԭ�ͣ�
					int WSAAsyncSelect(
						  SOCKET s,
						  HWND   hWnd,
						  u_int  wMsg,
						  long   lEvent
						);
				���ܣ�
					��socket����Ϣ����һ�𣬲�Ͷ�ݸ�����ϵͳ����
				����1��
					socket
				����2��
					���ھ����Ҫ�󶨵��ĸ������ϣ�
				����3��
					��Ϣ��ţ�ϵͳռ�õķ�Χ��WM_USER,����Ҫʹ�ã���WM_USER+1 WM_USER�Ժ�������ǳ���Աʹ�õķ�Χ��
					�����Զ���һ����(MY_MSG_ID)����¼����Ҫ�õı��
				����4��
					��Ϣ���ͣ���FD_READ�ȣ����WSASelectEventһģһ�������鿴�μ���
				����ֵ��
					�ɹ�������0
					ʧ�ܣ�����SOCKET_ERROR
		
		*/
	};
}
