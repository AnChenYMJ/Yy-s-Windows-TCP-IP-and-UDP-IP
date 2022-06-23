#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//����socket���¼��ŵ�ȫ�֣��������˳��������ã�ע���������ﲻҪ��д�����ˣ����ж�����
SOCKET SerVerSocket;//socket
WSAEVENT wsace;		//�¼�

//����˳���������
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerVerSocket);
		WSACloseEvent(wsace);
		WSACleanup();
		return TRUE;
		break;
	}
	return FALSE;
}

int main(void)
{
	//��ϵͳͶ��һ�����ӣ�����˳��ļ��ӣ�
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	//�������
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nrse = WSAStartup(WSVersion, &wsaData);
	if (0 != nrse)
	{
		int a = WSAGetLastError();
		printf("WSAStratup error: %d\n", a);
		system("pause>0");
		return 0;
	}
	//У��汾
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("�����汾����ȷ error: %d\n", a);
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
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//�����¼�����
	wsace = WSACreateEvent();
	if (WSA_INVALID_EVENT == wsace)
	{
		int a = WSAGetLastError();
		printf("�����¼����� error: %d\n", a);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//��socket������룬��Ͷ��
	int wsaes = WSAEventSelect(SerVerSocket, (HANDLE)wsace, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaes)
	{
		int a = WSAGetLastError();
		printf("���¼���socket��Ͷ�� error: %d\n", a);
		WSACloseEvent(wsace);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//�ȴ��ź�Ȼ����з��ദ��
	while (1)
	{
		//�ȴ��źţ�ѯ���źţ�
		DWORD wsawfmpe = WSAWaitForMultipleEvents(1, &wsace, TRUE, WSA_INFINITE, FALSE);
		//�ȴ���ʱ��
		if (WSA_WAIT_TIMEOUT == wsawfmpe)
		{
			printf("ѯ���źź�����ʱ��\n");
			continue;
		}
		//��������������
		if (WSA_WAIT_FAILED == wsawfmpe)
		{
			int a = WSAGetLastError();
			printf("ѯ���źź��� error: %d", a);
			break;
		}

		//�о��¼�����ȡ��Ӧ�¼������ͣ����崥�����������������ź�����
		struct _WSANETWORKEVENTS wsanet;
		int wsaene = WSAEnumNetworkEvents(SerVerSocket, wsace, &wsanet);
		if (SOCKET_ERROR == wsaene)
		{
			int a = WSAGetLastError();
			printf("ѯ���źź��� error: %d", a);
			break;
		}

		//�¼����ദ��
		//�ж�FD_READ
		if (wsanet.lNetworkEvents & FD_READ)
		{
			if (wsanet.iErrorCode[FD_READ_BIT] == 0)
			{
				//recvfrom����
				struct sockaddr stclient;	//����5�����ڼ�¼�Է�socket���ͻ��ˣ��Ķ˿����ַ
				int nlen = sizeof(stclient);//����6����5�ṹ���С
				char cbuf[548] = { 0 };		//��2���ַ����飬��Ž��յ�����Ϣ
				int rnum = recvfrom(SerVerSocket, cbuf, 548, 0, &stclient, &nlen);
				if (SOCKET_ERROR == rnum)
				{
					int a = WSAGetLastError();
					printf("recvfrom error: %d\n", a);
				}
				printf(" %s\n", cbuf);
				//sendto������Ϣ
				int snum = sendto(SerVerSocket, "Server �յ�\n", strlen("Server �յ�\n"), 0, (const struct sockaddr*)&stclient, sizeof(stclient));
				if (SOCKET_ERROR == snum)
				{
					int a = WSAGetLastError();
					printf("recvfrom error: %d\n", a);
				}
			}
			else
			{
				printf("FD_READ_BIT error code:%d\n", wsanet.iErrorCode[FD_READ_BIT]);
				continue;
			}
		}
	
		//�ж�FD_WRITE
		if (wsanet.lNetworkEvents & FD_WRITE)
		{
			if (wsanet.iErrorCode[FD_WRITE_BIT] == 0)
			{
				printf("FD_WRITE�Ѳ��������Է�����\n");
			}
			else
			{
				printf("FD_WRITE_BIT error code:%d\n", wsanet.iErrorCode[FD_WRITE_BIT]);
				continue;
			}
		}
	}

	//�ͷ�socket���ر������
	WSACloseEvent(wsace);		//����˳�����ҲҪ��������ͷ��¼������
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}


void studybook13(void)
{
	//4.1 �¼�ѡ��ģ��Ҫ�㡢�߼�����
	{
			//1.windows�����û���Ϊ�����ַ�ʽ
		/*
			windows�����û���Ϊ�����ַ�ʽ��
					1.��Ϣ���ƣ����ģ���Ϣ����
					2.�¼����ƣ����ģ��¼�����
			�û���Ϊ���������ǰ��¼����ϵĶ�Ӧ��ť������������ˣ�����ƶ��ˣ�����϶���һ��ʲô��Щ�����û���Ϊ

			��Ϣ���ƣ�
				���ģ���Ϣ����
				�ص㣺����ȫ��
					����
						��Ϣ�������ɲ���ϵͳ������ά���ģ����ǲ�����Ϊ��������ϵͳ���ӵ�������Щ��Ϊ����װ����Ϣ������
						���е��ص����Ƚ��ȳ�����ô����ʱ�ȴ�������Ϊ�ͻ�����װ�����в��������������
					ȫ�棺
						����ϵͳ���Ԥ�ȶ�����ˣ�������̣����ȵ���Ϊ����Ϣ����������Ҫ��һ����Ϸ�õ�wasd��
						�������ʱ������yuio����������Ϸ����ɣ�������Ȼ�������Ϣ������ŵ���Ϣ������
						���������ǴӶ���ȡ����Ϣ���������ʱ��ɸѡ������������Ҫ��
			�¼����ƣ�
				���ģ��¼�����
				�ص㣺���򡢲�ȫ��
					����
						�¼������У�����ϵͳ��û�д�����ʲô������Ҫ�ֶ�����Ϊ���¼������ֶ������¼�����
						�¼�����Ҫ��������ȡ����Ӧ���¼������±�С�Ŀ�ʼ��������ô���������У��±�С�Ŀ����Ǻ���Ӧ�ģ��±���������Ӧ��
						������Ϊ�����ľ��ޣ�ֻ�����ȴ����±�С�ģ������¼������������
					��ȫ�棺
						����ϵͳ��û��Ԥ�ȸ���Ϊ���¼������������ֶ�����һ���¼�������һ����ΪͶ�ݸ�����ϵͳ
						�����ǲ�ȫ��ģ�û�д����¼�������Ϊ����û�������Ӧ��Ϊ���¼�����
						
			��Ϣ���ƣ����ģ���Ϣ����
					�������
						���е��û��������������꣬�����̣�������ϵİ�ť....�ȵȣ����в��������ΰ�˳�򱻼�¼��װ��һ������
					�ص�
						��Ϣ�����ɲ���ϵͳά�����������Ĳ�����Ȼ�����Ϣȡ���������ദ��
						���Ⱥ�˳��
					����
						����֮��̴��win32,MFC�γ̣��ⲿ�ֶ��ǻ��������Ϣ���У���������ϸ���ܣ������ʱ�Ӻ�����
						�����¸�ģ�ͣ��첽ѡ����ǻ��������Ϣ��

			�¼����ƣ����ģ��¼�����
					�������
						������������Ϊ�û����ض�������һ���¼����¼��������Լ�����API��������Ҫ���ٴ�������
						���¼�Ͷ�ݸ�ϵͳ��ϵͳ�Ͱ����Ǽ����ţ����Բ������޴�����̫��ϵͳ���оͿ���
						������������ˣ������û�������ˣ���ô��Ӧ���¼��ͻᱻ�ó����źţ�Ҳ��������1��2�ˣ��ø������aaa
						����ֱ�ӻ�ȡ�����źŵ��¼���Ȼ����
					�ص�
						�����¼����������Լ�����ģ�ϵͳֻ�ǰ������������źţ����������Լ��ƹܶ���....
						�����
					����
						��������¼�ѡ�񣬾���Ӧ�����
		*/
			//2.�¼�ѡ����߼�
		/*
			�¼�ѡ����߼�
				Ϊ���ǵ�socket����һ���¼���������ֻ��һ��socket,Ͷ�ݸ�����ϵͳ�����Ͱ������첽�����ţ����пͻ��˷�������Ϣ�����¼��ͻᱻ�ó����źţ���ô���ǻ�ȡ�����źŵ��¼������Ĵ���
				��һ��
					����һ���¼����󣨱�������
						WSACreateEvent
				�ڶ���
					���¼���socket�󶨵�һ��ָ���������ͣ�recvfrom��sendto����Ͷ�ݸ�ϵͳ��
						WSAEventSelect���������;�����accept��close��recv��send�ȣ�
				������
					ѭ���ȴ��¼������ź�
						WSAWaitForMultipleEvents
						�ú����ȴ����̽������̹߳����ͷ�cpuʱ�䣨�����ڵȴ���ʱ�����̹߳һ��ȴ�����ռ��cpuʱ�䣩
						�Ա�select��select�����ȴ���ʱ��ռ�������̵߳�ʱ��ģ��������ģ�����������ǲ�ռ��ʱ���
				���Ĳ�
					���źŵĻ��ͷ��ദ��
						WSAEnumNetworkEvents

			�¼�ѡ��ģ�����̣�
					1.�������⣬����ͷ�ļ�
					2.�������
					3.У��汾
					4.����socket
					5.�󶨶˿ںš���ַ
					6.�¼�ѡ��
		*/
	};
	//4.2 �����¼����󲢰�Ͷ��
	{
		/*
			�����¼�����
				WSAEVENT WSAAPI WSACreateEvent();
					�ɹ�������һ���¼�
					ʧ�ܣ�����WSA_INVALID_EVENT
				�ر��¼���ʽ��
					::closesocket(socketServer);
					::WSACleanup();
				�¼�ת���壺HANDLE�������ID���ں˶���
					��ϵͳ���ں����룬�ɲ���ϵͳ���ʣ����ǲ��ܶ�λ�����ݣ�Ҳ�����޸�
						������void*��ͨ������ָ��
						ͬ���ָ���Ƕ��ں˵ı������Թ���ı������Ӷ�ʹ����ϵͳ�����ƽ�ȵģ���Ч�����У���������������
					���ú������������ú����ͷ�
						�������û�е����ͷţ���ô�����ܾ�һֱ�������ںˣ�����ں��ڴ�й©�� ����ֻ����������
					�ں˶����У�socket��Kernel Objects
				������غ�����
					BOOL WSAAPI WSACloseEvent( WSAEVENT hEvent);
						�ر�/�ͷ��¼���������þ�Ҫ�ͷ�
					BOOL WSAAPI WSAResetEvent( WSAEVENT hEvent);
						����WSAEventSelect����ʹ�õ��¼�����״̬����ȷ�����ǽ��¼�����ľ�����ݸ�hEventObject�����е�WSAEnumNetworkEvents������ �⽫�����¼�������ԭ�ӷ�ʽ�����׽����ϻFD�¼���״̬��
					BOOL WSAAPI WSASetEvent( WSAEVENT hEvent);
						��ָ���¼������ó����ź�
					���������������ã�����һ��Ҫ֪�����õ�ʱ�������ȷʹ��



			�󶨲�Ͷ�ݣ�
				����ԭ�ͣ�
					int WSAAPI WSAEventSelect
					(
					  SOCKET   s,
					  WSAEVENT hEventObject,
					  long     lNetworkEvents
					);
				���ܣ�
					���¼�����socket������룬��Ͷ�ݸ�����ϵͳ
				����1��
					���󶨵�socket������Ҫ��Ч����ÿһ��socket������һ���¼�
				����2��
					�¼������߼������ǽ�����1�����2����һ��
				����3��
					������¼�������¼��ð�λ�� | �����У����鿴�μ�����TCP���¼�ѡ��ģ�ͱʼ�
						FD_READ���пͻ��˷�����Ϣ��
						FD_WRITE�����Ը��ͻ��˷��ţ���accept��������ź�
						FD_ACCEPT��FD_CLOSE��FD_CONNECT���ͻ���ʹ�ã���UDP�ò���
						0��ȡ���¼����ӣ�WSAEventSelect��0����
						FD_OOB���������ݣ�����˵�ˣ�һ�㲻ʹ��
						FD_QOS���������������仯֪ͨ����������ʱ����Ӱ������ķ���ļ�¼
						FD_GROUP_QOS�������ã���ʱû�й�Ч
						�ص�I/Oģ�����õ��ģ�FD_ROUTING_ INTERFACE_CHANGE��FD_ADDRESS_ LIST_CHANGE
				����ֵ��
					�ɹ�������0
					ʧ�ܣ�����SOCKET_ERROR
			�ܽ᣺
				�������¼�һ��Ҫ�ֶ��ͷţ�����˳�������Ҳ���ϣ������¼��Ĳ�����UDP����������ӵģ�û�пͻ���socket��û�����ӣ�accept��close�ò���
				���Ǿ���	FD_READ��FD_WRITE�շ��ľͿ����ˣ�TCP�ķ�����socket�ǰ�accept
				
				�¼�ѡ��ģ���� �¼��Ǻ��ģ�������������¼������¼�����Ҫ������socket���¼��ͷŲ��ر������
		*/
	};
	//4.3 �ȴ��źŲ����ദ��
	{
			//�ȴ��źţ�Ҳ��ѯ���źţ�
		/*
			����ԭ�ͣ�
				DWORD WSAAPI WSAWaitForMultipleEvents(
					  DWORD          cEvents,
					  const WSAEVENT *lphEvents,
					  BOOL           fWaitAll,
					  DWORD          dwTimeout,
					  BOOL           fAlertable
					);
		
			���ã�
				�ȴ��¼����źŲ���
			����1��
				�¼��ĸ�����һ����ߴ���64����WSA_MAXIMUM_WAIT_EVENTS�������Ա���ڽ�β�ܽ��ٽ�
			����2��
				�¼��б��������¼����ϣ�Ҳ���������飬һ������������
			����3��
				�¼��ĵȴ���ʽ��
					TRUE:���д�����¼��������źŲŷ���
					FALSE������һ���¼������źţ����������أ����ҷ���ֵ��һ����������� ��ȥ WSA_WAIT_EVENT_0 == �������¼����±�
			����4��
				��ʱ�������select����5���ƣ�
				123���ȴ�123���룬��ʱ����WSA_WAIT_TIMEOUT
				0���������أ����������ź�
				WSA_INFINITE��һֱ�ȴ���ֱ�����¼�����
			����5��
				TRUE���ص�IOר��ʹ�õ�
				FALSE�����ص�IO�⣬�������

			����ֵ
				�ɹ���
					�����±������ֵ��
							����3Ϊtrue
								���е��¼������ź�
							����3Ϊfalse
								����ֵ��ȥWSA_WAIT_EVENT_0==�������¼����±�
					WSA_WAIT_IO_COMPLETION��
							����5ΪTRUE���Ż᷵�����ֵ
					WSA_WAIT_TIMEOUT��
							��ʱ�ˣ�continue����
				ʧ�ܣ�
					WSA_WAIT_FAILED


		*/
			//�о��¼�
		/*
			����ԭ�ͣ�
					int WSAAPI WSAEnumNetworkEvents(
						  SOCKET             s,
						  WSAEVENT           hEventObject,
						  LPWSANETWORKEVENTS lpNetworkEvents
						);
			���ã�
				��ȡ�¼����ͣ������¼��ϵ��ź����ã��������źŹ��ܵĺ�������WSAResertEvent��
			����1��
				��Ӧ��socket
			����2��
				��Ӧ���¼�
			����3��
				�������¼����ͣ��ɺ�����ȡ����Ȼ��װ�����������
				��һ���ṹ��ָ�룺
						struct _WSANETWORKEVENTS{long lNetworkEvents;int  iErrorCode[FD_MAX_EVENTS];} 
						��Ա1�����������һ���źſ��ܰ���������Ϣ���԰�λ�����ʽ����
						��Ա2�����������飬FD_ACCEPT�¼��������������ֵΪFD_ACCEPT_BIT���±��Ϊ0��û�д���
			����ֵ��
				�ɹ�������0
				ʧ�ܣ�����SOCKET_ERROR
		
		
		*/
			//�¼����ദ��
		/*
			���ദ���߼���
				�ɲ鿴TCP�ıʼǣ�������ʦ�ö��if��������if else ��������switch
				���ڽ��������������ͬʱ�������Ұ�λ������

			�����¼��ź���UDP�������
				TCP�У�һ�������������ϲ���accept���źţ����Ӻ��������write���ź�
				UDP��������ӣ�û��accept���ӣ���ôwrite�Ĳ��������Ƿ�����һ������ʱ����������

			����ʱ�ܽ᣺
				1���ȴ��źź���һ���������64����Ϊ��ͻ��������ޣ������ַ�ʽ��
					1.һ��һ���Ĵ�����һ��ѭ���У������һ�㣬�������1�������һ�����α����������ܴ���ܶ�ܶ࣬����Ӧ�������ƽ��һ��
					2.һ��һ��Ĵ���һ�����64���Ǿͷֶ�Σ���ε����������������������е��鷳
					3.���ö��̣߳�ÿ���߳��и�һ�飨64����ȥ����
				2��������
					1.TCP/IP������socket�ܶ࣬����Ҫ������ص�������
					2.UDP/IP��һ��socket���Ͳ��ÿ���˳��������
				3�������¼�����
					1.TCP/IP������socket�ܶ࣬ÿ��socketҪ��һ���¼�����������Ҫ���¼������ܶ����������߼�����
					2.UDP/IP��һ����û���¼�����
		*/
	};
}