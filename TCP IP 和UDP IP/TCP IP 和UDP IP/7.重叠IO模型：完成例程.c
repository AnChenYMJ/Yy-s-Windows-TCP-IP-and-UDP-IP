#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSOCK.lib")

//����socket���顢�ص�IO�ṹ������
#define MAX_COUNT 10240
SOCKET g_sockall[MAX_COUNT];	//socket����
OVERLAPPED g_olpall[MAX_COUNT];	//�ص�IO�ṹ������
int g_count = 0;				//����

//Recv�Ļص�����
void CALLBACK RecvCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
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
		//ѯ���ź�
		DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[0].hEvent, FALSE, WSA_INFINITE, TRUE);
		if (wfmte == WSA_WAIT_FAILED || wfmte == WSA_WAIT_IO_COMPLETION)
		{	//������  ��  ������̵�����ź�
			continue;
		}
		//���÷������ź�
		BOOL ChongZhi = WSAResetEvent(g_olpall[0].hEvent);
		if (FALSE == ChongZhi)
		{
			int a = WSAGetLastError();
			continue;
		}

		//�ͻ��˽�������
		//������������ˣ���ΪͶ�ݸ�����ϵͳ������ϵͳ����ˣ�
		printf("accept �ӳٿͻ���%d���ӳɹ� \n", g_sockall[g_count]);
		//��ϵͳͶ��recv����Ϊ����������
		PostRecv(g_count);		//ʹ��g_count����Ҫ��n����Ϊn�ǵ�ǰ�ķ���������g_count�����¿ͻ��˵�
		//�������Ͷ��send
		//PostSend(g_count);
		//����++
		g_count++;
		//����Ͷ��accept����Ϊ���������˵����֮ǰ��accept�Ѿ����
		PostAccept();
	}
	

	//���������������(socket �� event(�¼�))
	Clear();
	WSACleanup();

	system("pause>0");
	return 0;
}

//recv�Ļص�����
void CALLBACK RecvCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	//��1�Ǵ����룬Ϊ10054��˵����ǿ������
	//��2�ǵõ�recv��ȡ�����ݵ��ֽ�����Ϊ0��˵����������
	//��3�ǵ�ǰsocket���ص��ṹ
	//��4��WSArecv�Ĳ���5������recv�����Ĳ���5��ָ��Э�黺�����Ķ�ȡ��ʽ�������ж��˲�ɾ�������˾�ɾ�ȣ�
	
	//��ȡ��ǰ�¼����±�(�Ż���д������ַ-��Ԫ�ص�ַ == �±����ֵ)
	int i = lpOverlapped - &g_olpall[0];
	//���ߴ���
	if (10054 == dwError || 0 == cbTransferred)
	{
		printf("�ͻ���%dclose��\n", g_sockall[i]);
		//ɾ�����߿ͻ��˵�socket���¼�
		closesocket(g_sockall[i]);
		WSACloseEvent(g_olpall[g_count - 1].hEvent);
		//��ĩβԪ����䵽ɾ���Ŀͻ��˵�λ��
		g_sockall[i] = g_sockall[g_count - 1];
		g_olpall[i] = g_olpall[g_count - 1];
		//����-1
		g_count--;	
	}
	//������Ϣ
	if (cbTransferred != 0)
	{
		//��ӡ��Ϣ
		printf("recv: %s\n", g_recv);
		//��ȫ�ֱ���str����
		memset(g_recv, 0, MAX_RECV_COUNT);
		//�������Ͷ��send
		//�����Լ�����������WSARecv���ȴ�����Ϣ����
		PostRecv(i);
	}
}
//Send�Ļص�����
void CALLBACK SendCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	printf("WSASend�Ļص������ɹ�����\n");
}
//PostAcceptͶ�ݺ����������߼���װ
int PostAccept()	//�����򷵻�0�����򷵻ش�����
{
	while (1)
	{
		//����2���ͻ���socket����Ҫ�ֶ�����
		g_sockall[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == g_sockall[g_count])
		{
			int a = WSAGetLastError();
			break;
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
			//������������ˣ���ΪͶ�ݸ�����ϵͳ������ϵͳ����ˣ�
			printf("accept �����ͻ���%d���ӳɹ� \n", g_sockall[g_count]);
			continue;
		}
		//Ͷ�ݺ�û��������ɣ�Ҳ���ܳ�����
		else
		{
			int a = WSAGetLastError();
			//��ʱû�пͻ�������,�����ӳٴ���
			if (a == ERROR_IO_PENDING)
			{
				break;
			}
			//������
			else
			{
				break;
			}
		}
	}
	return 0;
}
//WSArecv�����������߼���װ
int PostRecv(int Index)
{
	while (1)
	{
		//����2
		struct _WSABUF buff;
		buff.buf = g_recv;			//str�Ѿ���ȫ����������
		buff.len = sizeof(g_recv);

		DWORD count;		//����4
		DWORD DwFlag = 0;   //����5
		int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], RecvCALL);
		//��������
		if (0 == nrecv)
		{
			//��ӡ��Ϣ
			printf("recv: %s", buff.buf);
			//��ȫ�ֱ���str����
			memset(g_recv, 0, MAX_RECV_COUNT);
			//�������Ͷ��send
			continue;
		}
		//�ӳٴ�����߳�����
		else
		{
			int a = WSAGetLastError();
			if (WSA_IO_PENDING == a)
			{
				break;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
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
	DWORD SeDwFlag = 0;		//����5
	int nSend = WSASend(g_sockall[Index], &buff, 1, &Sendcount, SeDwFlag, &g_olpall[Index], SendCALL);
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
		closesocket(g_sockall[i]);	//socket����
		WSACloseEvent(g_olpall[i].hEvent);	//�ص�IO�ṹ������
	}
	g_count = 0;
}

