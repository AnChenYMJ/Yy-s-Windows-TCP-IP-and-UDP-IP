#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <ws2tcpip.h>
#include <Mswsock.h>		//AcceptEx��ͷ�ļ�	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <tchar.h>
#include <Winsock2.h>				//Ҫ��win32���ڵ���Ŀ�У��ǵ��޸�
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

//����socket���顢�ص�IO�ṹ������
#define MAX_COUNT 10240
SOCKET g_sockall[MAX_COUNT];	//socket����
OVERLAPPED g_olpall[MAX_COUNT];	//�ص�IO�ṹ������
int g_count = 0;				//����

//PostAcceptͶ�ݺ����������߼���װ
int PostAccept();
//WSArecv�����������߼���װ
int PostRecv(int Index);
//WSASend�����������߼���װ
int PostSend(int Index);
//WSArecv�Ĳ���2Ҫʹ�õ�,recv���������ݻ�浽������
#define MAX_RECV_COUNT 1024
char g_recv[MAX_RECV_COUNT];		//ȫ�ֱ������Զ���ʼ����0
//WSASend�Ĳ���2Ҫʹ�õ�,sendд�ĵ����ݻ�浽������
#define MAX_SEND_COUNT 1024
char g_send[MAX_SEND_COUNT];		//ȫ�ֱ������Զ���ʼ����0

//�������麯��
void Clear();

int main(void)
{
	//�������
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA WSsockMsg;
	int nrse = WSAStartup(WSVersion, &WSsockMsg);
	if (0 != nrse)
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
	//У��汾
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//����socket
	SOCKET SerVerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//�󶨶˿����ַ
	struct sockaddr_in st;
	st.sin_family = AF_INET;
	st.sin_port = htons(22258);
	st.sin_addr.S_un.S_un_b.s_b1 = 127;
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	if (SOCKET_ERROR == bind(SerVerSocket, (const struct sockaddr*)&st, sizeof(st)))
	{
		int a = WSAGetLastError();
		closesocket(SerVerSocket);
		WSACleanup();
		return 0;
	}
	//��������
	if (SOCKET_ERROR == listen(SerVerSocket, SOMAXCONN))
	{
		int a = WSAGetLastError();
		closesocket(SerVerSocket);
		WSACleanup();
		return 0;
	}

	//�ص�IO��ʼ��
	//��ʼ�����ǵ�socket���ṹ������
	g_sockall[g_count] = SerVerSocket;
	g_olpall[g_count].hEvent = WSACreateEvent();	//һ��Ҫ�ֶ����¼���ϵͳ�������������һ��
	g_count++;
	//Ͷ��AcceptEx��ʹ�������Զ���ĺ�����Ͷ�ݣ�
	int Aer = PostAccept();
	if (0 != Aer)
	{
		Clear();
		WSACleanup();
		return 0;
	}
	//û��������ɣ������ѭ���ȴ��ź�
	while (1)
	{
		for (int n = 0; n < g_count; n++)
		{
			//ѯ���ź�
			DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[n].hEvent, FALSE, 0, FALSE);
			if (wfmte == WSA_WAIT_FAILED || wfmte == WSA_WAIT_TIMEOUT)
			{	//������  ��  �ӳٴ���
				continue;	//û�ȵ�����һ��ѭ��������
			}
			//�ȵ����źţ���ȡsocket�������
			DWORD dwState;	//����3
			DWORD lpflags;	//����5
			BOOL wolr = WSAGetOverlappedResult(g_sockall[n], &g_olpall[n], &dwState, TRUE, &lpflags);
			if (FALSE == wolr)
			{
				int a = WSAGetLastError();
				//ǿ�����ߣ�������Ϊ10054
				if (10054 == a)
				{
					printf("�ͻ���%dǿ��close��\n", g_sockall[n]);
					//ɾ�����߿ͻ��˵�socket���¼�
					closesocket(g_sockall[n]);
					WSACloseEvent(g_olpall[g_count - 1].hEvent);
					//��ĩβԪ����䵽ɾ���Ŀͻ��˵�λ��
					g_sockall[n] = g_sockall[g_count - 1];
					g_olpall[n] = g_olpall[g_count - 1];
					//ѭ�����Ʊ���
					n--;		//��ΪĩβԪ�ص��˵�ǰ��Ԫ��λ�ã���������Ҫn-1 ,�����������ǰ��ֵ������socket���¼���
					g_count--;	//����-1
				}
				continue;
			}
			//�����źţ���ʦ���˼ӣ�����BUG��
			BOOL ChongZhi = WSAResetEvent(g_olpall[n].hEvent);
			if (FALSE == ChongZhi)
			{
				int a = WSAGetLastError();
				continue;
			}
			//���ദ��
			//n == 0 ʱ���źţ�0�Ƿ�����socket����˵��������������Ҫaccept
			if (0 == n)
			{
				//������������ˣ���ΪͶ�ݸ�����ϵͳ������ϵͳ����ˣ�
				printf("accept �ͻ���%d���ӳɹ� \n", g_sockall[g_count]);
				//��ϵͳͶ��recv����Ϊ����������
				PostRecv(g_count);		//ʹ��g_count����Ҫ��n����Ϊn�ǵ�ǰ�ķ���������g_count�����¿ͻ��˵�
				//�������Ͷ��send
				PostSend(g_count);
				//�ͻ�������+1
				g_count++;
				//����Ͷ��accept����Ϊ���������˵����֮ǰ��accept�Ѿ����
				PostAccept();
				//����ѭ������ΪnΪ0��ʱ�򣬻�������ִ�У��ͻ�ѿͻ��˸��رյ�
				continue;
			}
			//�ͻ������ߣ�������������ߣ�ǿ�������ڴ������
			if (0 == dwState)
			{
				printf("�ͻ���%d����close��\n", g_sockall[n]);
				//ɾ�����߿ͻ��˵�socket���¼�
				closesocket(g_sockall[n]);
				WSACloseEvent(g_olpall[g_count - 1].hEvent);
				//��ĩβԪ����䵽ɾ���Ŀͻ��˵�λ��
				g_sockall[n] = g_sockall[g_count - 1];
				g_olpall[n] = g_olpall[g_count - 1];
				//ѭ�����Ʊ���
				n--;		//��ΪĩβԪ�ص��˵�ǰ��Ԫ��λ�ã���������Ҫn-1 ,�����������ǰ��ֵ������socket���¼���
				g_count--;	//����-1
			}
			//����/���ճɹ���
			if (0 != dwState)
			{
				//�ж�recv�Ƿ�浽�˶������浽����˵��recv�ɹ�����ʼ����recv
				if (0 != g_recv[0])
				{
					//��ӡ��Ϣ
					printf("recv: %s\n", g_recv);
					//��ȫ�ֱ���str����
					memset(g_recv, 0, MAX_RECV_COUNT);
					//�������Ͷ��send
					//�����Լ�����������WSARecv���ȴ�����Ϣ����
					PostRecv(n);
				}
				//�������recv��������Ϣ
				else
				{
					////�������Ͷ��send
					//PostSend(n);
				}
			}
		}
	}
	//���������������(socket �� event(�¼�))
	Clear();
	WSACleanup();

	system("pause>0");
	return 0;
}

