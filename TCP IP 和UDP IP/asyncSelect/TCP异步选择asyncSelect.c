//#include <Windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")



//�Զ�����Ϣ
#define UM_ASYNCSELECTMSG WM_USER+1

//�ص���������
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam);

//����һ������ͼ����������������ͷ�socket
#define MAX_SOCK_COUNT 1024			//�Զ���ĺ�
SOCKET g_Socketall[MAX_SOCK_COUNT];
int g_count = 0;

//����һ���ͻ���Y������
int y = 0;

//��1��ʵ�����(����Ҫ�������ڵ�ID) ����2�������ģ������ã�Ϊ�����¼����Բ����ģ���һ�����ڵľ�������Ǵ��������ڣ���Ѳ�1�����ݸ��Ƶ��´��ڣ�
//��3���������д���������4����ʾ��ʽ������򿪳�������С��״̬���֣�
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hpreInstance, LPSTR lpCmdLine, int ShowCmd)
{
	//�������ڽṹ��
	WNDCLASSEX WD;						//�Ƚ��°汾�Ĵ��ڽṹ���Ǵ�EX��
	WD.cbClsExtra = 0;					//���ڽṹ�����Ŀռ䡢��������
	WD.cbSize = sizeof(WNDCLASSEX);		//�ṹ���С
	WD.cbWndExtra = 0;					//���ڶ���Ŀռ䡢��������
	WD.hbrBackground = NULL;			//������ɫ��NULLΪĬ��
	WD.hCursor = NULL;					//��꣬NULLΪĬ��
	WD.hIcon = NULL;					//���Ͻ�ͼ��
	WD.hIconSm = NULL;					//��������ͼ��
	WD.hInstance = hInstance;			//����������1�����
	WD.lpfnWndProc = WinbackProc;		//��ص�����������
	WD.lpszClassName = "Yywindow";		//�������
	WD.lpszMenuName = NULL;		//�˵�������
	WD.style = CS_HREDRAW | CS_VREDRAW;	//���ڷ��,��ֱˢ����ˮƽˢ�£����細�ڴ�С���Ǹı䣬����ҲҪ���ű䣬�����Ժ���˵��

	//ע��ṹ��
	RegisterClassEx(&WD);
	//��������
	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, TEXT("Yywindow"), _T("Yy����"), WS_OVERLAPPEDWINDOW, 200, 200, 400, 300, NULL, NULL, hInstance, WinbackProc);
	if (NULL == hwnd)
	{
		return 0;
	}
	//��ʾ����
	ShowWindow(hwnd, ShowCmd);
	//���´���
	UpdateWindow(hwnd);


	//�������
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA WSsockMsg;				//��סһЩ������Ϣ
	int nres = WSAStartup(WSVersion, &WSsockMsg);
	if (0 != nres)
	{
		switch (nres)
		{
		case WSASYSNOTREADY:
			printf("�ײ�������ϵͳ��δ׼���ý�������ͨ��, ϵͳ�������⣬�����µ��ԣ����ws2_32���Ƿ���ڣ�C�̲���ws2_32��");
			break;
		case WSAVERNOTSUPPORTED:
			printf("Ҫʹ�õİ汾��֧��,����汾�����������");
			break;
		case WSAEPROCLIM:
			printf(" �Ѵﵽ��Windows�׽���ʵ��֧�ֵ��������������ƣ���65536���˿�ȫ�����ˡ��볢�Թرղ���Ҫ�������Ϊ��������ṩ�������Դ");
			break;
		case WSAEINPROGRESS:
			printf(" ��ǰ���������ڼ䣬����ĳЩԭ������������᷵������������롣�������������");
			break;
		case WSAEFAULT:
			printf(" lpWSAData����������Чָ�롣����Ա�Լ������⣬��������WSAStartup�Ĳ���2д����");
			break;
		}
	}
	//У��汾
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		printf("�����汾����");
		WSACleanup();
		return 0;
	}
	//����SOCKET
	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == ServerSocket)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//SOCKET�󶨶˿ں����ַ
	struct sockaddr_in st;
	st.sin_family = AF_INET;
	st.sin_port = htons(22258);
	st.sin_addr.S_un.S_un_b.s_b1 = 127;
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	if (SOCKET_ERROR == bind(ServerSocket, (const struct sockaddr*)&st, sizeof(st)))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}
	//��������
	if (listen(ServerSocket, SOMAXCONN))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}
	//��������socketװ�������У������������ͷ�
	g_Socketall[g_count] = ServerSocket;
	g_count++;

	//��socket����Ϣ�󶨣�Ͷ�ݸ���Ϣ����
	if (SOCKET_ERROR == WSAAsyncSelect(ServerSocket, hwnd, UM_ASYNCSELECTMSG, FD_ACCEPT | FD_WRITE | FD_READ))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}	
	//��Ϣѭ��
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//�ر������
	WSACleanup();


	return 0;
}