void studybook()
{
	{
		//7.1 ������̻���ԭ�����
		/*
		* ���ʣ�
		*		��socket��һ��������������Ȼ���ص�IOģ�͵����ԣ�Ͷ�ݵ�����ϵͳ�첽�����WSARecv�Ȳ�����ʱ��
		*		ϵͳ���첽���ø��԰󶨵ĺ����������Զ����ദ��
		*		���Ǿ��ں����н�����Ӧ����
		* �¼�֪ͨ��ģ�ͣ�
		*		Ͷ�ݺ�Ҫ����WSAGetOverlappedResult������ȡ�źŵĶ�Ӧ�Ĳ�����Ȼ���ֶ������߼����з��ദ��
		*		�Լ����࣬������߼��Ƚ϶�
		* �Աȣ�
		*		�Ա��£�������̻����һ�㣬��Ϊ���Զ����ú�����Ҳ�����Զ�����
		*		���¼�֪ͨ��Ҫһ������˳�������ѯ���źţ�Ч�ʱ��������������һ��
		*		����WSARecv��WSASend�Ĳ�����һЩ�����������Ҫ���ϻص���������
		*		�����Ĵ������AcceptEx��������һ���ģ������߼�Ҳһ��
		* �����߼���
		*		�����¼����飬socket���飬�ص��ṹ�����飺�±���ͬ�İ󶨳�һ��
		*		�����ص�IOģ��ʹ�õ�socket��WSASocket
		*		Ͷ��AcceptEx��
		*					������ɣ�
		*							�Կͻ����׽���Ͷ��WSARecv��WSARecvҲ������������ӳ���ɣ��൱�ڵݹ�
		*							��������Կͻ����׽���Ͷ��WSASend
		*							�Է������׽��ּ���Ͷ��AcceptEx
		*					�ӳ����
		*							ѭ�����źţ�
		*									���źţ�WSAWaitForMultipleEvents
		*									���źţ��������ӣ�Ͷ��AcceptEx
		*									��������Կͻ����׽���Ͷ��WSASend
		* 
		*		
		*/
		//7.2 ������̻ص���������
			/*
			* �ص�������
			*	��Ҫ�Ա�WSAGetOverlappedResult����ѧϰ��
			*	����ԭ�ͣ�
			*		void CALLBACK funnny
			*		(
			*		DWORD dwError, 
			*		DWORD cbTransferred, 
			*		LPWSAOVERLAPPED lpOverlapped,
			*		DWORD dwFlags
			*		)
			*	����ֵ
			*			void
			*	����Լ��
			*			CALLBACK	������Լ�����Ͳ����࣬�ǵ�д�ϣ�
			*	��������
			*			����
			*	����1
			*			������		����Ӧ����õ��źž�����������Ĵ����룬��WSAGetLastError��ȡ�Ĵ����룩
			*	����2
			*			�������߽��յ����ֽ�������Ӧ�õ��źž�����������Ĳ���3��������������Ϊ0���Ǹ�������
			*	����3
			*			�ص��ṹ		����Ӧ�õ��źž�����������Ĳ���2��
			*	����4
			*			����ִ�еķ�ʽ����ӦWSArecv�Ĳ���5��
			*			����WSArecv����5
			*
			*	�������̣�
			*			dwError == 10054��ǿ���˳������жϣ�Ȼ��ȷ������ɾ���ͻ���
			*			cbTransferred == 0�������˳�
			*			else����������
			*			����Ͷ�ݽ�������
			*	
			*	WSAGetOverlappedResult����:	��ȡ�źŴ����¼��ľ������
			*			BOOL WSAAPI WSAGetOverlappedResult
			*			(
			*			  SOCKET          s,
			*			  LPWSAOVERLAPPED lpOverlapped,
			*			  LPDWORD         lpcbTransfer,
			*			  BOOL            fWait,
			*			  LPDWORD         lpdwFlags
			*			);
			* 
			*			�Ա�����¼�֪ͨ��Ҫ�ĺ���
			*			���ö������ǻص������Ĳ�������
			*			�¼�֪ͨͨ���ú����õ���Щ��Ϣ
			*			ͬ������Ϣ���������Ҳ��Ҫ������ͨ����������
			*		
			*/
		//7.3 ��������¼����ദ��
			/*
			* 
			* 
			*	��Ҫ���¼�֪ͨ������ӻ��޸ĵĵط���
			*		1.	DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[0].hEvent, FALSE, WSA_INFINITE, TRUE);
			*			����4 ��0 ��Ϊ WSA_INFINITE������5��FALSE��ΪTRUE TRUE�Ǹ��������ʹ�õģ�����������������
			*			�±껻�ɷ������ģ�����ֻҪһֱ�ȴ��������ľͺ��ˣ����ǵȴ�accept�������Ľ����ص��������жϴ���
			*			ע��ĳ�WSA_INFINITE��һֱ�ȴ����󣬺�������ֵ����ʱ�ж�Ҫ�޸ĳ� �ȴ��¼���������WSA_WAIT_IO_COMPLETION
			*			�Ҿ������￨�˰��죬ԭ����û�н���ʱ������жϻ���WSA_WAIT_IO_COMPLETION������û�����ӵ�����£��ͻ��˷�����Ϣ
			*			���Ǽ�������ִ�У�����g_count���ϵ�++ ����������Ҳ����ִ�У�������Ϊû��WSA_WAIT_IO_COMPLETION���ж�
			*			��û��ִ��continue �� û�й��˵�������һֱ��ִ��whileѭ��
			* 
			*		2.	��ӻص�������recv�ĺ�send��
			*			//��1�Ǵ����룬Ϊ10054��˵����ǿ������
			*			//��2�ǵõ�recv��ȡ�����ݵ��ֽ�����Ϊ0��˵����������
			*			//��3���ص��ṹ
			*			//��4��WSArecv�Ĳ���5������recv�����Ĳ���5��ָ��Э�黺�����Ķ�ȡ��ʽ�������ж��˲�ɾ�������˾�ɾ�ȣ�
			* 
			*		3. WSARecv �� WSASend �����һ����������NULL��Ϊ���Ƕ���õĻص����������֣��磺
			*			int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], RecvCALL);
			* 
			*		4.	�󲿷ַ��ദ��ɾ����������PostRecv(g_count);  PostAccept();  g_count++;�⼸�����ߴ�ӡ��Ϣ
			*			����Postsend���ⲿ�־����ӳٵȴ�������·����ģ�ֻҪ�ӳٵȴ�����һ����·�Ǻ����ڵ�������ɣ�
			*			�������������·Ҳ������Ͷ�ݵ��������������ɡ��ӳٵȴ�����·��Ҫ����Ͷ��һ��
			* 
			*		5.  ���ߴ���ͽ���Recv�Ļص����������ദ��ֻҪһ���ߣ���Ӧ�ͻ��˵��¼��ͻᴥ���źţ�Ȼ�����ϵ��ûص�����
			*			Ȼ����ڻص������н������ߴ���
			* 
			*		���ע���ҵ��Ǹ����⣬��Ϊѯ���źź����Ĳ���4�ı䣬ԭ���ĳ�ʱ����Ҫ�滻��WSA_WAIT_IO_COMPLETION
			*		��Ȼ�ͻ��˷�����Ϣ���ͻ�ִ��һ����ʱ�ȴ�������·�����к������ã��ͻ��BUG
			*		
			* 
			*	ѯ���źź�������5����WSA_INFINITE����Ϊֻ��ѯ�ʷ��������¼����Ǿ����ź�������ȫû����
			*	����6���ó�TRUE�����壺
			*				���ȴ��¼�������������̵Ļ��ƽ�ϵ�һ��TRUE����Ϊ����������õ�
			*				��Ϊ����ʵ�֣��ȴ��¼�������������̺������첽ִ�У���ִ����������̺������ͻ���ȴ��¼�����һ���ź�
			*				�ȴ��¼���������WSA_WAIT_IO_COMPLETION
			*				��WSAWaitForMultipleEvents�����ܻ�ȡ�¼����ź�֪ͨ�����ܻ�ȡ������̵�ִ��֪ͨ
			* 
			* 
			*	
			* �ܽ᣺һ��Ҫ��������Ϥ���������ȷ���������ڵĻ����ʹ�ȷ����λ�ÿ�ʼ�����������ķ�������ä������	
			*/
		//7.4 �ص�IOģ�ͼ��ܽ�
			/*
			*	��ʱ61����ʦ��һ������ͼ�����ǶԱ���ȥ������֮ǰѧ�������̵�����
			*	��ʦ˵�ص�IO�Ƚ��ѣ�����ɶ˿�ģ����70%���ص�IOһ����ѧ�ĺõĻ�����ɶ˿�ģ�;͸�������һ��
			*	���Ժú�����ص�IOģ�ͣ���ɶ˿����ص�IO���Ż�
			*	
			* ���ζԴ�����޸ģ�
			*		���ݹ��д���޸ĳ�ѭ���ķ�ʽ��������ɾ�continue���ӳٴ����break
			*		ѭ���Ǹ��õģ��ݹ��ǲ��ܲ���̫��ģ��ᱬջ����ѭ�����õ��ģ�
			*				һ������������ģ�Ͳ�����̫���ݹ�����
			*		
			*		�޸�������RECV�Ļص������еĻ�ȡ�±�ķ�ʽ�����������Ż���
			*			ѭ����ȡ�±�����⣺��������ͻ�������̫�࣬ѭ���ͻ�Ч�ʵ���
			*			��ô������ٽݾ�����Ϊ�ص��ṹ��ĳ�Ա�����飬������ص��ǿռ���������ַ����
			*			�������õ�ǰԪ�صĵ�ַ����ȥ��Ԫ�صĵ�ַ����ת����int���ͻ���˲�ֵ
			*			�������ֵ�������㣬���������±����ֵ
			*		�����Ż��ܺã������и��õģ�����ɶ˿�ģ���н���
			* 
			*	�¼�֪ͨ��������̵�����
			*		�¼�֪ͨ�������ǽ���ǧ������û� ������߳�ȥ�ȴ�����Ȼ�󰤸�����ѯ���ź�
			*		������̣����Ǿͼ���һ���������˵ľͿ����ˣ����ź��˾��Զ����ûص�����
			*			�����������Ҫ���¼�֪ͨҪ����һ��
			* 
			*		�¼�֪ͨ�������Լ�������������waitfor������˳���ܱ�֤��ѭ�������࣬�±�Խ��Ŀͻ����ӳٻ�Խ��
			*		�������������д����������룬ϵͳ���ݾ����¼��Զ��������ǵĴ��룬�Զ�����
			*		���ʵ�����ûص�����Ҳ�����߳��е��ò�ִ�еģ����Ե���ִ�лص������Ĺ���Ҳ���첽��
			*			����������̱��¼�֪ͨ�������Ժ�һЩ
			* 
			* ֮ǰѧ������ģ�Ͱ����ص�IO�����⣺
			*		��һ�δ�������У��ͻ��˲������send���������������ν�����Ϣ����һ�ν�����Ϣ������������Ϣ
			*		����ִ��һ�������Ĺ����У��ͻ����������˺ü������Ǵ�����֮ǰ�ģ���������ͻ���ʱ���ͻὫ�������͵���Ϣ�����ϳ�һ��������
			* 
			*	CSģ�ͣ����ӿͻ��˵Ĺ����������ģ�ִ��recv�� send���ƵĹ���Ҳ��������
			*	�ص�IO���ȴ��Ĺ��̽�������ϵͳ�����첽�ģ�recv��send���ƵĹ���Ҳ���ɲ���ϵͳ����ɣ�WSASend��WSARecv�ύ������ϵͳ��ɣ�������ֻ�ý����źŷ���ͺ���
			*		�����ԱȾ�֪������ѧϰ��ģ�͵Ľ�����
			*	
			*/
	};
}