//PostAcceptͶ�ݺ����������߼���װ
//�����򷵻�0�����򷵻ش�����
int PostAccept()
{
	//����2���ͻ���socket����Ҫ�ֶ�����
	g_sockall[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == g_sockall[g_count])
	{
		int a = WSAGetLastError();
		return 0;
	}
	g_olpall[g_count].hEvent = WSACreateEvent();	//��ʱ�����±�+1����Ϊ��ûȷ�Ͽͻ������ӳɹ�������Ҫ��������Ϣ��+1
	//����3��������NULL������4��0��ȡ������3����
	char str[1024];
	//����7��������NULL��Ҳ������DWORD
	DWORD DwRecvCount;
	//tͶ�ݷ�����socket���첽�Ľ�������
	BOOL acptex = AcceptEx(g_sockall[0], g_sockall[g_count], &str, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &DwRecvCount, &g_olpall[0]);
	//Ͷ�ݺ����������
	if (acptex == TRUE)
	{
		//Ͷ��WSArecv�����տͻ�����Ϣ
		PostRecv(g_count);
		//�������Ͷ��send
		//�ͻ�������++
		g_count++;		//ֻ��ȷ���˿ͻ������ӳɹ����ҽ�������Ϣ���ü����±���Ч����ʦ�ķ���,��ʦ��Recv����Ҫ���ͻ��˵��±꣬�������ڲ�+1��
		//Ͷ��AcceptEx��Ҫ�������ӿͻ��˾ͱ�������Ͷ�ݣ�
		PostAccept();	//�ݹ�ķ�ʽ���ɸ��÷ǵݹ飨ѭ�������������continue����ѭ�����ӳٴ���������break��
		return 0;
	}
	//Ͷ�ݺ�û��������ɣ�Ҳ���ܳ�����
	else
	{
		int a = WSAGetLastError();
		//��ʱû�пͻ�������,�����ӳٴ���
		if (a == ERROR_IO_PENDING)
		{
			return 0;
		}
		//������
		else
		{
			return a;	//�����򷵻ش�����
		}
	}
}
//WSArecv�����������߼���װ
int PostRecv(int Index)
{
	//����2
	struct _WSABUF buff;
	buff.buf = g_recv;			//str�Ѿ���ȫ����������
	buff.len = sizeof(g_recv);

	DWORD count;				  //����4
	DWORD DwFlag = MSG_PARTIAL;   //����5
	int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], NULL);
	//��������
	if (0 == nrecv)
	{
		//��ӡ��Ϣ
		printf("recv: %s", buff.buf);
		//��ȫ�ֱ���str����
		memset(g_recv, 0, MAX_RECV_COUNT);
		//�������Ͷ��send
		//�����Լ�����������WSARecv���ȴ�����Ϣ����
		PostRecv(Index);
		return 0;
	}
	//�ӳٴ�����߳�����
	else
	{
		int a = WSAGetLastError();
		if (WSA_IO_PENDING == a)
		{
			return 0;
		}
		else
		{
			return a;
		}
	}
}
//WSASend�����������߼���װ
int PostSend(int Index)
{
	//����2
	struct _WSABUF buff;
	buff.buf = g_send;			//str�Ѿ���ȫ����������
	buff.len = sizeof(g_send);
	//����Ҫ��������
	int s = scanf("%s", buff.buf);

	DWORD Sendcount;					//����4
	DWORD SeDwFlag = MSG_PARTIAL;		//����5
	int nSend = WSASend(g_sockall[Index], &buff, 1, &Sendcount, SeDwFlag, &g_olpall[Index], NULL);
	//��������
	if (0 == nSend)
	{
		//��ӡ��Ϣ
		printf("����ͻ��˷���\n");
		//��ȫ�ֱ���str����
		memset(g_send, 0, MAX_RECV_COUNT);
		//���ü���Ͷsend�ˣ���Ҫ��ʱ����send
		return 0;
	}
	//�ӳٴ�����߳�����
	else
	{
		int a = WSAGetLastError();
		//�ӳٴ���
		if (WSA_IO_PENDING == a)
		{
			//��ӡ��Ϣ
			printf("����ͻ��˷���\n");
			//��ȫ�ֱ���str����
			memset(g_send, 0, MAX_RECV_COUNT);
			//���ü���Ͷsend�ˣ���Ҫ��ʱ����send
			return 0;
		}
		else
		{
			return a;
		}
	}
}
//��������
void Clear()
{
	for (int i = 0; i < 20; i++)
	{
		closesocket( g_sockall[i]);	//socket����
		WSACloseEvent(g_olpall[i].hEvent);	//�ص�IO�ṹ������
	}
	g_count = 0;
}

