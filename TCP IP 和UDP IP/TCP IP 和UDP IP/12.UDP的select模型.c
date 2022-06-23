#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//����socket�ŵ�ȫ�֣��������˳��������ã�ע���������ﲻҪ��д�����ˣ����ж�����
SOCKET SerVerSocket;

//����˳���������
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerVerSocket);
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

	//selcet����
	while (1)
	{
		//1.socket���� fd_set����
		fd_set fd;					//�ṹ���������
		FD_ZERO(&fd);				//��������
		FD_SET (SerVerSocket, &fd);	//���һ��socketԪ��
		//2.select��ѯ
		TIMEVAL st;			//�ṹ�����
		st.tv_sec = 5;		//��
		st.tv_usec = 5;		//΢��
		int sele = select(0, &fd, NULL, NULL, &st);
		//3.��ѯ���ˣ����д���
			//��ʱ������ϲ���5
		if (0 == sele)
		{
			printf("�ȴ���ʱ��\n");
			continue;
		}
			//����ִ�г���������
		if (SOCKET_ERROR == sele)
		{
			int a = WSAGetLastError();
			printf("select error: %d", a);
			continue;
		}
			//���¼���Ӧ�����д���
		if (0 < sele)
		{
			//recvfrom������Ϣ
		struct sockaddr stclient;	//����5�����ڼ�¼�Է�socket���ͻ��ˣ��Ķ˿����ַ
		int nlen = sizeof(stclient);//����6����5�ṹ���С
		char cbuf[548] = { 0 };				//��2���ַ����飬��Ž��յ�����Ϣ
		int rnum = recvfrom(SerVerSocket, cbuf, 548, 0, &stclient, &nlen);
		if (SOCKET_ERROR == rnum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}

		printf("  %s\n", cbuf);
		//sendto������Ϣ
		int snum = sendto(SerVerSocket, "Server �յ�\n", strlen("Server �յ�\n"), 0, (const struct sockaddr*)&stclient, sizeof(stclient));
		if (SOCKET_ERROR == snum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		}
	}

	//�ͷ�socket���ر������
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}


void studybook12(void)
{
	//3.1 UDP/IP��selectģ�� 1
	{
		/*
			��ǰȰ�棺��ʦ˵����д����Щģ���ǻ���֪ʶ������������������ԭʼ�Ĵ���ģ��
					�ǲ���ֱ���õ�ʵ����Ŀ��ȥ�ģ�ʵ����ĿҪ�������������и����Ż����޸ġ���ӣ���Щ����ģ���кܶ�BUG
		 
			selectģ�����̣�
				1.�������⣬����ͷ�ļ�
				2.�������
				3.У��汾
				4.����socket
				5.�󶨶˿ںš���ַ
				6.select

			select�Ա�C/Sģ�ͣ�
				�������tcp/ipЭ���c/sģ����accept rcev ɵ�ȵ����⣬��������������ɵ�ȣ��ж�Ӧsocket�źŲ����ŵ�����غ���
				����UDP/IPЭ���c/sģ�Ͳ�����ɵ�ȵ����⣬��������recvfrom���һЩ
			select�߼�
				1������socketװ��һ������FD_SET��UDPֻ��һ��socket
				2��ͨ��select���������1�е�socket�����ĸ�����Ӧ��Ҳ���Ǽ����û����Ϣ����
				3�����ˣ�����recvfrom

				��һ��������һ��socket����
					fd_set���綨��õ�ר�Ÿ������õ�
						ת���忴��fd_set������
							Ĭ��FD_SETSIZE��64, ��ҿ�����Winsock2.hǰ��������꣬���������ֵ�����͸����ˣ���Ҫ����
							��Ϊԭ����ǲ����ı�����⣬Խ��϶�Ч��Խ�ͣ��ӳ�Խ�����Դ�Ҿ�����Ҫ̫�󣬼��ٸ���1024�Ͳ���ˣ���Ȼ��Ҳ��������������ø���~
							���ԣ�selectģ��Ӧ�ã�����С�û�������������ʮ���٣��򵥷��㡣
					�ĸ�����fd_set�Ĳ�����
						����ת���忴���߼���������߼����ü������㼴�ɣ�������¼��ЧԪ�أ��ü����������Ч���
							FD_ZERO����������0��FD_ZERO(&clientSet);
							FD_SET���򼯺������һ��socket������������64�����Ҳ����ڵ�ʱ��FD_SET(socket, &setRead);
							FD_CLR�������ɾ��ָ��socket��FD_CLR(socket, &setRead); ����Ҫ�ֶ��ͷ�
							FD_ISSET���ж�һ��socket�Ƿ��ڼ����У����ڷ���0���ڷ��ط�0��FD_ISSET(socket, &setRead)
		*/
	};
	//3.1 UDP/IP��selectģ�� 2
	{
		/*
			select������

			����ԭ�ͣ�
					int WSAAPI select
					(
					  int           nfds,
					  fd_set        *readfds,
					  fd_set        *writefds,
					  fd_set        *exceptfds,
					  const timeval *timeout
					);

			���ã�
				����socket���ϣ����ĳ��socket�����¼������ӻ����շ����ݣ���ͨ������ֵ�Լ�������������

			����1�����ԣ���0���������������Ϊ�˼���Berkeley sockets.
			����2������Ƿ��пɶ���socket�����ͻ��˷�����Ϣ�ˣ���socket�ͻᱻ���ã�������������ᱻ�������¼���socket����һ��socket����
					����UDPֻ��һ��socket�����Ƿ����������socket
			����3������Ƿ��п�д��socket��û�пͻ��ˣ��Ͳ����ˣ���Ϊ����������ӣ�û�пͻ��˵�socket���˲�����UDP����������������ˣ���NULL�Ͳ����ˣ�
			����4������׽����ϵ��쳣����Ҳ�����ˣ����Ǿ�һ��socket����NULL���ܲ���
			����5�����ȴ�ʱ�䣬����select��������ǰsocket���Ƿ�Ҫ�ȴ��¼����������߲��ȴ�
					�����ó����֣�Ч���ַ�Ϊ���ࣺ
						1.TIMEVAL �ṹ�壬��������Ա��tv_sec����tv_usec��΢��
							0 0��������״̬�����̷��أ�����������
							3 4���Ǿ����޿ͻ�����Ӧ������µȴ�3��4΢��
						2.NULL  select��ȫ������ֱ���ͻ����з�Ӧ���Ҳż���
							��ʱ��recvfrom������һ����
					���recvfrom��������������ó� 0 0 ���ǲ������ȣ�ֻ��һ��socket����������recvfrom����
			����ֵ��
					0���ͻ����ڵȴ�ʱ����û�з�Ӧ������continue������
					>0���пͻ�����������
					SOCKET_ERROR�������˴��󣬵õ�������WSAGetLastError()

			�ܽ᣺
				1.�ȴ�״̬����select��ִ�������ģ����ݲ���5������״̬
					���ȴ� 0 0 ��ִ������
					��ȴ� 3 4 ��ִ������+������
					ȫ�ȴ� NULL��ִ������+Ӳ����������
				2.selectģ�Ͷ��ڻ���UDP/IPģ��ûʲô���ǿ
				3.����Ҫע�⵽selectģ�͵�ʹ�ã�������΢��tcp/ip��һ���ˣ�������������
				4.ѧϰ֪ʶ������������֪ʶ�ľ��裬��䲻������
				5.����5���ó��еȴ�ʱ�䣬Ȼ��ʱ�����У��ܽ���һЩ������Ҫ�Ĳ�����������Ȼ����ȥ�ȴ������Ժ����
		*/
	};
}