//�ص�����
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
{
	//��ȡ������ڵ�HDC���ͻ��������ID������������Ҫ�ͷž��
	HDC hdc = GetDC(hwnd);
	//���ദ��
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:	//������Ϣѭ��������ϢͶ�ݵ����Ȼ���ͨ��case�����ദ����
		//MessageBox(NULL, L"���ź���", TEXT("��ʾ"), MB_YESNO);
		{
			//��ȡSOCKET
			SOCKET sock = (SOCKET)wparam;
			//��ȡ��Ϣ 
			if (0 != HIWORD(lparam))		//������
			{
				if (10053 == HIWORD(lparam))
				{
					TextOut(hdc, 0, y, TEXT("FD_CLOSE"), strlen("FD_CLOSE"));
					y += 15;
					//�ر�socket�ϵ���Ϣ
					if (SOCKET_ERROR == WSAAsyncSelect(sock, hwnd, 0, 0))
					{
						int a = WSAGetLastError();
					}
					//��������ɾ�����socket
					for (int i = 0; i < g_count; i++)
					{
						if (sock == g_Socketall[i])
						{
							g_Socketall[i] = g_Socketall[g_count];
							g_count--;
						}
					}
					//�ͷ�socket
					closesocket(sock);
				}
				break;
			}
				
			//���ദ��
			switch (LOWORD(lparam))			//������		//�յ��˲����룬��ʼ���ദ��
			{
			//�пͻ�������
			case FD_ACCEPT:					
			{
				//�ڴ��ڿͻ�����ӡFD_ACCEPT
				TextOut(hdc, 0, y, "FD_ACCEPT", strlen("FD_ACCEPT"));
				y += 15;
				//�������Ӳ������ͻ���socket
				SOCKET ClientSocket = accept(sock, NULL, NULL);
				if (INVALID_SOCKET == ClientSocket)
				{
					int a = WSAGetLastError();
					break;						
				}
				//�ͻ���socket����Ϣ��Ͷ�ݸ���Ϣ����
				if (SOCKET_ERROR == WSAAsyncSelect(ClientSocket, hwnd, UM_ASYNCSELECTMSG,FD_WRITE | FD_READ | FD_CLOSE))
				{
					int a = WSAGetLastError();
					closesocket(ClientSocket);
					break;
				}
				//���ͻ���socketװ�������У������������ͷ�
				g_Socketall[g_count] = ClientSocket;
				g_count++;
			}	
				break;
			case FD_WRITE:
			{
				TextOut(hdc, 0, y, TEXT("FD_WRITE"), strlen("FD_WRITE"));
				y += 15;
				//������ԶԿͻ��˽���send��
			}
				break;
			case FD_READ:
			{
				TextOut(hdc, 0, y, TEXT("FD_READ"), strlen("FD_READ"));
				y += 15;
				//FD_READ��Ϣ��ʾ�ͻ��˷�������Ϣ
				char buf[1024] = {0};
				if (SOCKET_ERROR == recv(sock, buf, 1023, 0))
				{
					int a = WSAGetLastError();
					break;
				}
				TextOut(hdc, 80, y - 15, buf, strlen(buf));	
			}
				break;
			case FD_CLOSE:
				TextOut(hdc, 0, y, TEXT("FD_CLOSE"), strlen("FD_CLOSE"));
				y += 15;
				//�ر�socket�ϵ���Ϣ
				if (SOCKET_ERROR == WSAAsyncSelect(sock, hwnd, 0, 0))
				{
					int a = WSAGetLastError();
				}
				//��������ɾ�����socket
				for (int i = 0; i < g_count; i++)
				{
					if (sock == g_Socketall[i])
					{
						g_Socketall[i] = g_Socketall[g_count];
						g_count--;
					}
				}
				//�ͷ�socket
				closesocket(sock);
				;
			}
		}
		break;
	case WM_CREATE:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	

	ReleaseDC(hwnd, hdc);

	return DefWindowProc(hwnd, msgID, wparam, lparam);
}


