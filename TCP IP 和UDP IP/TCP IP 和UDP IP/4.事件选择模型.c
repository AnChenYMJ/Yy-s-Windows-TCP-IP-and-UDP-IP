#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

//�Զ���ṹ�壬���ڼ�¼Ҫ���󶨵��¼���socket
struct FD_se_set
{
	unsigned short index;								//�±�
	SOCKET		SocketAll[WSA_MAXIMUM_WAIT_EVENTS];		//socket
	WSAEVENT	EventAll[WSA_MAXIMUM_WAIT_EVENTS];		//�¼�
	///WSA_MAXIMUM_WAIT_EVENTS �Ǻ� Ϊ64 ����Ϊ�ȴ��źź���WSAWaitForMultipleEvents��һ�����ֻ�ܴ�64������
};

//����һ������¼���������Ϊ64��
struct FD_se_set1
{
	unsigned short index;			//�±�
	SOCKET		SocketAll[1024];	//socket
	WSAEVENT	EventAll[1024];		//�¼�
};

int main(void)
{
	//������� WORD MAKEWORD WSADATA WSAStartup()
	WORD WSversion = MAKEWORD(2, 2);	//�汾��ʽת��
	WSADATA WSsockMsg;					//����2����סһЩ������Ϣ
	int nrse = WSAStartup(WSversion, &WSsockMsg);
	if (nrse == 0)
	{
		switch (nrse)
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
	//У��汾	HIBYTE LOBYTE
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		printf("�����汾����");
		WSACleanup();
		return 0;
	}
	//����SOCKET		socket()
	SOCKET Serversocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	//������������Э��������
	if (Serversocket == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//�󶨶˿ںţ���ַ bind
	struct sockaddr_in st;
	st.sin_family = AF_INET;		//��ַ����
	st.sin_port = htons(22258);		//�˿ں�ת����
	st.sin_addr.S_un.S_un_b.s_b1 = 127;	//��ַ
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	int brse = bind(Serversocket, (const struct sockaddr*)&st, sizeof(st));
	if (brse == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//�������� 
	if (listen(Serversocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//�����¼�
	WSAEVENT ServerEvent = WSACreateEvent();
	if (ServerEvent == WSA_INVALID_EVENT)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//���¼���socket�������룬Ͷ�ݸ�����ϵͳ������
	if (SOCKET_ERROR == WSAEventSelect(Serversocket, ServerEvent, FD_ACCEPT | FD_READ | FD_CLOSE | FD_WRITE))
	{
		int a = WSAGetLastError();		//�õ�������
		closesocket(Serversocket);		//����׽���
		WSACloseEvent(ServerEvent);		//��������һ��Ҫ��
		WSACleanup();					//�ر������
		return 0;
	}
	//�ýṹ���Ҫ�󶨵�socket���¼���װ��һ�����±�����¼
	//struct FD_se_set allES = { {0},{0},{NULL} };
	//allES.SocketAll[allES.index] = Serversocket;	//socket����
	//allES.EventAll[allES.index]  = ServerEvent;		//�¼�����
	//allES.index++;									//�±�

	//ѭ������ȡ�������źŲ������¼���û�н��������Ż��İ汾���������ã�
	{
		//while (1)
		//{
		//	//�ȴ��źţ�ѯ���źţ�
		//	DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES.index, allES.EventAll, FALSE, WSA_INFINITE, FALSE);
		//	//���ʧ����
		//	if (WSA_WAIT_FAILED == WSwaitforevent)
		//	{
		//		int a = WSAGetLastError();		//�õ�������
		//		printf("�����룺%d\n", a);
		//		break;
		//	}
		//	//�����ʱ��
		//	if (WSA_WAIT_TIMEOUT == WSwaitforevent)
		//	{
		//		continue;
		//	}
		//	//�ɹ��ˣ�ͨ������õ���Ӧ�¼����±�
		//	DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

		//	//�оٲ�������ȡ���źŵ��¼��Ĵ����������ͣ����������¼��ź�
		//	WSANETWORKEVENTS NetWorkEvents;		//��3�Ľṹ�壬������ס�������ݵ�һЩ��Ϣ
		//	if (SOCKET_ERROR == WSAEnumNetworkEvents(allES.SocketAll[nIndex], allES.EventAll[nIndex], &NetWorkEvents))
		//	{
		//		int a = WSAGetLastError();		//�õ�������
		//		printf("�����룺%d\n", a);		//�������ʧ�ܵĴ����룬���3�еĴ����������޹�
		//		break;
		//	}

		//	//���ദ��
		//	//���ж�FD_ACCEPT�����루���¼��������źţ��������ж��������Ĳ�������ʲô��Ȼ����д���
		//	if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//	{	//ɸѡ��FD_ACCEPT���ܰ�λ��Ľ�����õ����Լ��Ķ�������


		//		//���û�з�������
		//		if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//		{
		//			//��������		//��1����Serversocket��������socket��Ҳֻ�����ܵõ�FD_ACCEPT�Ĳ�����
		//			SOCKET clientSocket = accept(allES.SocketAll[nIndex], NULL, NULL);	//���������ǿͻ��˵���Ϣ�����Ե����ú�����ȡ��������NULL
		//			if (INVALID_SOCKET == clientSocket)
		//				continue;
		//			//socket���¼���ÿ��socket��Ҫ���¼���
		//			WSAEVENT clienteve = WSACreateEvent();
		//			if (WSA_INVALID_EVENT == clienteve)
		//			{
		//				closesocket(clientSocket);
		//				continue;
		//			}
		//			//Ͷ�ݸ�ϵͳ
		//			if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//			{
		//				closesocket(clientSocket);
		//				WSACloseEvent(clienteve);
		//				continue;
		//			}
		//			//���ͻ��˵�socket���¼���ӵ��ṹ�弯����
		//			allES.SocketAll[allES.index] = clientSocket;
		//			allES.EventAll[allES.index] = clienteve;
		//			allES.index++;
		//		}
		//		//��������˴���
		//		else
		//			continue;
		//		printf("FD_ACCEPT\n");
		//	}
		//	//�ж�FD_WRITE	
		//	if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//	{
		//		//�����������ʼ���������˼�뾭�飬���߼��ϲ�������ǰ�棬��ֻ����һ�Σ����Խ��г�ʼ��һЩ������

		//		//û�д����ܽ�����Ϣ����
		//		if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//		{
		//			if (SOCKET_ERROR == send(allES.SocketAll[nIndex], "connect success", strlen("connect success"), 0))
		//			{
		//				int a = WSAGetLastError();
		//				printf("send faild, error code:%d\n", a);
		//				continue;
		//			}
		//		}
		//		//�д�����
		//		else
		//		{
		//			printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//��ӡ������
		//			continue;
		//		}

		//		printf("FD_WRITE\n");
		//	}
		//	//�ж�FD_READ
		//	if (NetWorkEvents.lNetworkEvents & FD_READ)
		//	{
		//		//û�д����ܶ�ȡ��Ϣ
		//		if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//		{
		//			char read[1500] = { 0 };
		//			if (SOCKET_ERROR == recv(allES.SocketAll[nIndex], read, 1499, 0))
		//			{
		//				int a = WSAGetLastError();
		//				printf("recv error code:%d\n", a);	//��ӡ������
		//				continue;
		//			}
		//			printf("recv data:%s\n", read);
		//			continue;
		//		}
		//		//�д�����
		//		else
		//		{
		//			printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//��ӡ������
		//			continue;
		//		}
//	}
//	//�ж�FD_CLOSE
//	if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
//	{
//		//FD_CLOSE ���ͻ��������ߣ�������ǿ�ƻ���������������һ�������룺10053�����������������������Ĵ���һ�������������0��������֣�
//		printf("close client\n");
//		printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//��ӡ������
//		closesocket(allES.SocketAll[nIndex]);						//�ͷ����ߵĿͻ��˵�socket
//		allES.SocketAll[nIndex] = allES.SocketAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
//		WSACloseEvent(allES.EventAll[nIndex]);
//		allES.EventAll[nIndex] = allES.EventAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
//		allES.index--;
//	}

//	////switch���жϣ����Ƽ�ʹ�ã��޷����� ��λ�� �ķ��ദ��
//	//switch (NetWorkEvents.lNetworkEvents)
//	//{
//	//case FD_ACCEPT:
//	//	//���û�з�������
//	//	if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
//	//	{
//	//		
//	//		SOCKET clientSocket = accept(allES.SocketAll[nIndex], NULL, NULL);
//	//		if (INVALID_SOCKET == clientSocket)
//	//			continue;
//	//		WSAEVENT clienteve = WSACreateEvent();
//	//		if (WSA_INVALID_EVENT == clienteve)
//	//		{
//	//			closesocket(clientSocket);
//	//			continue;
//	//		}			
//	//		if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
//	//		{
//	//			closesocket(clientSocket);
//	//			WSACloseEvent(clienteve);
//	//			continue;
//	//		}
//	//		allES.SocketAll[allES.index] = clientSocket;
//	//		allES.EventAll[allES.index] = clienteve;
//	//		allES.index++;
//	//	}
//	//	//��������˴���
//	//	else
//	//		continue;
//	//	printf("FD_ACCEPT\n");
//	//	break;
//	//case FD_WRITE:
//	//	//û�д����ܽ�����Ϣ����
//	//	if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
//	//	{
//	//		if (SOCKET_ERROR == send(allES.SocketAll[nIndex], "connect success", strlen("connect success"), 0))
//	//		{
//	//			int a = WSAGetLastError();
//	//			printf("send faild, error code:%d\n", a);
//	//			continue;
//	//		}
//	//	}
//	//	//�д�����
//	//	else
//	//	{
//	//		printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//��ӡ������
//	//		continue;
//	//	}
//	//	printf("FD_WRITE\n");
//	//	break;
//	//case FD_READ:
//	//	//û�д����ܶ�ȡ��Ϣ
//	//	if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
//	//	{
//	//		char read[1500] = { 0 };
//	//		if (SOCKET_ERROR == recv(allES.SocketAll[nIndex], read, 1499, 0))
//	//		{
//	//			int a = WSAGetLastError();
//	//			printf("recv error code:%d\n", a);	//��ӡ������
//	//			continue;
//	//		}
//	//		printf("recv data:%s\n", read);
//	//		continue;
//	//	}
//	//	//�д�����
//	//	else
//	//	{
//	//		printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//��ӡ������
//	//		continue;
//	//	}
//	//	break;
//	//case FD_CLOSE:
//	//	printf("close client\n");
//	//	printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//��ӡ������
//	//	closesocket(allES.SocketAll[nIndex]);						//�ͷ����ߵĿͻ��˵�socket
//	//	allES.SocketAll[nIndex] = allES.SocketAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
//	//	WSACloseEvent(allES.EventAll[nIndex]);
//	//	allES.EventAll[nIndex] = allES.EventAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
//	//	allES.index--;
//	//	break;
//	//}
//}
	};

	//�������Ż��İ汾
	{
		//while (1)
		//{
		//	//�ȴ��źţ�ѯ���źţ�
		//	DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES.index, allES.EventAll, FALSE, WSA_INFINITE, FALSE);
		//	//���ʧ����
		//	if (WSA_WAIT_FAILED == WSwaitforevent)
		//	{
		//		int a = WSAGetLastError();		//�õ�������
		//		printf("�����룺%d\n", a);
		//		break;
		//	}
		//	//�����ʱ��
		//	if (WSA_WAIT_TIMEOUT == WSwaitforevent)
		//	{
		//		continue;
		//	}
		//	//�ɹ��ˣ�ͨ������õ���Ӧ�¼����±�
		//	DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

		//	//�����Ż��������̬������⣬
		//	for (int i = nIndex; i < allES.index; i++)
		//	{
		//		//�����źŵ���С�±꿪ʼ�������һ��һ�������ж�ִ�У�ÿ�εȴ�1���¼��ģ�����¼����ˣ����ձ�˳���������������һ�鰴˳�����ˣ��Ž�����һ��
		//		DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES.EventAll[i], FALSE, 0, FALSE);	//�˴���4�߸ĳ�0����Ҫ�ȴ�����Ȼĳһ���¼�û��Ӧ��һֱ����
		//		if (WSA_WAIT_FAILED == NeiSwaitforevent)
		//		{
		//			int a = WSAGetLastError();		//�õ�������
		//			printf("�����룺%d\n", a);
		//			break;
		//		}
		//		if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
		//		{
		//			continue;
		//		}

		//		//��ȡ���źź󣬽��в���
		//		//�оٲ�������ȡ���źŵ��¼��Ĵ����������ͣ����������¼��ź�
		//		WSANETWORKEVENTS NetWorkEvents;		//��3�Ľṹ�壬������ס�������ݵ�һЩ��Ϣ
		//		if (SOCKET_ERROR == WSAEnumNetworkEvents(allES.SocketAll[i], allES.EventAll[i], &NetWorkEvents))
		//		{
		//			int a = WSAGetLastError();		//�õ�������
		//			printf("�����룺%d\n", a);		//�������ʧ�ܵĴ����룬���3�еĴ����������޹�
		//			break;
		//		}

		//		//���ദ��
		//		//���ж�FD_ACCEPT�����루���¼��������źţ��������ж��������Ĳ�������ʲô��Ȼ����д���
		//		if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//		{	//ɸѡ��FD_ACCEPT���ܰ�λ��Ľ�����õ����Լ��Ķ�������


		//			//���û�з�������
		//			if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//			{
		//				//��������		//��1����Serversocket��������socket��Ҳֻ�����ܵõ�FD_ACCEPT�Ĳ�����
		//				SOCKET clientSocket = accept(allES.SocketAll[i], NULL, NULL);	//���������ǿͻ��˵���Ϣ�����Ե����ú�����ȡ��������NULL
		//				if (INVALID_SOCKET == clientSocket)
		//					continue;
		//				//socket���¼���ÿ��socket��Ҫ���¼���
		//				WSAEVENT clienteve = WSACreateEvent();
		//				if (WSA_INVALID_EVENT == clienteve)
		//				{
		//					closesocket(clientSocket);
		//					continue;
		//				}
		//				//Ͷ�ݸ�ϵͳ
		//				if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//				{
		//					closesocket(clientSocket);
		//					WSACloseEvent(clienteve);
		//					continue;
		//				}
		//				//���ͻ��˵�socket���¼���ӵ��ṹ�弯����
		//				allES.SocketAll[allES.index] = clientSocket;
		//				allES.EventAll[allES.index] = clienteve;
		//				allES.index++;
		//			}
		//			//��������˴���
		//			else
		//				continue;
		//			printf("FD_ACCEPT\n");
		//		}
		//		//�ж�FD_WRITE	
		//		if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//		{
		//			//�����������ʼ���������˼�뾭�飬���߼��ϲ�������ǰ�棬��ֻ����һ�Σ����Խ��г�ʼ��һЩ������

		//			//û�д����ܽ�����Ϣ����
		//			if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//			{
		//				if (SOCKET_ERROR == send(allES.SocketAll[i], "connect success", strlen("connect success"), 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("send faild, error code:%d\n", a);
		//					continue;
		//				}
		//			}
		//			//�д�����
		//			else
		//			{
		//				printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//��ӡ������
		//				continue;
		//			}
		//			printf("FD_WRITE\n");
		//		}
		//		//�ж�FD_READ
		//		if (NetWorkEvents.lNetworkEvents & FD_READ)
		//		{
		//			//û�д����ܶ�ȡ��Ϣ
		//			if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//			{
		//				char read[1500] = { 0 };
		//				if (SOCKET_ERROR == recv(allES.SocketAll[i], read, 1499, 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("recv error code:%d\n", a);	//��ӡ������
		//					continue;
		//				}
		//				printf("recv data:%s\n", read);
		//				continue;
		//			}
		//			//�д�����
		//			else
		//			{
		//				printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//��ӡ������
		//				continue;
		//			}
		//		}
		//		//�ж�FD_CLOSE
		//		if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
		//		{
		//			//FD_CLOSE ���ͻ��������ߣ�������ǿ�ƻ���������������һ�������룺10053�����������������������Ĵ���һ�������������0��������֣�
		//			printf("close client\n");
		//			printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//��ӡ������
		//			closesocket(allES.SocketAll[i]);						//�ͷ����ߵĿͻ��˵�socket
		//			allES.SocketAll[i] = allES.SocketAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
		//			WSACloseEvent(allES.EventAll[i]);
		//			allES.EventAll[i] = allES.EventAll[allES.index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
		//			allES.index--;
		//		}
		//	}
		//}
	};

	//����1��һ��һ���ģ�����¼���������Ϊ64��
	{
		////Ϊ�˷���1��������д�Ľṹ�壬��Ա���������
		//struct FD_se_set1 allES1 = { {0},{0},{NULL} };
		//allES1.SocketAll[allES1.index] = Serversocket;	//socket����
		//allES1.EventAll[allES1.index] = ServerEvent;		//�¼�����
		//allES1.index++;									//�±�

		//while (1)
		//{
		//	for (int i = 0; i < allES1.index; i++)
		//	{
		//		//�ȴ��źţ�ѯ���źţ�
		//		DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES1.EventAll[i], FALSE, 0, FALSE);
		//		if (WSA_WAIT_FAILED == NeiSwaitforevent)
		//		{
		//			int a = WSAGetLastError();		//�õ�������
		//			printf("�����룺%d\n", a);
		//			break;
		//		}
		//		if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
		//		{
		//			continue;
		//		}

		//		//��ȡ���źź󣬽��в���
		//		//�оٲ�������ȡ���źŵ��¼��Ĵ����������ͣ����������¼��ź�
		//		WSANETWORKEVENTS NetWorkEvents;		//��3�Ľṹ�壬������ס�������ݵ�һЩ��Ϣ
		//		if (SOCKET_ERROR == WSAEnumNetworkEvents(allES1.SocketAll[i], allES1.EventAll[i], &NetWorkEvents))
		//		{
		//			int a = WSAGetLastError();		//�õ�������
		//			printf("�����룺%d\n", a);		//�������ʧ�ܵĴ����룬���3�еĴ����������޹�
		//			break;
		//		}

		//		//���ദ��
		//		//���ж�FD_ACCEPT�����루���¼��������źţ��������ж��������Ĳ�������ʲô��Ȼ����д���
		//		if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//		{	//ɸѡ��FD_ACCEPT���ܰ�λ��Ľ�����õ����Լ��Ķ�������

		//			//���û�з�������
		//			if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//			{
		//				//��������		//��1����Serversocket��������socket��Ҳֻ�����ܵõ�FD_ACCEPT�Ĳ�����
		//				SOCKET clientSocket = accept(allES1.SocketAll[i], NULL, NULL);	//���������ǿͻ��˵���Ϣ�����Ե����ú�����ȡ��������NULL
		//				if (INVALID_SOCKET == clientSocket)
		//					continue;
		//				//socket���¼���ÿ��socket��Ҫ���¼���
		//				WSAEVENT clienteve = WSACreateEvent();
		//				if (WSA_INVALID_EVENT == clienteve)
		//				{
		//					closesocket(clientSocket);
		//					continue;
		//				}
		//				//Ͷ�ݸ�ϵͳ
		//				if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//				{
		//					closesocket(clientSocket);
		//					WSACloseEvent(clienteve);
		//					continue;
		//				}
		//				//���ͻ��˵�socket���¼���ӵ��ṹ�弯����
		//				allES1.SocketAll[allES1.index] = clientSocket;
		//				allES1.EventAll[allES1.index] = clienteve;
		//				allES1.index++;
		//			}
		//			//��������˴���
		//			else
		//				continue;
		//			printf("FD_ACCEPT\n");
		//		}
		//		//�ж�FD_WRITE	
		//		if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//		{
		//			//�����������ʼ���������˼�뾭�飬���߼��ϲ�������ǰ�棬��ֻ����һ�Σ����Խ��г�ʼ��һЩ������

		//			//û�д����ܽ�����Ϣ����
		//			if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//			{
		//				if (SOCKET_ERROR == send(allES1.SocketAll[i], "connect success", strlen("connect success"), 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("send faild, error code:%d\n", a);
		//					continue;
		//				}
		//			}
		//			//�д�����
		//			else
		//			{
		//				printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//��ӡ������
		//				continue;
		//			}

		//			printf("FD_WRITE\n");
		//		}
		//		//�ж�FD_READ
		//		if (NetWorkEvents.lNetworkEvents & FD_READ)
		//		{
		//			//û�д����ܶ�ȡ��Ϣ
		//			if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//			{
		//				char read[1500] = { 0 };
		//				if (SOCKET_ERROR == recv(allES1.SocketAll[i], read, 1499, 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("recv error code:%d\n", a);	//��ӡ������
		//					continue;
		//				}
		//				printf("recv data:%s\n", read);
		//				continue;
		//			}
		//			//�д�����
		//			else
		//			{
		//				printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//��ӡ������
		//				continue;
		//			}
		//		}
		//		//�ж�FD_CLOSE
		//		if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
		//		{
		//			//FD_CLOSE ���ͻ��������ߣ�������ǿ�ƻ���������������һ�������룺10053�����������������������Ĵ���һ�������������0��������֣�
		//			printf("close client\n");
		//			printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//��ӡ������
		//			closesocket(allES1.SocketAll[i]);						 //�ͷ����ߵĿͻ��˵�socket
		//			allES1.SocketAll[i] = allES1.SocketAll[allES1.index - 1];//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
		//			WSACloseEvent(allES1.EventAll[i]);
		//			allES1.EventAll[i] = allES1.EventAll[allES1.index - 1];	 //�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
		//			allES1.index--;
		//		}
		//	}	
		//}
	};

	//����2��һ��һ��ģ��ҵ���ɰ�
		//�ýṹ���Ҫ�󶨵�socket���¼���װ��һ�����±�����¼
		struct FD_se_set allES2[20];							//ÿ�������Ա��һ���ṹ�壬ÿ���ṹ�����64��socket���¼�
		memset(allES2, 0, sizeof(allES2));						//��ʼ��
		allES2[0].SocketAll[allES2[0].index] = Serversocket;	//socket����
		allES2[0].EventAll[allES2[0].index] = ServerEvent;		//�¼�����
		allES2[0].index++;

		while (1)
		{
			for (int wai = 0; wai < 20; wai++)
			{
				//��Ϊ�����˽ṹ�����飬����һЩԪ�ؽṹ��Ϊ0��������WSAWaitForMultipleEvents�����Ĳ����淶������Ҫ�����ж�
				if (0 == allES2[wai].index)
				{
					continue;
				}
				//�ȴ��źţ�ѯ���źţ�
				DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES2[wai].index, allES2[wai].EventAll, FALSE, 0, FALSE);
				//���ʧ����
				if (WSA_WAIT_FAILED == WSwaitforevent)
				{
					int a = WSAGetLastError();		//�õ�������
					printf("�����룺%d\n", a);
					break;
				}
				//�����ʱ��
				if (WSA_WAIT_TIMEOUT == WSwaitforevent)
				{
					continue;
				}
				//�ɹ��ˣ�ͨ������õ���Ӧ�¼����±�
				DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

				//�����Ż��������̬������⣬
				for (int i = nIndex; i < allES2[wai].index; i++)
				{
					//�����źŵ���С�±꿪ʼ�������һ��һ�������ж�ִ�У�ÿ�εȴ�1���¼��ģ�����¼����ˣ����ձ�˳���������������һ�鰴˳�����ˣ��Ž�����һ��
					DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES2[wai].EventAll[i], FALSE, 0, FALSE);	//�˴���4�߸ĳ�0����Ҫ�ȴ�����Ȼĳһ���¼�û��Ӧ��һֱ����
					if (WSA_WAIT_FAILED == NeiSwaitforevent)
					{
						int a = WSAGetLastError();		//�õ�������
						printf("�����룺%d\n", a);
						break;
					}
					if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
					{
						continue;
					}

					//��ȡ���źź󣬽��в���
					//�оٲ�������ȡ���źŵ��¼��Ĵ����������ͣ����������¼��ź�
					WSANETWORKEVENTS NetWorkEvents;		//��3�Ľṹ�壬������ס�������ݵ�һЩ��Ϣ
					if (SOCKET_ERROR == WSAEnumNetworkEvents(allES2[wai].SocketAll[i], allES2[wai].EventAll[i], &NetWorkEvents))
					{
						int a = WSAGetLastError();		//�õ�������
						printf("�����룺%d\n", a);		//�������ʧ�ܵĴ����룬���3�еĴ����������޹�
						break;
					}

					//���ദ��
					//���ж�FD_ACCEPT�����루���¼��������źţ��������ж��������Ĳ�������ʲô��Ȼ����д���
					if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
					{	//ɸѡ��FD_ACCEPT���ܰ�λ��Ľ�����õ����Լ��Ķ�������

						//���û�з�������
						if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
						{
							//��������		//��1����Serversocket��������socket��Ҳֻ�����ܵõ�FD_ACCEPT�Ĳ�����
							SOCKET clientSocket = accept(allES2[wai].SocketAll[i], NULL, NULL);	//���������ǿͻ��˵���Ϣ�����Ե����ú�����ȡ��������NULL
							if (INVALID_SOCKET == clientSocket)
								continue;
							//socket���¼���ÿ��socket��Ҫ���¼���
							WSAEVENT clienteve = WSACreateEvent();
							if (WSA_INVALID_EVENT == clienteve)
							{
								closesocket(clientSocket);
								continue;
							}
							//Ͷ�ݸ�ϵͳ
							if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
							{
								closesocket(clientSocket);
								WSACloseEvent(clienteve);
								continue;
							}
							//����ǰ���ӵĿͻ��˵�socket���¼���ӵ��ṹ�弯����
							//�����ǰ�Ľṹ��װ���ˣ���װ����һ��Ԫ�أ��ṹ�壩��
							for (int n = 0; n < 20; n++)
							{
								if (allES2[n].index < 64)
								{
									allES2[n].SocketAll[allES2[n].index] = clientSocket;
									allES2[n].EventAll[allES2[n].index] = clienteve;
									allES2[n].index++;
									break;
								}
							}		
						}
						//��������˴���
						else
							continue;
						printf("FD_ACCEPT\n");
					}
					//�ж�FD_WRITE	
					if (NetWorkEvents.lNetworkEvents & FD_WRITE)
					{
						//û�д����ܽ�����Ϣ����
						if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
						{
							if (SOCKET_ERROR == send(allES2[wai].SocketAll[i], "connect success", strlen("connect success"), 0))
							{
								int a = WSAGetLastError();
								printf("send faild, error code:%d\n", a);
								continue;
							}
						}
						//�д�����
						else
						{
							printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//��ӡ������
							continue;
						}
						printf("FD_WRITE\n");
					}
					//�ж�FD_READ
					if (NetWorkEvents.lNetworkEvents & FD_READ)
					{
						//û�д����ܶ�ȡ��Ϣ
						if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
						{
							char read[1500] = { 0 };
							if (SOCKET_ERROR == recv(allES2[wai].SocketAll[i], read, 1499, 0))
							{
								int a = WSAGetLastError();
								printf("recv error code:%d\n", a);	//��ӡ������
								continue;
							}
							printf("recv data:%s\n", read);
							continue;
						}
						//�д�����
						else
						{
							printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//��ӡ������
							continue;
						}
					}
					//�ж�FD_CLOSE
					if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
					{
						//FD_CLOSE ���ͻ��������ߣ�������ǿ�ƻ���������������һ�������룺10053�����������������������Ĵ���һ�������������0��������֣�
						printf("close client\n");
						printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//��ӡ������
						closesocket(allES2[wai].SocketAll[i]);						//�ͷ����ߵĿͻ��˵�socket
						allES2[wai].SocketAll[i] = allES2[wai].SocketAll[allES2[wai].index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
						WSACloseEvent(allES2[wai].EventAll[i]);
						allES2[wai].EventAll[i] = allES2[wai].EventAll[allES2[wai].index - 1];	//�������һ��Ԫ���������ͷŵ�Ԫ�ص�λ��
						allES2[wai].index--;
					}
				}
			}
			
		}
	
	

	//�ͷž�����׽��֣�������������޵ķ���2��
	for (int m = 0; m < 20; m++)
	{
		for (int n = 0; n < allES2[m].index; n++)
		{
			//�ͷ��¼����
			WSACloseEvent(allES2[m].EventAll[n]);	//����ֵ��BOOL����
			//�ͷ��׽���
			closesocket(allES2[m].SocketAll[n]);
		}
	}
	
	////�ͷ��¼����
	//WSACloseEvent(ServerEvent);	//����ֵ��BOOL����
	////�ͷ��׽���
	//closesocket(Serversocket);
	//�ر������
	WSACleanup();
	

	system("pause>0");
	return 0;
}


void study()
{
	if (4.1 - 4.7)
	{
		//4.1 windows�¼���������Ϣ����
	/*
	* windows���� �û���Ϊ �����ַ�ʽ��������Ҫ��
	*		��Ϣ���ƣ����ģ���Ϣ����
	*		�¼����ƣ����ģ��¼�����
	* ��ʦ˵��������Ϣ���Ƹ���������ṹ�Ĳ���
	*			�¼����Ƹ�����Ǿֲ�ʵ�ֵĲ���
	*			�������Ի������ʹ��
	*
	* ��Ϣ���ƣ�
	*		���ģ���Ϣ����
	*		������̣�
	*			���е��û���������������꣬�����̣����ֹ�����
	*			������һ�����ݣ�����˳���Ž�һ�������У��Ƚ��ȳ���
	*			���еĴ������ɲ���ϵͳ����ɵ�
	*		�ص㣺
	*			��Ϣ�����ɲ���ϵͳ��ά�����������Ĳ������ǰ���Ϣȡ���������д���ͷ���
	*			��˵�������ǲ��ù������Ϣ���У�ֻҪ֪����ôȡ�������ݣ�������ݶ�Ӧ�������û���һЩ����
	*			Ȼ������������ݣ���Ϣ����ִ����Ӧ�ĺ����������ǳ���Ա�Ĺ���
	*			��Ϣ�������Ⱥ�˳��
	*		������
	*			��ʦ˵���������Win32��MFC�ȿγ̣��������ǻ�����Ϣ���У��Ⱥ�����
	*			������һ��ѧϰ��ģ�ͣ��첽ѡ����ǻ��������Ϣ���е�
	*
	* �¼����ƣ��¼�����	�����ʵ�ֵĵ�Ƭ���е��жϣ�
	*		������̣�
	*			������Ҫ������Ϊ�û����ض�������һ���¼����¼��������Լ�����API������Ҫ���پ��ܽ�����
	*			���¼����ݸ�ϵͳ����ϵͳ�����Ǽ��ӣ����Բ���̫�࣬��Ȼϵͳ�Ϳ���
	*			������������ˣ����簴��꣬��������Ӧ�¼��ͻ�ת������Ӧ���źţ���һ�����������
	*			Ȼ�����ǻ�ȡ�����źŵ��¼���������д���
	*		�ص㣺
	*			�����¼����������Ƕ���ģ�ϵͳֻ�ǰ����Ƿ����ź�
	*			�¼��������������
	*			��οε��¼�ѡ�񣬾��ǻ����¼���������ɵ�
	*/
	//4.2 �¼�ѡ��ģ�͵Ĵ����߼�
	/*
	* ��Ϣ���У�
	*		�ɲ���ϵͳ�����������ɲ���ϵͳ��ܣ�����ֻҪȡ������
	*		Ȼ����������ݽ��д���
	* �¼����ƣ�
	*		����Ҫ�����¼���Ϊ�¼��󶨲�����Ȼ���ɴ�����ϵͳ���¼�һ���ź�
	*		������������ź������д���
	*		����Ҫ�����±���Ϣ���ж�
	*
	* �¼�ѡ��ģ�ͣ����һ��Ǿ���Ҫ���ж���������͸��������Qt�е��¼���
	*		�����߼���select��࣬���ֽ�WSAEventSelect ������select�Ľ��װ�
	*	��1��������һ���¼����󣨱�����
	*	��2����Ϊÿ���¼������һ��socket�Լ�������accept��read��close������Ͷ�ݸ�ϵͳ
	*			Ͷ�ݸ�ϵͳ�󣬾Ͳ��ù��ˣ�ϵͳ�Լ����ܣ����ǾͿ�������Ĳ�����
	*	��3�����鿴�¼��Ƿ����ź�
	*	��4�������źŵĻ��ͷ��ദ��
	*
	* ������Щ��ǰ�����ܽ��˵ľ��������Ծ��ܸ��ӣ���������ʹ��ǰ������õĶ����ͱȽϼ�
	*
	* �첽������һ���µ�ʱ��Ҳ��ȥ����һ���£����ǽ��ִ������������
	*/
	//4.3 �����¼������Լ��ں˶���ļ򵥽���
	/*
	* �¼�ѡ��ģ�͵����̣�
	*		1.����ͷ�ļ��������
	*		2.�������
	*		3.У��汾
	*		4.����SOCKET
	*		5.�󶨶˿ںţ���ַ
	*		6.��ʼ����
	*		7.�¼�ѡ��
	*
	* �¼�ѡ��
	*		����һ���¼�����
	*		�ö����socket��Ͷ�ݸ�����ϵͳ
	*
	* �����¼�����
	*		WSAEVENT WSAAPI = WSACreateEvent();
	*		WSAEVENT��һ��������ɿ���ID��Ψһ��ʶ����������viod*���޷�ȷ�����������ͣ�
	*		��������������ں˺������ں��ǲ���ϵͳ�Ŀռ�
	*		���ǵ����г��򣬶����ڲ���ϵͳ����ɵģ����Կ���һ������ĳ���
	*		��������VS��Щ�������������������������еģ��ǾͿ��Լ򻯿�����һ��������������������
	*		��������VS����Ŀռ��൱�ھֲ����������ں˺��������Ķ��󣨿ռ䣩���൱���������ڵı�����
	*		��VS�ĳ������ʱ�����ͷŵ�����������Ŀռ䣬�����ں˶���Ŀռ������ڲ���ϵͳ�ģ��޷�һ���ͷŵ�
	*		��������Ҫ�ֶ�����һ���ͷź������ͷ������ں˺������������ͷŵĺ����ǳɶԳ��ֵ�
	*		socket����һ���ں˺�����������Ҫ�ֶ�����closesocket�������ͷ�
	*		�������������ں˶��󻹴��ڣ���һֱ���ڣ����²���ϵͳ�ڴ�Խ��Խ��
	*
	*		�ɹ�������һ���¼�
	*		ʧ�ܣ�����WSA_INVALID_EVENT�����Ի�ȡ�����룬Ȼ��ʧ�ܴ���Ҫ�ͷŵ�socket���ر������
	*		���Ƕ�WSAEVENT��������ڲ���ϵͳ�г��õ��� F12�õ�������HANDLE��������õ�void* ���޷�ȷ�����ͣ����ݣ����ʺϵ�������ϵͳ���������ͣ�
	*
	* �ں˶���
	*		�ɲ���ϵͳ���ں�����
	*		�ɲ���ϵͳ����
	*		void*��ͨ������ָ�룩�������޷���λ�����ݣ�Ҳ�����޸ģ����ڶ��ں˵ı������Թ���ı������Ӷ�ʹ����ϵͳ����ġ�ƽ�ȵġ���Ч�����У�������������
	*		���ú������������ú����ͷţ������ǲ���ϵͳ�ģ����Բ����Զ��ڳ������ͷţ����Դ������ͷ�������ʹ����Ҫ�����õ�
	*		�������û�е����ͷţ���ô�����ܾ�һֱ�������ںˣ�����ں��ڴ�й©�� ����ֻ����������
	*		�ں˶����У�socket��Kernel Objects��ȥMSDN�Ͽ��Բ鿴��
	*
	* ����������
	*		BOOL WSAAPI WSACloseEvent(WSAEVENT hEvent);	�ر��ͷ��¼���������þ�Ҫ�ͷţ�����һֱ����
	*		BOOL WSAAPI WSAResetEvent(WSAEVENT hEvent);	���ã�������һ��Ҫѧ�����ݣ�ϵͳ�Ὣ�¼��ó����źŵģ�������������ܽ��������Ϊ���źŵ�
	*		BOOL WSAAPI WSASetEvent(WSAEVENT hEvent);	��ָ���¼��������ó����ź�
	*
	*/
	//4.4 WSAEventSelect �������⣨�¼���socket������룬Ͷ�ݸ�����ϵͳ��
	/*
	* WSAEventSelect��
	*		���ã����¼�����socket������룬��Ͷ�ݸ�����ϵͳ
	*				�� | �� ��ͬʱ������������
	*				ϵͳ�����¼��Ƿ��ж�Ӧ���������¼����һ���ź�
	*
	*	����1�����󶨵�socket��  ��������Ŀ������ÿ��socket������һ���¼�
	*	����2���¼��������¼������ò�1 ����2�󶨵�һ��
	*	����3��Ҫ�жϵľ����¼�������д�����ͣ�������Ҫ��עѧϰǰ�ĸ���
	*	����ֵ���ɹ����� 0�� ʧ�ܷ���SOCKET_ERROR��ʧ�ܴ���Ҫ�ͷž����
	*
	*		����3�Ǽ��Ӷ�Ӧ�¼��ģ����ݲ�ͬ������ѡ�񣬼��ӵ�����Ҳ��һ������Ӧ�����ݷ����˱仯������������
	*		��������Ӧ���źţ��������������ӵĹ������ɲ���ϵͳ����ɵ�
	*
	* ����3���������룩	��ǰ4������Ҫѧϰʹ�õģ�
	*		FD_ACCEPT	���пͻ������ӣ�����ʱ�ͻ�������źţ���ͻ���socket�󶨣���accept�йأ�accept��ִ�У�����ϵͳ�ͼ�⵽�����¼�һ���ź�
	*		FD_READ		��ͷ�ͻ��˷�����Ϣ�����յ���Ϣ�����������źţ���ͻ���socket�󶨣���recv�й�
	*						���Զ�����Բ��У����磺 FD_READ | FD_ACCEPT �û�|���Ϳ������ϳ�һ��������
	*		FD_CLOSE	���ͻ������ߣ���ͻ���socket�󶨣�������ǿ���������������߶���һ���ģ���ͻ���socket��
	*		FD_WRITE	�����Ը��ͻ��˷���Ϣ������accept�����������������źţ��յ����źſ���˵���ͻ��������ӳɹ�����ʱ��send������ͻ���socket��
	*
	*		FD_CONNECT	���ͻ�����ʹ�õģ����ͻ���Ҳ�����¼�ѡ��ģ�ͣ�����������ʱ���յ��ź�
	*		0			��ȡ���¼����ӣ����������Ѿ���socket����һ���¼���������WSAEventSelect��0���ͻḲ�ǵ�ԭ�ȵģ���0���ǲ����ӣ���ȡ����ԭ�����¼�������
	*						���Զ���¼���������д������ĻὫǰ��ĸ��ǵ�
	*		FD_OOB		��������ݣ�����һ�����ݼ����һ�������ֽڣ�һ�㲻ʹ��
	*		FD_QOS		���׽��ַ���������״̬�����仯���������źţ�������Ϣ֪ͨ
	*						��������ͬʱ�����ļ�������Ӱ�����裬�����ҳ���Ǵ���ķ����ǲ�����ģ�����ĳһ����Ϊ�����ȼ���
	*						��������Ĵ���͸ߣ���ʱ�ķ������������ı䣬�ͻ������Ϣ֪ͨ
	*						�����ǣ�WSAIoctl������ʦ˵��ǿ���Ժ����
	*		FD_GROUP_QOS�������Ĳ��������ͣ�����ASCII����һЩ������û�ж�Ӧ�ַ��ģ���Ϊ�˱������ã��Ժ���»��������ܾ������ˣ���ʦ˵����������ձ�
	*
	*		�ص�IOģ���н��ܵģ��������������Ͻ��ܱȽ��٣�����ὲһ�£��Ƚ�ƫ�ţ�
	*			FD_ROUTING_ INTERFACE_CHANGE�����Ƿ�������ͻ���֮���·��·�������˸ı䣬��������Ӧ�ź�
	*											Ҫͨ��WSAIoctlע��֮�󣬲���ʹ��
	*			FD_ADDRESS_ LIST_CHANGE		����Ҫ�����׽��ֵ�ַ��ı��ص�ַ�б����֪ͨ���������Ƿ����������źܶ�ͻ��ˣ��ͻ���һ��������¼����Щ�ͻ�����Ϣ
	*											���ͻ��˷����仯�������һ����һ�����ͻᱻ���Ӳ���������Ӧ�ź�
	*											Ҫͨ��WSAIoctlע��֮�󣬲���ʹ��
	*/
	//4.5 �߼������Ż�
	/*
	* �����Ż�˼·��
	*		����һ���ṹ��
	*		��Ա��һ�������±꣩�������飬����ֱ��¼�Ŷ�Ӧ��socket���¼�
	*		Ŀ���Ǹ����±꣬����֪������������Ҫ���󶨵�һ�𣬲�Ͷ�ݸ�����ϵͳ��
	*
	*		��������ͬʱ�����socket��Ĭ����64������һ���꣺WSA_MAXIMUM_WAIT_EVENTS  ������64���������Ϊ�˺ñ��ϳ���Ĭ��ֵ���Ŷ���ĺ꣨���ܽ�Ϊ�����˼�룩
	*		����������������СҲ�Ͷ���Ϊ64
	*		����selectģ�ͣ��ܸı��ܴ���ĸ�������Ϊselect������Ǹ����飬ͨ���������Ƚ�ֱ�ӣ�
	*		�¼�ѡ�����첽Ͷ�ţ���ϵͳ�������ǾͲ�������޸��ˣ�Ҫ���չ�����
	*		��index���ڵ����±꣬Ҫ��socket���¼�װ��ȥ�Ժ�� +1 ����������Ͳ��Ǵ�0��ʼ������0��������
	*
	*		��ʼ��ʱ �¼�������Ϊ�����void* ���͵ģ����Գ�ʼ����NULL����ʵsocket������Ҳ�ܳ�ʼ����NULL
	*/
	//4.6 �ȴ��źţ�ѯ���źţ�������WSAWaitForMultipleEvents
	/*
	*	����ѧϰʱҪע�⣬��ʵ�����̣�����ѧ��һЩ��������������Ĳ�������࣬�����Ǽ������ͻ�����
	*	����Ӧ����ԽѧԽ�򵥵�
	*
	* WSAWaitForMultipleEvents��
	*	Ҫ�����һ��д�Ľṹ����ʹ�ã����ýṹ��Ҳ���ԣ������鷳�ܶ࣬������ԱҪ�����������
	*	���ǲ���Ҫ֪����������ֵ����������ʲô������������
	*	����ֻҪ֪��������ֵ ��ȥ һ���� == �õ���Ӧ�¼����±�  �Ϳ�����
	*
	*	����������ص�IOģ������ʹ�ã�����˵�ص�IOģ�������ǵ��¼��������й����ģ�
	*	����ѧϰ��ģ�͵�·������˼·��
	*		C/Sģ�� �����������ܶ������޷������Ҫ�����Ż� --> Ϊ�˽��ɵ������������selectģ�ͣ�
	*		selectģ�ͱ���Ҳ��ȱ�㣬����ִ������û������ͻ���һ���ӳپ����� --> Ϊ���Ż��ֳ�������˼�룺1.��windows���¼����� 2.����Ϣ����
	*		�¼����� --> ���ݻ��˱��ε��¼�ѡ��ģ�� --> �����¼����Ƶķ���������Ż����õ����ص�IOģ��
	*		��Ϣ���� --> �ݻ������첽ѡ��ģ��		 --> ������Ϣ���Ƶķ����Ż����룬�õ�����ɶ˿�ģ��
	*
	*	���ã�
	*		��ȡ�����źŵ��¼�����������ϵͳ��أ����źŷ������¼��ͷ��أ�
	*		�������Ҳ���Խ�ѯ���źţ�������ȴ��źŷ�������˼��һ����
	*
	*	����1��
	*		Ҫѯ�ʵ��¼��ĸ�����������ΪWSA_MAXIMUM_WAIT_EVENTS ��64��������һ�εıʼ��н����
	*		��Ϊ�ǲ���ϵͳ������ͳ�Ƶģ��������ǲ�����selectģ��һ��ȥ�޸��������ǹ̶���
	*
	*	����2��
	*		�¼��б���������д�õ��¼������飨���ϣ�
	*
	*	����3��
	*		�¼��ȴ��ķ�ʽ��TRUE �� FALSE
	*		TRUE	:���е��¼����������źţ��Ż��к������أ��ص�IOģ��ʹ�ã��Ȳ�����
	*					���ӳٺܸߣ���������ڵ������¼������źŲŷ��أ�����ǰ���ź���Ӧ���¼�������õȵ������¼����źŲ��ܷ��أ�
	*		FALSE	:�κ�һ���¼��������źţ������Ϸ��ء�
	*					��������ֵ��һ��������������ֵ ��ȥ WSA_WAIT_EVENT_0���꣺Ϊ0�� == �±꣨��֪��ΪɶҪ����
	*					�õ����±��ʾ�¼������������ͨ���±����ҵ���Ӧ���¼�����
	*					������ú����ڼ䣬�ж��ʱ���źŲ�������᷵���±���С����һ��
	*					���ᰴ����Ӧ�źŵ��Ⱥ�˳�򣬲���ʲôʱ���Ȼ��Ǻ󣬶�ֻ������һ����Ӧ�ź����±���С���¼�
	*
	*	����4��
	*		��ʱ����������ȴ�ʱ�䣩���Ժ���Ϊ��λ����select�Ĳ���5һ��������
	*		123		����ʾ�ȴ�123���룬��ʱ����WSA_WAIT_TIMEOUT
	*		0		������¼������״̬���������أ�������û���ź�
	*		WSA_INFINITE	���ȴ���ֱ�����¼�����
	*		�����������select��������ķ�ʽ�����ӣ������жϵȴ�������������ɲ���ϵͳͳһ��أ�һ���أ������ڱ��������
	*		�����������������������Ϊ��λ�ĳ�ʱ�������һ���¼�û��һ����Ӧ����ʱ�����߼�����
	*		��select����ÿ���ͻ���Ϊ��λ���������ǲ�һ���ģ�ע������
	*		����������WSA_INFINITE�ͺ��ˣ����������ⶼ�Ƕ���Ӧ��ʱ����д���ģ�һֱ����Ҳû��ϵ����Ӱ���¼���Ӧ�ķ����ӳ�
	*
	*	����5��
	*		TRUE	:�ص�IOģ���ٽ�
	*		FALSE	:�¼�ѡ��ģ��������дFALSE����
	*
	*	����ֵ��
	*		�����±������ֵ�����ݲ���3ΪTRUE ���� FALSE������
	*				TRUE:�����źž�����Ӧ
	*				FALSE������ֵ - WSA_WAIT_EVENT_0 == ��Ӧ�¼��������е��±�
	*		WSA_WAIT_IO_COMPLETION��
	*				����5����дTRUE���Ż᷵�����ֵ
	*		WSA_WAIT_TIMEOUT��
	*				��ʱ�ˣ��õ��������continue����ѭ����ͷ��������
	*				�����3��ѡ���йأ��������3��дΪWSA_INFINITE��ֱ������Ӧ���У��Ǿ�û�л��᷵�����ֵ
	*		ʧ���ˣ�����WSA_WAIT_FAILED
	*/
	//4.7 �оٲ����ź�
	/*
	* ϸ�ڲ�ע��Ļ������׳�BUG����Ϊ��ⲻ��λ�����εĲ���3���Ǻ�ϸ�ڵĵط������׳���
	*
	* ����ϵͳ������Ͷ�ݸ����¼��󣬾ͺ������жϵȴ�ɶ���źŴ����޹��ˣ����������ģ�Ȼ����¼��ź�
	* ����Ҫ��Ҫ�õ������Ӧ���¼������ǵ��£���Ӱ�����ϵͳ�ļ���
	*
	* ���ã�
	*		1.��ȡ�¼��Ĳ������ͣ������¼���Ӧ���Ӧ�Ĳ������� accept recv close�ȵ�
	*		2.���¼��ϵ��ź����ã������ǵȴ��źź����õ���Ӧ���¼���Ҫ������¼����ź�����
	*
	* ����1��
	*		��Ӧ��socket
	*
	* ����2��
	*		��Ӧ���¼�
	*
	* ����3������Ҫ��ϸ�ڣ�
	*		struct _WSANETWORKEVENTS{ long lNetworkEvents; int iErrorCode[FD_MAX_EVENTS];}
	*		�����Ĳ��������Ͷ�������װ�ţ���������Ӧ�¼�����Ϣװ������ṹ����
	*		��һ���ṹ��ָ�룺 LPWSANETWORKEVENTS ( ȥ��LP�Ͳ���ָ���ˣ�����LP��ָ�����͵�������)
	*			��Ա1����Ӧ�¼��ľ���Ĳ���
	*						����������Ͷ��ʱ������ | Ͷ���˶�������룬���п�����������ͬʱ����ʱ��
	*						����������ͽ����� ��λ ���������ǰ�λ��õ��Ľ���Ͳ�֪�����ĸ���ֵ��(����������ʵ͵��ǻ��Ǵ��ڵ�)
	*			��Ա2������������飬
	*						���Ƕ�Ӧ�¼��Ĵ����������������ӣ����ͣ����� ��Щ����û�гɹ�����Щ�������װ��������
	*						װ��˳���������ģ����Ƕ�Ͷ��ʱ�Ĳ�����F12�ܿ���FD_READ_BIT 1��2��3��4����Ӧ����Ӧ�Ĳ�����
	*						���������ɶ�Ӧ�Ĳ��������������ԣ�1��2��3���±�Ŀռ����洢��Ӧ�Ĳ���������
	*						�����������ֱ�Ǵ�������ʲô�ˣ��±��Ӧ�Ų����룬�������Ӧ�Ŵ���������
	*
	* ����ֵ��
	*		�ɹ������� 0
	*		ʧ�ܣ����� SOCKET_ERROR	������ʧ�ܴ���
	*
	*		ע�⣺ʧ�ܴ����ȡ�Ĵ������Ǳ�����ʧ�ܵĴ����룬��3��Ա2�Ĵ��������飬�Ƕ�Ӧ�¼��Ĵ�����������recv�ȣ�ʧ�ܵĴ�����
	*/
	}

	if (4.8)
	{
		//4.8 �¼����ദ���߼� 1
		/*
		*
		* ΪʲôҪ�жϣ�
		*			���¼��������źţ��������ж��������Ĳ�������ʲô��Ȼ����д���
		*			������FD_ACCEPT�����룬�����Ǿ�֪���������������������Ǿͽ���accept�������ӣ�����д����룬�Ͳ�����
		*
		* ��4.7 �Ĳ���3������FD_ACCEPT
		*		ÿ�������룬F12���Կ�������Ψһ��Ӧ�������꣨��ֵ��FD_ACCEPT_BIT  ��  FD_ACCEPT�������Ƶ�
		*		FD_ACCEPT_BIT��1��2��3��4��������������ÿ����������Ψһ��Ӧ�ģ������ܹ���������4.7�еĲ���3�������±�
		*		ֻҪ����±��Ӧ�Ŀռ䲻Ϊ0�������д����룬����±��Ӧ�Ŀռ�Ϊ0����û�д����±��Ӧ�Ų�����Ψһ�ı�ʶ
		*
		* �Ƚ���FD_ACCEPT�İ�λ���жϣ����ദ����
		*		ɸѡ���˰�λ������⣬���ð�λ�뱾���ֵõ��˱���
		*		��λ�����㣬���������4.7���е����������밴λ���ˣ����ٰ�λ��ͻص�ԭ���Ķ�������
		*		����A B��λ���ˣ� Ȼ��õ��Ķ������ٸ� A���а�λ�룬 ����ֵõ���A�ö����ƣ��߼�����Ľ������ס���۾ͺ��ˣ�
		*/
		//4.9 �¼����ദ���߼� 2
		/*
		* FD_ACCEPT �� FD_WRITE:
		*		���пͻ������ӵ�ʱ�򣬲���ϵͳ�������ȴ���һ��FD_ACCEPT
		*		Ȼ����������ϴ���һ��FD_WRITE����ֻ����һ��
		*		���ǿ����ڶ�Ӧ�Ĵ����м����ӡ���鿴
		* 
		*		����FD_WRITE���߼����ڿͻ������ӹ��̵���ǰ�棬
		*		�����������ô���FD_WRITEʱ�����г�ʼ�����ܹ㷺�ı��˼�룬�߼�����ǰ��ģ���������ʼ����
		* 
		* FD_WRITE_BIT�Ĵ����룺
		*		�Ǵ���ͻ������ӹ����г��ֵĴ�����һ�ֹ������������
		*		���Ǻ���ִ�еĴ�����
		* 
		* strlen �� sizeof������
		*		strlenͳ�Ƶ��ַ����������ַ�����β��\0
		*		sizeofͳ�Ƶ��ַ����ǰ�����\0��
		*		ͬ�����ַ�����sizeof��strlenͳ�Ƶ���ֵ����1��
		* 
		* FD_CLOSE��
		*		���ֲ�����ô���ߣ������д�����10053
		*		�ͷŵ�ʱ����Ϊ����ռ������޵ģ����Ա�ɾ����Ԫ�أ���ԭ�ȵ�λ��Ҫ������Ԫ�����
		*		����ֱ����ĩβԪ���������λ�ã�����ɾ�����
		*		ĩβԪ�ص��±���[allES.index - 1]�����Ҫע�⣬˼��һ�¾�֪���ˣ���Ϊ�±�ָ��ĩβԪ�ص���һ��
		*/
		//4.10 ʱ����ദ���߼� 3
		/*
		* �����Ͻڿ�FD_CLOSE������Ĵ�����10053�����⣺
		*		FD_CLOSE ��MSDN�鿴����3�ִ����룬
		*		���У�WSAECONNABORTED�ǿͻ������ߣ�������ǿ�ƻ����������ߣ�����ֹ����ʱ���ͻ᷵�ص�
		*		����FD_CLOSE�ǲ����ڷ���0������ģ�������ζ��᷵��һ��������
		*		֪������������ǾͲ����д�����Ϊ0���жϾͺ���
		* 
		* ����1��
		*		������WSAWaitForMultipleEvents���ȴ��źŴ����¶ϵ㣬����
		*		Ȼ����Ͽͻ��ˣ��������ݸ���������Ȼ�������F10�鿴�������
		*	���
		*		��ʱ����������Ϊ�ϵ�ԭ��FD_WRITE��FD_READ��ͬʱ�յ�
		*		�ͻᵼ�����������˰�λ����������λ���Ľ����ʮ���Ƶ�3
		*		���Ǿ��ܹ��۲⵽������д�ķ����ж��Ƿ���Ч
		*		�ҷ��ֽ�����FD_WRITE�жϲ����󣬽����ֽ�����FD_READ���жϲ���
		*		���������� if else ���ŵ㣬else if �� switch ��ִֻ��һ�Σ�Ȼ��ʣ�µ��ж϶���ִ����
		*	�ܽ�
		*		�����Ѿ���socket���¼���Ͷ�ݸ�����ϵͳ�����ˣ��������ǳ�����Կ�ס
		*		���ǲ���ϵͳ���������м��Ӳ����źŵ�
		*		Э��ջҲ����Ȼ�ڹ�������Ҳ�Ƿ�װ�ĺô����е㲻�����
		*		����ϵͳ�����¼�ʱ�������źţ��ڵȴ��źź������������źŽ����˰�λ�����һ����ֵ
		*		����������жϾ��ǽ�����źŽ��������ദ����λ�����ʽ����Ȼ���ж�ִ�ж�Ӧ����
		* 
		* ����2��
		*		�ò���1�ķ�ʽ�¶ϵ㣬���У������ͻ��˷�����Ϣ
		*		�������ǿͻ����ڷ������ϵ��ʱ���������ͼ�����Ϣ
		*		Ȼ��������з������۲������յ���Ϣ�����
		*	���
		*		������յ�����Ϣ�����ǲ����Ƿּ������յģ��������ϳ���һ��
		*		�����Э�黺���������ã��ͻ��˷��͵Ķ������ڷ�������Э��ջ��Э�黺�����������
		*		Ȼ���������ͣ��Ͷ���������Э�黺�������������������У��õ�FD_READ��Ϣ���ͽ���recv�Ĳ���
		*		recvȥȡ��ʱ�򣬾Ͱ�Э�黺�����Ķ�ȡ�����ˣ���������������ӡ����������
		*	����
		*		Э��ջ��Э�黺�����������Ƕϵ����ʱ�����ڹ������Ұ���Ϣ����Э�黺����
		* 
		* ����3��
		*		ʹ��else if �����в���1
		*		ʹ��shitch�����в���1
		*	else if:
		*		���ж���һ����ִ�к󣬾Ͳ�ִ��ʣ�µ��ж���
		*		���Գ��ֲ����밴λ��������ֻ�ܴ����߼�����ǰ�����һ��
		*	switch��
		*		�޷����в�����Ľ�������Ϊcase ���޷���������
		*		���Ծ��޷��Բ����밴λ����д�������FD_CLOSE��FD_READ��λ���Ժ�õ�3
		*		��û�в�������3�������޷����������ദ����Ҳ���޷�ִ���κβ���
		*	�ܽ�
		*		�����Ƽ����ֲ����밴λ��ʹ��else if ������ִֻ��һ������һ����ִ�У�����Ч�ʸ���һ�㣬��Ϊֻ���ж�һ��
		*		�����õ�if ���߼��ϸ�����
		*		switch�޷���������밴λ������������Ҫʹ�ã�
		*		�����밴λ��Ĵ���ͽ������¼����ദ���߼�������������3�ڿε�����
		*		
		*/
		//4.11 �����Ż���̬���
		/*
		*	��ʦ˵Windows�������Ȿ�飬����ֱ��MSDN�����ݣ�û��ʲôԭ���Ķ���
		* 
		* ��̬������⣺
		*		�ȴ��źź���WSAWaitForMultipleEvents���᷵�����źŲ������¼���
		*		�����ص��Ǽ����У�����2���±���С��
		*		���������ˣ�����ڼ����У��±�5���¼�����Ӧ���±�1���¼�����Ӧ���Ǹ���������������ԣ����ȷ����±�1���¼���Ȼ����к�������
		*		���ں��������±�1���¼������У��±�1���¼�����Ӧ�ˣ����ٴ�ѭ��ʱ�������ȴ��źź������ַ������±�1���¼�
		*		�������±�1������¼���Ҫ�ȵ�1��ȫû��Ӧ�˲��ܴ������һ��ǰ����±�Ĵ�С�߼������ش���
		*		�����Ͳ������ˣ���Ϊ�������Ⱥ�˳��Ļ�������Ӧ�Ĳ�һ���ȴ����ӳپͿ��ܻ�ܸ�
		*	�������
		*		�����������߼��Ͻ���һ���������Ż������������̸���ƽ
		* 
		* ������
		*		��һ�εȴ��¼���Ȼ��ӷ��ص��±꿪ʼ��forѭ���������һ��һ���ı����жϴ���
		*		�����±�1�Ĵ����굱ǰ�Ĳ������͵��±�2�Ŀ�ʼ��Ȼ��3��4��5��ֱ��������һ�飬�ٴ�ͷ��ʼ
		*		��������ȫ�Ĺ�ƽ�����򣬵��Ǳ������Ҫ�úܶ�
		*	���ϵ�����
		*		���Ǵ��±�0��ʼ���������ѭ����һ�εȴ��źţ�Ȼ������źŵ���С�±꿪ʼ
		*		�ͱȴ�0��ʼЧ�ʸ�����һ��
		*	
		*		�����ԵĹ�ƽ��ֻ����Թ�ƽ���������
		*		�ô����һ��ѭ���¶��ܵõ�����
		*		���ǲ�������ȫ���˳�����⣬ֻ�Ǵﵽ��Թ�ƽ
		*		���ԣ��¼�ѡ��ģ�Ͳ������ڴ��û��������
		* 
		* ע�⣺
		*		WSAWaitForMultipleEvents�������ڲ�ѭ���У�Ҫ�޸Ĳ���
		*		�����ǵ�4����������Ҫ�ú꣬����0������Ч���֣�����Ϊ��λ���ĵȴ��¼�
		*		�ú�Ļ����ͷ�Ҫ�ȵ�һ���¼����źŲ��ܼ������ͳ�����ɵ������
		*		
		*		
		*/
		//4.12 �����¼�������������� ����Ҫ��
		/*
		*	//�����Ƚϼ򵥣����ǻ��ǿ�����ṹ�Ƚ�������
		*	  ��һ��û��˵�̵߳Ĵ���ʽ������Ҫ���Ƿ��������˼�룩������Ҫ������һ�㣬��һ�ε�����
		* 
		* �¼���������ޣ�
		*		�ȴ��źź���WSAWaitForMultipleEvents��һ�����ֻ�ܴ�64�������������ǲ���ϵͳֻ��Ͷ64��
		*		����������������ǵĿͻ�������������Ҫ��취ͻ���������
		* 
		* ����1��
		*		�ô����飨�޸Ľṹ�弯�ϵ����ޣ���һ�θ��ȴ��źŴ���1����Ȼ��ѭ����ÿ��ѭ��ֻ��һ����Ȼ��Ҫ���٣���ѭ�����Σ��Ǿͽ����һ��ֻ�ܴ�64��������
		*		����̳߳أ��ݲ����ɶ��˼��
		* 
		* ����2��
		*		һ��һ����������øĽṹ�壬���ýṹ��������ѭ���Ľ�ϣ�ÿ��ѭ����һ���ṹ�壨����һ��ѭ������������⣩
		*		�´�ѭ������һ���ṹ�壬�ͽ�����������⣬�����ֵ��߳�����߳�
		*	���̣߳�
		*		���̣߳�һ��һ��˳����ͺ���
		*	���߳�
		*		ÿ���ṹ�嶼��һ���̣߳���ÿ���̶߳������64���Ϳ�����
		*		��������̣߳�ÿ���̴߳���һ���¼������64
		*	
		*		
		*	ע�⣺
		*		���÷���2ʱ���пͻ��������¼���������ӿͻ��˵�socket���¼�ʱ
		*		Ҫע�⵱ǰ�ṹ���Ƿ����ˣ�������һ��Сѭ����ӵ��п��еĽṹ����
		*		��Ϊ�����˽ṹ�����飬����һЩԪ�ؽṹ��Ϊ0��������WSAWaitForMultipleEvents�����Ĳ����淶��
		*		��������Ҫ�����жϣ���WSAWaitForMultipleEvents����ǰ
		* 
		*	��ͬ�ķ����ͷ�socket������ķ�ʽ�ǲ�һ����
		*		����1Ҫ��ѭ��
		*		����2Ҫ��˫��ѭ��
		*	ע��
		*		��Ϊ�˰Ѹ��ַ����ֿ�������{}
		*		����Ҫ�ر�ע�⣬����{}����{}�ڵı������Ǿֲ�����
		*		�������ͷ�ʱ��Ҳֻ����{}���ͷţ����Ҫע�⣬��{}���޷�ʶ��ֲ�����
		* 
		* 
		*/
		//4.13 selectģ�����¼�ѡ��ģ�͵�����
		/*
		* ��ʦ��һ��ͼ���Ƚ����������ǿ��Կ�һ�¶Աȣ���42��ʱ
		* 
		* select���¼�ѡ��Ĺ�ͬ��
		*		�ڷ��ദ���Ժ���һ����
		*		���ǵ�����غ��������н������ӡ����͡�������Ϣ���ͻ������ߴ���
		* 
		* select���¼�ѡ��Ĳ�ͬ�㣺
		*		
		*	�¼�ѡ��
		*		���¼���socket��Ͷ�ݸ�����ϵͳ���ɲ���ϵͳ���Ӹ��ź�
		*		Ȼ���¼�ѡ��Ͷ�ݺ���Ϊ�첽�ģ�����ϵͳ���в�Ӱ�����ǳ���
		*		���ǿ����ڳ��������κ���
		*		ֱ�����ǵȴ��źź����õ��¼����±�
		*		Ȼ��ʼ������WSAEnumNetworkEvents�õ���Ӧ�¼��Ĳ����룬������֪��Ӧ����ʲô
		*		��Ȼ��ͽ��з��ദ��������غ����������ӡ����͵Ȳ�����
		*		ִ��send recv ����Э�黺�������и��ƶ�д�Ĺ����У���������
		*	select��
		*		��socketͨ��select�������ݸ�����ϵͳ��������������飩��ϵͳ�ᰤ������������������Ӧ��socketװ������2��3��
		*		ִ��select�Ĺ�������ͬ���ģ����򲻿������������飬��������
		*		Ȼ��õ�����2��3�е�����Ӧ��socket��Ȼ����з��ദ��
		*		ִ��send recv ����Э�黺�������и��ƶ�д�Ĺ����У���������
		*		select����2��������ȥ��socket�����У����յ�����������򷵻ص�������������Ǿ�֪��Ҫ����accept��
		*		select����3��������ȥ��socket�����У����յ���Ϣ������򷵻ص�������������Ǿ�֪��Ҫ����recv��
		*/
	}
}
	
	
	