//ѧϰ�ʼ�
void studybook()
{
	//��ʦ˵��������е��ѣ�����˵�򵥵��𣿣��������߼��ϼ򵥣�
	//��֮�Ҿ������ܶ����⣬�����˺ܶ�ʱ��
	//��ʦ˵���ϵĴ�������ĸ��ӣ���Ϊ���˺ܶ��Ż�������ֻ��д��������Ĺ��ܣ���������ѧϰ������һЩBUG

	//6.1 �ص�IOģ��֪ʶ���
	{
/*
			* �ص�IOģ�����壺
			*		�ص�IO��windows�ṩ��һ���첽��д�ļ��Ļ���
			* ���ã�
			*		socket�ı��ʾ����ļ�����
			*		������д�ļ�����recv��sendִ��ʱ�������ģ�����ִ�б��recv��send
			*		recvҪ�ȴ�Э�黺����������ȫ�����Ƶ����ǵ�buf���Զ������飩�У��������ܽ����������ظ��Ƶĸ���
			*		sendҲ��һ���ģ�ִ��send�Ĺ����У���Ķ�д����������ִ��
			*
			*		�ص�IO��������recv����ָ������ǵ�buf���Զ������飩Ͷ�ݸ�����ϵͳ��Ȼ����ֱ�ӷ���
			*		����ϵͳ�������һ���̣߳������ݸ��Ƶ����ǵ�buf�У�������ƹ����ǲ���ϵͳִ�еģ������ǵĳ����޹�
			*		�����Ļ������Ǿ����ڲ���ϵͳ���ƹ������������£��������ᵢ�󣬾����� ��д ����첽����ͬʱͶ�ݶ����д����
			*
			*		���ǽ���accept��recv��send�Ż������첽������������AcceptEx WSARecv WSASend������
			*		���IOģ�;��Ƕ�C/S����ģ�͵�ֱ���Ż�
			*
			* ���ʣ�
			*		����һ���ṹ��ı������ñ�����socket��
			*
			*		�ṹ�壺WSAOVERLAPPED
			*			��Ա��ǰ�ĸ���Ա1��2��3��4�ǲ���ϵͳ��ʹ�õģ��������޹أ��Ƿ����ṩ�̵Ŀ�����Ա����������ߣ�������ʹ�õ�
			*			���һ����Ա5���¼�����������Ҫ��ע�ģ�������ɺ����ͻᱻ�ó����ź�
			*
			* ʹ�ã�
			*		�첽ѡ��ģ�ͣ����ǰ�socket����Ϣ����һ�𣬲���ϵͳ����Ϣ��������������
			*		�¼�ѡ��ģ�ͣ����ǰ�socket���¼�����һ�𣬲���ϵͳ���¼���������������
			*		�ص�IOģ��  �����ǰ�socket���ص��ṹ���ṹ�壩����һ�𣬲���ϵͳ���ص�IO��������������
			*	���ֶ���socket�󶨺�Ͷ�ݸ�����ϵͳ
			*
			* �ص�IO�ķ������ַ�ʽ��
			*		1���¼�֪ͨ
			*		2��������̣��ص�����
			*
			* �ص�IO�Ļ����߼���	���ַ�ʽ�����Ե���ʹ�ã�Ҳ�������ʹ��
			*		�¼�֪ͨ��
			*				����AcceptEx WSARecv WSASendͶ��
			*				����ɵĲ��������ó����źŵ�
			*				��ȡ�źţ�����WSAWaitForMultipleEvents��ȡ�¼��ź�
			*		�ص�����/������̣�	���ص�����Ҳ��������̣�
			*				����AcceptEx WSARecv WSASendͶ��
			*				��ɲ��������ó����źź�ͻ��Զ����ûص�����
			*
			* ���ܣ�
			*		�������˲��ԣ���һ̨6000Ԫ���ҵ��ԣ�20000���ͻ���ͬʱ�������ӣ�������Ϣ��
			*		ʹ���ص�IOģ�ͣ�ϵͳ��CPUʹ��������40%���ң�
			*		ʹ����ɶ˿�ģ�ͣ�ϵͳ��CPUʹ��������2%����
			*		������ɶ˿�ģ�Ͳ������������Ƶģ��ܺ��õ�ģ�ͣ�����õ�ģ��
			*	������ģ��Ҳ�������ô��ͻ��ˣ�������Ϊͬ�������������ӳٺܸ�
			*
			* ���䣺
			*		�ص�IO�ǽ�ÿ��ִ�������Ĳ�����recv�ȣ�����һ���̣߳���������CPU�̲߳������޵�
			*		������һ����ֵ5000 ���̵߳���5000ʱ��Ч���ǻ������ģ����Ǵ���5000��Ч�ʾͻ����
			*		��Ϊ�߳�Ҳ����Ҫ�л��Ȳ�����
			*
			*		��ǧ����Ŀͻ��ˣ�����Ҳ���ص�IO����ɶ˿��ܱ�����ģ�ͺõö���
			*		���ص�IOռ��CPUʹ�����е��
			*
			*		�ص�����Ҳ���������
			*		�ص�IO�Ļ����߼����ַ�ʽ�����Ե���ʹ�ã�Ҳ�������ʹ�� ���¼�֪ͨ�ͻص�������
			*/
	}
	//6.2 �¼�֪ͨʵ���߼�
	{
/*
			*		C3˵��ڿ����˽�����߼����������ֺ������ĸ���
			*		ͨ�����ֲ�̫����⣬��ʵ�ܼ򵥵�
			*
			* �������飺
			*		�����¼����顢socket���顢�ص��ṹ������
			*		�±���ͬ�İ󶨵�һ��  ���ξͲ����¼�������
			*		���Ϻܶ�̳��Ǵ������¼�����ģ�����ὲҪ��Ҫ������
			*
			* �����ص�IOģ��ר��ʹ�õ�socket��
			*		��WSASocket��ר�������첽������SOCKET
			*
			* �����߼���
			*		1.Ͷ��AcceptEx
			*		2.ѭ���ȴ��ź�
			*
			*	Ͷ��AcceptEx��
			*			������ɣ�
			*					������ɵĻ��������϶Կͻ���socketͶ��WSARecv�����Ͷ�ݺ�Ҳ������������ӳ����
			*					Ȼ����������Ƿ�Ͷ��WSASend�����Ͷ�ݺ�Ҳ������������ӳ����
			*					���Ҫ���¶Է�����socketͶ��AcceptEx	����Ϊ֮ǰͶ�ݵ�AcceptEx�Ѿ�����ˣ���Ҫ����ʹ�þͱ�������Ͷ�ݣ�
			*			�ӳ����
			*					����Ͷ�ݺ�û��������ɣ��ǲ���ϵͳ�Ὺ��һ���̳߳�����������߳�������ȴ�AcceptEx���ź����
			*					�����Ļ����ȴ��ź���ɽ�������ϵͳ���̣߳������ؾ͵�ѭ���еȴ��ź������Ȼ����մ���
			*	ѭ���ȴ��źţ�
			*		֮ǰͶ�ݵ�AcceptExû��������ɣ���������ϵͳȥ��ܺ����ǻص�ѭ���еȴ��ź����
			*			���ź�
			*					��WSAWaitForMultipleEvents���������յȴ��ź�
			*			���ź�
			*					�յ��źź󣬻�ȡ�ص����ṹ�ϵ���Ϣ����WSAGetOverlappedResult����
			*					Ȼ��ʼ���ദ������ź���ʲô����ô����
			*						����ǿͻ����˳������Ǿ�ɾ����Ӧ�ͻ�����Ϣ
			*						����ǽ������ӣ����Ǿ�Ͷ��AcceptEx��Ͷ�ݺ���߼��������ǵ������߼���
			*						����ǽ�����Ϣ��������Ϣ�󣬼����Կͻ���socketͶ��WSARecv�����������߼���
			*						����Ƿ�����Ϣ����������Կͻ���socketͶ��WSASend
			*
			*
			*/
	}	
	//6.3 WSASocket������ϸ����
	{
		/*
			* ����㣺
			*		���ǲ���Ҫ�ֶ���socket���ص�IO�ṹ�壬�����ǵ��ú���ʱ���������Զ������ݵĲ������а�
			*		���ǲ�Ҫ����ѧϰ������ģ�͵�ʹ�ã����ǻ��������ĺô��������������ʲô
			*		���Ժ����ǹ������ܺܺõķ�������˼��ȥ���
			*
			* �������̣�
			*		1.�������
			*		2.У��汾
			*		3.����socket����һ����C/S��һ������Ҫ��WSAsocket����
			*		4.�󶨶˿ں����ַ
			*		5.��������
			*		6.�ص�IO��ʼ��
			*
			* �����¼����顢�ص�IO�Ľṹ�����飺
			*		�����ó������Ԫ������
			*
			* �����첽������SOCKET��	WSAsocket
			*		���ã�
			*			����һ�������첽������SOCKET
			*			WSASocket�� windowsר�ã�WSA�ĺ�������windowsר�õ�
			*			WSA: Windows  socet  async  ��������֧���첽�����ĺ���
			*
			*	����1��2��3��
			*			��socket�����Ĳ�����һ���ģ���ַ���͡��׽������͡�Э������
			*			��3�������0����ϵͳ�Լ��ж�������Э�飬�����з��գ���Ϊ���ܶ��Э������ͬ��ǰ����������
			*			����ϵͳ�жϵģ���������Ҫ��Э��Ͳ�һ����
			*	����4��
			*			�����ǣ�ָ��WSAPROTOCOL_INFO�ṹ��ָ��
			*			�����׽�����ϸ�����ԣ�
			*				���磺���������Ƿ���Ҫ����
			*					�Ƿ�֤����������������䶪���Ƿ�����
			*					�������3����0����ô����������ָ��Ϊ�ĸ�Э��
			*					���ô�������ֽ���
			*					�����׽���Ȩ��
			*					������ౣ���ֶΣ�����չʹ��
			*			������֪��������������Ժ����õ�����֪����ôȥѧϰʹ����
			*			��ʹ�õĻ��� NULL
			*	����5��
			*			��0���ɣ������Ĳ�������δʹ�ã�����չ,
			*			��ʦ������һ��socket����ID���������ͬʱ�������socket
			*	����6��
			*			ָ���׽��ֵ����ԡ�
			*			��д��WSA_FLAG_OVERLAPPED �� ����һ�����ص�IOģ��ʹ�õ�socket
			*			�����ģ�
			*				���ڶಥЭ�飺WSA_FLAG_MULTIPOINT_C_ROOT��WSA_FLAG_MULTIPOINT_C_LEAF�� WSA_FLAG_MULTIPOINT_D_ROOT�� WSA_FLAG_MULTIPOINT_D_LEAF
			*				��ϲ���4�������׽��ֲ���Ȩ�ޣ�WSA_FLAG_ACCESS_SYSTEM_SECURITY�������Զ��׽���send����Ȩ�ޣ�������send��ʱ��ͻᴥ�����Ȩ�����ã�������
			*				�׽��ֲ��ɱ��̳У����ڶ��߳̿�����WSA_FLAG_NO_HANDLE_INHERIT
			*					���߳̿����У����̻߳�̳и��߳�desocket�����������������ھֲ���ʹ�á�
			*					���߳�ʹ�ø��߳�socket�����ַ�ʽ��
			*						1.ֱ���ø��̵߳�socket��������
			*						2.���߳̽����̵߳�socket����һ�ݣ��Լ��ã�������socket������ȫһ�������̳�
			* ����ֵ��
			*			�ɹ����򷵻ؿ��õ�socket�������˾���Ҫ�����׽��֣���closesocket
			*			ʧ�ܣ�����INVALID_SOCKET������ʧ�ܴ���Ҫ�ر������
			*
			*/

	}	
	//6.4 AcceptEx����ʹ�����
	{
/*
			* ��ʼ��socket���ṹ�����飺
			*		�ص�IO�ṹ������ĳ�Ա5���������õģ��������Ǻ���������Ա�����ģ�
			*		����һ��Ҫ����Ա5�ֶ���ʼ��һ���¼������ú���WSACreateEvent����һ���¼����ظ���
			*		��Ϊϵͳ���ᴴ���¼��������Ǿ��޷�ȷ���������ݣ�Ҳ���޷�ʹ��
			* ѧϰ������
			*		һЩ�����Ĳ����ܶ࣬��Ҫ���£�MSDN������ϸ����
			*		������һ������⣬����ѧ���
			* ���β�����
			*		��ʼ����socket���ṹ������
			*		д���Զ��庯����PostAcceptͶ�ݺ����������߼���װ
			*		ע�⿴if else�Ľṹ����������0�����󷵻ش�����
			* ����ԭ�ͣ�
			* BOOL AcceptEx(
			*				  SOCKET       sListenSocket,
			*				  SOCKET       sAcceptSocket,
			*				  PVOID        lpOutputBuffer,
			*				  DWORD        dwReceiveDataLength,
			*				  DWORD        dwLocalAddressLength,
			*				  DWORD        dwRemoteAddressLength,
			*				  LPDWORD      lpdwBytesReceived,
			*				  LPOVERLAPPED lpOverlapped
			*				);
			*
			* AcceptEx������
			*	���ܣ�	Ͷ�ݷ�����socket���첽�Ľ������ӣ��������ǳ����ù������ò���ϵͳ���߳�ȥ�������ӵĹ��̣����ǿ��Ըɱ���£�
			*
			*	����1��
			*			��������socket
			*	����2��
			*			Ҫ���ӵ��������� �ͻ��˵�socket
			*				ע�⣺���socket��Ҫ�����ֶ���������socket��WSASocket�����ԣ���WSASocket�������߼�
			*				���ǲ������Ļ���ϵͳ�ǲ��ᴴ���ģ���Ҫ�ֶ��������ٴ�������2
			*				ϵͳ�ڿ����߳̽�������ʱ���ͻ��Զ������socket��ͻ��˵Ķ˿ںš�IP��ַ����һ���൱���Զ�������bind����
			*	����3��
			*			������ָ�룬�������������Ϸ��͵ĵ�һ������
			*			char str[1024]����һ���ַ����������͡������ֶ���������Ȼ�󴫵�����3���ɡ�
			*			�������ó�NULL���������ͨ������4��ȡ��������ܡ�
			*				���彲�⣺����������������������3�Ĺ��ܣ���ô�ͻ������ӵ��������󣬵�һ��send���͵���Ϣ��������WSARecv������
			*						 �������������3�����գ��ڶ����Ժ󣬾�����WSARecv��������
			*						 �����������3��һ�����ڽ��տͻ��˵�һ�η��͵����ݵ����飬��һ���ַ������飬Ҳ���Խ������ָ�루��ַ��
			*	����4��
			*			0  ���ó�0��ȡ������3�Ĺ���
			*			1024�� ���óɲ���3������Ԫ�����ֵ����ô�ͻῪ������3�Ĺ���
			*					��ʱ���ͻ������Ӳ�������һ����Ϣ���������Ų����ź�
			*					���Ի�������״���������Ӳ��У�����ȿͻ��˷���һ����Ϣ���������¶������ź�
			*			�������ó�0�ͺ��ˣ�����3��4�Ĺ��ܲ����ã�һ��������գ��Ǹ����յģ��߼��͸�����
			*	����5��
			*			sizeof(struct sockaddr_in) + 16
			*			���ص�ַ��Ϣ �ı������ֽ�������ֵ�����ʹ�õĴ���Э�������ַ����Ҫ��16���ֽ�
			*				���彲�⣺���ǵõ����ص�ַ��Ϣ�������ַ��Ϣ�����ڴ��д����
			*					���Ǵ��������Ժ�Ҫ�������浽�ļ�λ�ã������൱�ڱ����ݴ�һ����
			*					����Ǳ������ļ�λ�õ�Ԥ���Ĵ�С
			*	����6��
			*			sizeof(struct sockaddr_in) + 16
			*			Զ�̵�ַ��Ϣ �ı������ֽ�������ֵ�����ʹ�õĴ���Э�������ַ����Ҫ��16���ֽ�
			*				���彲�⣺ͬ����5һ����������Զ�̵�ַ����Ϣ
			*	����7��
			*			������д NULL��Ҳ������дDWORD�����ĵ�ַ
			*			����ϲ���3��4��ʹ�õģ�����������ڸպÿͻ������ӵ���������ʱ�򣬿ͻ��˷�����һ����Ϣ
			*			�ǲ���3�ͻ����Ͻ��յ������Ϣ�����Ҳ���7װ�Ų���3���յ����ַ���
			*				���⣺����ֻ���ڸպ������ϵ�ʱ�򣬼������߼����������ɣ�ͬ����ʱ��������Ϣ������7�Ż�����
			*					 ������첽��ɣ�����Ͷ�ݴ˺���û��������ɣ��ҿͻ��˷�����Ϣ�������������û�ã�û������
			*	����8��
			*			���ǵ��ص��ṹ��
			*	����ֵ��
			*			TRUE:������ɷ��أ���ִ�У����пͻ��˾�������
			*			FALSE�������ˣ���int a = WSAGetLastError()����ô�����
			*				���a == ERROR_IO_PENDING�������첽�ȴ�����ʱû�пͻ�������
			*				�����Ĵ����룺���ݾ����������
			*/
	}
	//6.5 WSARecv�������
	{
/*
			* ��Թ�� ���Ͻڿ��ִ���һ��AcceptEx�������������˺ܶ�ʱ�䣬����Ҳ�����ˣ�Ҳû��ס����
			*		��ڿε�WSARecv��������������Ҿ͸���ճ��������
			*
			* �ܽ᣺
			*		����ԭ�ͺ���Ҫ������ĳ�����������ǲ��ܹ⿿����ȥ��ס��
			*		����ԭ�ͻ��������������������Ͱ�����������ֵ�����Ͱ��������������ѧϰ�º���������
			*		���������ʹ�LP�Ķ���ָ�����ͣ���ַ���ͣ�
			*		�ݹ�ķ�ʽ���ɸ��÷ǵݹ飨ѭ�������������continue����ѭ�����ӳٴ���������break��
			*		ֻ��ȷ���˿ͻ������ӳɹ����ҽ�������Ϣ���ü����±���Ч����ʦ�ķ���,��ʦ��Recv����Ҫ���ͻ��˵��±꣬�������ڲ�+1��
			*
			* WSARecv����ԭ�ͣ�
			*		int WSAAPI WSARecv(
			*							SOCKET                              s,
			*							LPWSABUF							lpBuffers,
			*							DWORD                               dwBufferCount,
			*							LPDWORD								lpNumberOfBytesRecvd,
			*							LPDWORD								lpFlags,
			*							LPWSAOVERLAPPED						lpOverlapped,
			*							LPWSAOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine);
			* ���ã�
			*		�첽������Ϣ
			* ����1��
			*		�ͻ���socket
			* ����2��
			*		���պ����Ϣ�洢buffer��һ���ṹ�壬��Ա�ֱ�Ϊ���ַ����飬�ַ�������ֽ�������������һ����ֵģ����ַ����飬�������Ĵ�С����
			*		struct _WSABUF {ULONG len; CHAR  *buf;} ��Ա1���ֽ�������Ա2��ָ���ַ������ָ��
			* ����3��
			*		����2�Ľṹ��WSABUF �ĵĸ���
			*		��1����
			* ����4��
			*		���ճɹ��Ļ�������װ�ųɹ����յ����ֽ���
			*		����6�ص��ṹ��ΪNULL��ʱ�򣬴˲����������ó�NULL
			* ����5��
			*		ָ�������޸�WSARecv����������Ϊ�ı�־��ָ�루����recv�����Ĳ���5��ָ��Э�黺�����Ķ�ȡ��ʽ�������ж��˲�ɾ�������˾�ɾ�ȣ�
			*		MSG_PEEK��Э�黺������Ϣ���Ƴ�������ɾ������recv����5һ��
			*		MSG_OOB���������ݣ������ţ�
			*		MSG_PUSH_IMMEDIATE��֪ͨ���;������
			*					���紫��10M���ݣ�һ��ֻ�ܴ�1M�������Ҫ���10�ַ��ͣ���һ�ݷ����У�����9��Ҫ�Ŷӵ��ţ�ָ���������ǣ�
			*					��ôָʾ�˿���������ˣ���ôû�����͵ľͱ������ˣ��Ӷ���������ݷ���ȱʧ
			*				�ò������������ڶ����ݵķ��ͣ����������û���⣬�����ļ�ʲô�ľͲ�������
			*				��ʾϵͳ���촦�������ܼ���һ�����ӳ�
			*		MSG_WAITALL���������ṩ�Ļ����������������ѹرա�������ȡ����������
			*		MSG_PARTIAL�������ģ�����Ķ��Ǵ���ģ�����ʾ���Ǵ˴ν��յ��������ǿͻ��˷�����һ���֣�������������һ���֣���ʦ˵�����ʵ�ã�
			* ����6��
			*		�ص��ṹ
			* ����7��
			*		�ص����������������ʽʹ��
			*		�¼���ʽ���Բ�ʹ�ã���ʹ�þ�����NULL
			*
			* ����ֵ��
			*		0 ������������������Ƚ��У����̾ͷ���ȥ��
			*		SOCKET_ERROR����ȡ��������
			*			����������ǣ�WSA_IO_PENDING����ʾ�ص��ṹ�ɹ��ˣ�������������ɣ������ӳٴ���
			*			�������������ʾ�������г�����
			*
			*
			*/
		}
	//6.6 ��ȡ�첽����ľ����¼�
	{
/*
		*		�����ڿ��ǽ�������ɣ���ڿ��ǽ��ӳٵȴ���
		* 
		* ���������������:
		*		ѭ�������뿴����
		* 
		* ѭ���ȴ��źţ�
		*		ÿ��ѯ���źź���ֻ����һ���������ͬʱ64������Ȼ����forѭ�����α����������ǵ�socket���飬��ͻ����64������
		*		����Ҳ����ԵĹ�ƽ�ˣ���Ϊѯ���źź�������������Ӧ���±���С���¼�������ÿ��ѯ��һ��Ȼ��������Ͳ�����ֲ���ƽ������
		*		��Ҳֻ����Թ�ƽ
		*	
		*		n == 0 ʱ���źţ�0�Ƿ�����socket����˵��������������Ҫaccept
		*
		*		WSAGetOverlappedResult�����Ĳ���3���Ὣ���ͻ��߽��յ���ʵ���ֽ������ݵ�����������У�����0��˵���ͻ���������
		*		�����Ĳ���5������WSARecv�Ĳ���5
		*		
		* 
		* ��ȡsocket�źŵľ��������
		*	����ԭ�ͣ�
		*			BOOL WSAAPI WSAGetOverlappedResult(
		*					  SOCKET          s,
		*					  LPWSAOVERLAPPED lpOverlapped,
		*					  LPDWORD         lpcbTransfer,
		*					  BOOL            fWait,
		*					  LPDWORD         lpdwFlags );
		* ����
		*		��ȡ��Ӧsocket�ϵľ������
		* ����1
		*		���źŵ�socket
		* ����2
		*		��Ӧ���ص��ṹ
		* ����3
		*		DWORD���ͣ������Ὣ���ͻ��߽��յ���ʵ���ֽ������ݵ������������	
		*		�����0 ��ʾ�ͻ�������		
		* ����4
		*		�����ص�����ѡ���˻����¼������֪ͨʱ�����ܽ�fWait��������ΪTRUE��
		*		��TRUE��ѡ���¼�֪ͨ	�������ģ�;����¼�֪ͨ
		* ����5
		*		1��װWSARecv�Ĳ���5 lpflags
		*		2��������NULL
		* ����ֵ
		*		trueִ�гɹ�
		*		falseִ��ʧ��
		*			
		*/
		}	
	//6.7 �ص�IOģ���¼����ദ��
	{
			/*
		* //ģ�;���
		*	AcceptEx��ĩβ���������ص�IO�Ľṹ��Ҫ�����Ǵ����źŵ�socket�±�һ��������0
		*	�ͻ���ǿ�����߻��д�����10054���������߲����д����룬�������ߴ���Ҫ�������������
		*	�ź���Ҫ�������ã������һֱ���Ǹ��ź��ˣ���WSACL
		*	��֮ǰû��������⣺�޷��������ⲿ���� _AcceptEx@32 ����Ϊû�ӿ⣬��ֻ����ͷ�ļ������´ο�MSDNҪע��ͷ�ļ��Ϳ��ˣ�
		*	�ڽ��з����ж�ִ�н������Ӳ�����ʱ�򣬽�βҪ��continue����ѭ������ΪnΪ0��ʱ�򣬻�������ִ��,
		*	��ʱ�õ��ź����͵Ĳ���3dwStateΪ0����Ϊ�Ƿ�����socket�����ͻ�ѿͻ��˸��رյ�
		* 
		*	�����Ŀ�����ͺ��ˣ���ڿεĴ�����ǰ�whileѭ����ķ��ദ���д����
		*	�ܶ�ע�Ͷ���ѭ����ҳ���һ��������������ʼǵ�����������Ҳ��˰��춼��֪����������
		*	��Ϊ�Ҷ����̸�������Ϥ��Ҫ��ǿһ�����Ĵ�����
		*/
		}	
	//6.8 WSASend����ʹ��
	{
		/*
		* �Ͻڿ������ܽ᣺
		*		1��accept 
		*				��������λ������AcceptEx���������±�Ҫ�����1���±�һ��
		*		2��recv
		*				��������
		*		3��close 
		*				�������
		*				ǿ���˳��ô�����10054���
		*				�����˳��ò���3==0���
		*		4���ź��ÿ�
		*				WSAResetEvent(g_allOlp[i].hEvent);
		* 
		* ��ڿ����ݣ�
		*		д��WSASend�ķ��ͺ��������ڲ���һֱһֱ���ͣ�����WSASend���������Լ������Լ�������Ҫ��ʱ����þͺ���
		*		���Ҫ��scanf���룬�벻Ҫ��ִ��WSASend�����룬Ҫ��WSASend����ǰ�����룬�뿴PostSend����
		*		���ն�Ҳ��Ҫ���recv�������ܽӵ�
		*	���ص�IO���¼�ģ�ʹ��ڵ����⣺
		*		��һ�δ�������У��ͻ��˲������send���������������ν�����Ϣ����һ�ν�����Ϣ������������Ϣ
		* 
		* WSASend������
		*			���˲���5����ָ�����ͣ���������WSARecvһ��
		*		����ԭ�ͣ�
		*				int WSAAPI WSASend(
		*					  SOCKET                             s,
		*					  LPWSABUF                           lpBuffers,
		*					  DWORD                              dwBufferCount,
		*					  LPDWORD                            lpNumberOfBytesSent,
		*					  DWORD                              dwFlags,
		*					  LPWSAOVERLAPPED                    lpOverlapped,
		*					  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine  );
		*		����
		*			Ͷ���첽������Ϣ
		*		����1
		*			�ͻ���socket
		*		����2
		*			���պ����Ϣ�洢buffer���ṹ�壬��recv��һ����
		*						��Ա1
		*							�ֽ���
		*						��Ա2
		*							ָ���ַ������ָ��
		*		����3
		*			����2�Ǹ�WSABUF �ĵĸ���
		*			1������
		*		����4
		*			���ճɹ��Ļ�������װ�ųɷ��͵��ֽ���
		*			����6�ص��ṹ��ΪNULL��ʱ�򣬴˲����������ó�NULL
		*		����5
		*			����������Ϊ�ı�־��
		*		����6
		*			�ص��ṹ
		*		����7
		*			�ص�����
		*				���������ʽʹ��
		*					�¼���ʽ��ʹ��
		*					����NULL
		*/
		}
		
	
}