void kejian()
{
	{
		//5.1 �������ڽṹ��
		/*
		* ���ݣ�
		*		ûɶ���ݣ����Ǵ������ڽṹ��
		*		��ϣ����ʱ���Ȳ��ü�ס��Щ���ݣ�̫�����ˣ�
		*		��ʦ˵�첽ѡ��ģ�ͺܼ򵥣��Ҿ���ʱ����ס��Щ���Ա��ɶ��
		*		WD.lpszClassName = L"Yywindow"��һ��L���ַ���ת��������L����ʾ��char * ת��Ϊ wchar_t *��
		*		��һ�ּ�L �ڶ��֣��һ���Ŀ�����ԣ�ѡ���ַ��������Ƽ�����������������������֣�TEXT("") �����֣�_T("")
		* ���ģ���Ϣ����
		*		����ϵͳΪÿһ�����ڴ���һ����Ϣ���в���ά��
		*		��������Ҫʹ����Ϣ���о�Ҫ����һ������
		* 
		* ��1����
		*		�����ǵ�socket��һ����Ϣ�󶨣�Ͷ�ݸ�������ϵͳ
		* ��2����
		*		��Ϣ���ദ�����ж�Ӧ�Ĳ������ͻ������Ϣ
		* ��ʦ˵�ܼ򵥣�
		*		����ģ��ֻ������windows
		*		�������ǿ���ѧ�����ִ���˼��
		*
		*/
		//5.2 win32��������
		/*
		*	�������ڽṹ������EX�汾����ע��ṹ�壬�������ڶ�Ҫ��EX�汾��������ʹ�õġ�
		*	ʵ���������ǵ�Ӧ�ó��򣬿��ɿ�����Ŀ��
		*	ʵ����ʵ���ľ����ID�����ṹ���нṹ��ľ���������д��ڵľ������ˢҲ�л�ˢ�ľ��
		*	ͨ�������ID������ʶ���ҵ����ǣ�HWND���Ǵ��ڵľ������
		* 
		* ע��ṹ��: RegisterClassEx
		*		�����ô��ڽṹ�壬Ҫ�ڲ���ϵͳ��ע�����ʹ�ã������ò���ϵͳ�Ͽ�ʶ����
		* 
		* ��������: CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, TEXT("Yywindow"), _T("Yy����"), WS_OVERLAPPEDWINDOW, 200, 200, 400, 300, NULL, NULL, hInstance, NULL)
		*		��1��������񡢲�2���ṹ��������3������������4�������񣨿����ð�λ�� | �����ö��ַ��
		*		��5��6����ʼ���꣨�������Ͻǵ����꣩����7��8�����ڵĿ���
		*		��9�������ڡ���10���˵������þ���NULL������11������ʵ���ľ������12���ص�����
		* 
		* ��ʾ���ڣ�ShowWindow(hwnd, ShowCmd);
		*		��1�������Ĵ��ڡ���2����ʾ��ʽ���������������һ��������
		* 
		* ���´��ڣ�
		*		����Qt��Ļ洰�ڣ�����Ϊ���Ǵ����Ĵ���
		* 
		* ��Ϣѭ����
		*		GetMessage(&msg, NULL, 0, 0)��
		*			��1����Ϣ�ṹ�塢��2����Ҫ���յ���Ϣ�Ķ�Ӧ���ڣ���NULL����ȫ�����ڶ��ܽ��յ���
		*			��3��4�����յķ�Χ����0���������з�Χ��
		*		��Ϣѭ���𵽷���ַ������ã�����Qt��Event
		*		�رմ��ڷ���0
		* 
		* �ص�������
		*		WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
		*			��1�����ڡ���2����Ϣ��ID����3���õ�����Ϣ���ĸ�socket�����ģ�ͨ�������������socket���ص������У�����4������һ������������
		*			��4���зָ�λ���λ����λHIWORDװ�����룬��λLOWORDװ��������FD_ACCEPT��
		*		�̶���ʽ��д��
		*		�����ǰ������߲������ź�ת����ϵͳ��ʶ��Ĵ��룬���к����Ĳ���
		*		
		*/
		//5.3 WSAAsyncSelect������ʹ��
		/*
		* ��ʼ���������첽ѡ��ģ�ͣ�
		*		������⵽���������Ĳ�����ֻҪ�ŵ���Ϣѭ��֮ǰ�Ϳ����ˣ���ֻ����һ��
		*		����ͷ�ļ�������⣩��windows.h�а����뱻������ϵ������������һ���Ϳ�����
		* 
		* WSAAsyncSelect��
		*		���¼���socket��Ͷ�ݳ�ȥ
		*		
		*	���ܣ���socket����Ϣ����һ�𣬲�Ͷ�ݸ�����ϵͳ
		*	
		*	����1��socket
		*	����2�����ھ����Ҫ�󶨵��ĸ������ϣ���д�Ǹ����ڵľ����ID,���ڵı�ţ�
		*	����3����Ϣ��ţ��Զ������Ϣ�������ʾ���һ������
		*	����4����Ϣ�����ͣ���������֮ǰģ�͵Ĵ����룬��FD_WRITE�ȣ�
		*	����ֵ���ɹ�����0  ʧ�ܷ���SOCKET_ERROR
		*	��һ���������һ���꣬������ͷ�ļ���ǰ��ͺ��� #define _WINSOCK_DEPRECATED_NO_WARNINGS
		* 
		* �����߼���
		*		���ǰ�socket����Ϣ����һ��ͨ������������װ��һ����Ϣ�����У����ǿ�������
		*		Ȼ����Ϣѭ����GetMessage���������δ���Ϣ��������������Ϣ
		*		ͨ��ѭ���е����������ַ���Ȼ��ַ����˻ص�������
		*		�ص��������յ�����Ϣ��ͨ��caseȥ���ദ��
		* 
		* �ص�������
		*		WM_CREATE��Ϣ�ǳ�ʼ����ִֻ��һ�Σ�����ִֻ��һ�εĶ��ǳ�ʼ������
		*		�������������ģ�͵�ͨ�ýṹ�����������~�������������Էŵ��ص�������
		*		W:windows M ��Message��Ϣ
		*		WM_����ϵͳ�Ѿ�����õ���Ϣ
		*		������Ϣѭ���������ǵ��Զ�����ϢͶ�ݵ��ص�������Ȼ�����ͨ��case�����ദ����
		*		����Ϣ���������ص������о��ܲ�����
		* 
		* �Զ�����Ϣ��
		*		#define UM_ASYNCSELECTMSG WM_USER+1
		*		WM_USER��ϵͳ�ṩ�������Զ�����Ϣ��
		*		��Ϊ��Ϣ����һ�������������Զ������Ϣ����ֵ������ϵͳ�Ѿ�����õ���Ϣ�ظ�
		*		�Ǳ�WM_USER��������ֵС�� ����ϵͳ�Ѿ�������˵ģ����ǲ�����
		*		������WM_USER���� ����������ʹ���˵�
		*		UM_�����������Զ����
		* 
		* MessageBox(NULL, L"���ź���", TEXT("��ʾ"), MB_OK);
		*		һ����ʾ���ڣ�
		*		��1��ָ�������ڣ�Ҳ���Բ�ָ��
		*		��2�����ڵ�����
		*		��3�����ڱ���
		*		��4�������ʾ���ڵķ��MB_OK����һ����ť��MB_YESNO ��������ť
		*/
		//5.4 ��Ϣ���ദ��1
		/*
		* �ص�������
		*		����4�зָ�λ���λ����λHIWORDװ�����룬��λLOWORDװ��������FD_ACCEPT��
		*		�����λΪ0 ��˵��û�г�����Ϊ0��˵��������
		*		�ص�����һ��ֻ����һ����Ϣ
		* 
		*		switch�У�����ֱ����������������������{}����������
		*	
		* ��ڿε����ݣ�
		*		�ص������У����з��ദ��ͨ������3��socket�����ص�����
		*		��FD_ACCEPT�Ĵ����У����ǽ������ӣ����ͻ���socket����Ϣ��Ͷ�ݸ���Ϣ����
		*		�Զ���һ������ͼ�����Ϊ�˼�¼socket�����������ͷ�
		*		ͨ������3��ȡ�����ݽ��ص�������socket������4��ȡ��������ʹ�����
		*	�ص������ڴ������break���ɣ����ùر�����⣨С�²��ùرգ�
		* 
		* �¼����ദ���߼���
		*		����Ҫ�㣺�ͻ���socket�������Ĵ����롢�������Ϣ����
		*/
		//5.5 ��Ϣ���ദ��2
		/*
		* 1.�ͻ�����ӡ�����⡢�ͷž��������
		*		TextOut:�ڿͻ�����ӡ����
		*				�ͻ������ǿͻ��������ڱ������϶������Ƿǿͻ���������ʾ���ݵľ�������пͻ���
		*				HDC���ǿͻ����ľ���������кܶ࣬����ֽ�кܶ࣬Ҫͨ��һ��IDȥ�����ҵ�����ֽ
		*			��1���ͻ���HDC ��2��3����ӡ���ݵ���ʼ���꣨�ͻ�����ԭ�������Ͻ�0��0��
		*			��4����ӡ�����ݡ���5����ӡ���ٸ��ַ�����strlen�����ʣ�sizeof�Ļ����\0Ҳ��ӡ
		*		HDC�����������ѭ�������ͷŵ��������´ν���ص�����ʹ�ã���Ϊÿ��ѭ��ִֻ��һ��
		* 2.FD_WRITE����Ϣ�Ĵ���
		*		�����ٴ�ӡʱ����ӡ������λ��Ҫ�����䶯��
		*		���Զ���һ��ȫ�ֱ������������Y��������б仯
		*		FD_READ�����У���������TextOut��ӡ�յ����ַ�����������ʾ����������: �ӡ�char [1024]������LPCWSTR�������Ͳ�����
		*		�����һ���Ŀ-����-�ַ��� ��Ϊ���ֽ��ַ����ͺ��ˣ�����Ҫ��֮ǰ��L TEXT("") ��ȥ��
		*		FD_ACCEPT��FD_WRITE��Ϣ������˳����֮ǰѧ��ģ��һ���ģ�������FD_ACCEPT�����Ӻ���������FD_WRITE
		* 
		*/
		//5.6 ��Ϣ���ദ��3 FD_CLOSE����
		/*
		*		֮ǰ����Ҳ֪���������������߻���ǿ�����ߣ�FD_CLOSE���д�����10053����Ϊ��WSAECONNABORTED��
		*		��������ҲҪʶ��������룬Ȼ���������3������
		*		������FD_CLOSEû�д��������������Ҳ��������3����������ΪMSDN�϶�������������������
		*		�����������
		* 
		* 1.�ر�socket�ϵ���Ϣ
		*		�ر���Ϣ����Ϊ�ͻ������ߣ�����������Ҫȡ�����socket����Ϣ
		*		�ò���ϵͳ�����ټ����
		*		�رյķ�ʽ����WSAAsyncSelect��Ͷ�ݺ������������������ó�0���͸��ǵ�ԭ�������ݣ��͹ر���
		* 2.��¼������ɾ����socket
		*		��ѭ���ҳ����socket��λ�ã�Ȼ����ĩβԪ�ؽ���
		*		Ȼ�����-1����
		* 3.�ͷ����socket
		*		closesocket();���ͷ�
		*/
		//5.7 �첽ѡ��ģ������������Ż�
		/*
		* ��ʱ49 ��ʦ�л�һ������ͼ�����ǿ��ԶԱ�һ��֮ǰ������ģ�͵�����ͼ��ѧϰ���
		* 
		* ����������
		*		ִ�д���һ����Ϣ��ʱ�����޷�ִ����������Ϣ��
		*		�����û�����Ҫ̫��
		*	���������
		*		���ö��̣߳������ֶ�����ÿ���������64���û�
		*		Ȼ��ÿ�����ڷ���1���̼߳��ɽ��
		* 
		* СBUG��
		*		���統������ִ�д���1�Ĳ�����ʱ��3�ͻ������������˶�����Ϣ
		*		�Ǵ�����1����ȥ����3��ʱ��recv��ȡ�Ὣ�������Ϣһ�������
		*		��Ϊ�������Ϣ������Э�黺���������ˣ��¼�ѡ��ģ��Ҳ���������
		*	���������
		*		Ҳ�ǲ��ö��߳̾��ܽ��
		*		���߳������ڻ�û��ѧϰ������Ҫ�������ֽ��˼·
		*/
	}
}