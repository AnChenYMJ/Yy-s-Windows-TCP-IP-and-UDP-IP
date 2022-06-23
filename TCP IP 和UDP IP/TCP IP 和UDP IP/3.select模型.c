#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

fd_set Allsockets;				//�������ϣ�����װsocket


BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//�ͷ�����socket
	case CTRL_CLOSE_EVENT:
		for (u_int i = 0; i < Allsockets.fd_count; i++)
			closesocket(Allsockets.fd_array[i]);
		//�ر������
		WSACleanup();
	}
	return TRUE;
}

int main(void)
{
	//�ص�������Ϊ���ó���������ͷ�����socket�����һ��֪ʶ�㣺����̨�ر��¼�
	SetConsoleCtrlHandler(fun, TRUE);

	//�������
	WORD wdVersion = MAKEWORD(2,2);
	WSADATA wssocket;
	int wssock = WSAStartup(wdVersion, &wssocket);
	if (wssock != 0)
	{
		switch (wssock)
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
	if (2 != HIBYTE(wssocket.wVersion) || 2 != LOBYTE(wssocket.wVersion))
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//����SOCKET
	SOCKET Serversock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Serversock == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//�󶨶˿ں͵�ַ
	struct sockaddr_in socad;
	socad.sin_family = AF_INET;
	socad.sin_port = htons(22258);
	socad.sin_addr.S_un.S_un_b.s_b1 = 127;
	socad.sin_addr.S_un.S_un_b.s_b2 = 0;
	socad.sin_addr.S_un.S_un_b.s_b3 = 0;
	socad.sin_addr.S_un.S_un_b.s_b4 = 1;
	int  ace = bind(Serversock, (const struct sockaddr *)&socad, sizeof(socad));
	if (ace == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//�򿪼���
	int list = listen(Serversock, SOMAXCONN);
	if (list == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//�������ӣ����ʣ������ͻ��˵�socket��
	struct sockaddr_in cliesock;
	int add = sizeof(cliesock);
	SOCKET clientsock = accept(Serversock, (struct sockaddr*)&cliesock, &add);
	if (clientsock == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//selectģ��
	FD_ZERO(&Allsockets);			//��ռ���
	FD_SET(Serversock,&Allsockets);	//�򼯺������һ��socket�����Ϊ����������socket
	//FD_CLR(clientsock, &Allsockets);	//�Ӽ�����ɾ��ָ��socket
	FD_ISSET(clientsock, &Allsockets);
	//ѭ��ִ��selectģ�͵�����
	while (1)
	{
		fd_set Readsockets = Allsockets;	//����2�����м��������¼���ϣ�����select�����ڲ���ı��������
		fd_set writesockets = Allsockets;	//����3
		fd_set errorsockets = Allsockets;	//����4
		struct timeval tim;	//����5���������ȴ�ʱ�䣬ʱ��Ϊ�ṹ������Ա���
		tim.tv_sec = 3;		//��
		tim.tv_usec = 0;	//΢��
		int nREs = select(0, &Readsockets, &writesockets, &errorsockets, &tim);
		if (nREs == 0)	//û��socket�����¼�
		{
			continue;	//�ص�ѭ����ͷ����ѭ��
		}
		else if (nREs > 0)//socket�����з����¼�
		{
			//����4����������ͨ�Ź����е�ͨ���쳣����
			for (u_int i = 0; i < errorsockets.fd_count; i++)
			{
				char opt[1500] = {0};		//��4
				int len = sizeof(opt);		//��5
				//�õ��쳣�׽����ϵľ��������
				if (SOCKET_ERROR == getsockopt(errorsockets.fd_array[i], SOL_SOCKET, SO_ERROR, opt, &len))
				{
					int a = WSAGetLastError();
					printf("getsockopt��������ִ�д����޷��õ�ͨ�Ŵ�����Ϣ\n");
				}
				printf("ͨ���쳣�����룺%s\n", opt);
			}
			//����3 ���������п�д�Ŀͻ���
			for (u_int i = 0; i < writesockets.fd_count; i++)
			{
				//printf("������%d �ͻ���%d\n", Serversock, writesocket.fd_array[i]);	//�����ͻ��˾�һֱ�����޴�ӡ����Ϊ�����ӵĿͻ��ˣ�һֱ�������3��writesocket
				
				if (SOCKET_ERROR == send(writesockets.fd_array[i], "ok", sizeof("ok"), 0))
				{
					int a = WSAGetLastError();
				}
			}
			//����2 ��ѭ����������Ӧ���¼�socket,i�պö�Ӧ���±�
			for (u_int i = 0; i < Readsockets.fd_count; i++)
			{
				//����Ƿ��������¼���֤���пͻ�����������
				if (Readsockets.fd_array[i] == Serversock)
				{
					//��������
					SOCKET socketclient = accept(Serversock, NULL, NULL);
					//����Ͳ��ò���2��3��������ȡ��Ӧ�Ŀͻ�����Ϣ����getpeername()��getsockname()�������ɻ�ȡ
					if (socketclient == INVALID_SOCKET)	//��������
					{
						int a = WSAGetLastError();		//��ȡ������
						continue;
					}
					//�����ӵĿͻ��˵�socketװ��������
					FD_SET(socketclient, &Allsockets);
				}
				//����ǿͻ��˵��¼���֤��������Ϣ����
				else
				{
					//������Ϣ
					char sad[1500] = { 0 };
					int nrec = recv(Readsockets.fd_array[i], sad, 1499, 0);
					//�����������
					if (nrec == SOCKET_ERROR)
					{
						int a = WSAGetLastError();
						//ע�⣺���ǲ���ʱ�رտͻ��˵Ĵ��ڣ�����ǿ�����ߡ����������ߣ���һ������
						//��������Ŀͻ�������
						SOCKET temclose = Readsockets.fd_array[i];	//��¼���socket
						//Ҫ���м�������������ʱ�Ὣselect�ж���ԭ��λ�õ�����socket���޳�
						switch (a)
						{
						case 10054:
							//����socket�Ӽ�����ɾ����Ҳ��select��ɾ�����������
							FD_CLR(Readsockets.fd_array[i], &Allsockets);//�Ӽ�����ɾ��
							FD_CLR(Readsockets.fd_array[i], &Readsockets);//��select��ɾ��
							closesocket(temclose);						//�����socket
							break;
						}	
					}
					//����ͻ�������
					else if (nrec == 0)
					{
						//�ͻ�������
						//ע�⣺���ǲ���ʱ�رտͻ��˵Ĵ��ڣ�����ǿ�����ߡ����������ߣ���һ������
						//��������Ŀͻ�������
						
						//����socket�Ӽ�����ɾ����Ҳ��select��ɾ�������������
						//Ҫ���м�������������ʱ�Ὣselect�ж���ԭ��λ�õ�����socket���޳�
						SOCKET temclose = Readsockets.fd_array[i];	//��¼���socket
						FD_CLR(Readsockets.fd_array[i], &Allsockets);//�Ӽ�����ɾ��
						FD_CLR(Readsockets.fd_array[i], &Readsockets);//��select��ɾ��
						closesocket(temclose);
					}
					//�������
					else
					{
						//��ӡ��Ϣ
						printf("%d  %s\n", nrec, sad);
					}
				}
			}
		}
		else if (nREs == SOCKET_ERROR)	//select�������г���
		{
			//�õ�������
			int a = WSAGetLastError();
			//��Ӧ����
		}
	}

	//���socket���ر���·��
	for (u_int i = 0; i < Allsockets.fd_count; i++)
	{
		closesocket(Allsockets.fd_array[i]);
	}
	WSACleanup();

	system("pause>0");
	return 0;
}

	//3.1 select ����
	/*
	* selectģ���ص㣺
	* 1.�������C/Sģ���У�ɵ������������
	*		ɵ��������accept �� recv ִ��ʱ�����û�еȵ����Ӻ�û���յ���Ϣ������һֱִ������������ȵ���ĵ���
	*		ִ����������send��recv��accept��ִ�й������и���ճ����������̶��������ģ�������Ϊѭ��ɶ�ģ�
	*				select�����������ɵ��������������ִ��������Ҳ���ǲ��ǽ���⼸������ִ�е���������
	*		select�ǽ��ɵ�������ģ������ִ������
	* 
	* 2.ʵ�ֶ���ͻ������ӣ������û��ֱ�ͨ��
	* 
	* 3.���ڷ��������ͻ����ò����������Ϊֻ��һ��socket��
	*		�ͻ���recv��ʱ���ǲ�Ҳ�ǲ���send��
	*		ֻҪ��������һ���̣߳�recv�ŵ��߳���ͽ����
	* 
	* 
	* select�߼���
	*		1.��������ÿ���ͻ��˶���һ��socket������������Ҳ��һ��socket
	*			�����е�socketװ��һ�������У����ݽṹ��
	*		2.ͨ��select���������������е�socket����ĳ��socket����Ӧ
	*			�����socketͨ������/����ֵ�������������Ƕ��������ֵ������Ӧ����
	*		3.��Ӧ�Ĵ���
	*			�����⵽���Ƿ����������socket���Ǿ�֤���� �пͻ�����Ҫ���ӷ��������͵���accept
	*			�����⵽���ǿͻ��˵�socket���Ǿ��������ӵĿͻ�������ͨ�ţ��͵���send����recv
	* 
	* selectģ�ʹ������̣�
	*		1.�������ͷ�ļ��������
	*		2.�������
	*		3.У��汾
	*		4.����SOCKET
	*		5.��socket�󶨵�ַ���˿ں�
	*		6.��ʼ����
	*		7.select
	* 
	*		//����ǰ6���������C/Sģ�͵���һ���ģ�ֻ�ǵ�7���Ժ󻻳���select
	*
	*/
	//3.2 fd_set������ʹ��
	/*
	* fd_set:
	*		����һ��װ�ͻ���socket�����ݽṹ
	*		fd_set�����綨��ù�����ʹ�õ�
	*		ת���忴��fd_set����������������ṹ����������Ա��һ����¼������Ԫ����Ч��������һ��������select�ܴ���Ŀͻ���������Ĭ����64����
	*		���ǿ����Զ����Ǹ��꣬�������䣬�磺#define FD_SETSIZE      128
	*		��ע�ⲻҪ�ĵ�̫���ˣ���Ϊselect�ǲ�ͣ����ȥ������̫����Ч�ʾͺܵ��£�������Ҫ̫�󣬼��ٸ���1024�Ͳ���ˣ���Ȼ��Ҳ��������������ø���~
	*		����ͻ�������̫����ʹ�ø��߼�������ģ�ͣ�select�ʺ��û����ٵķ���
	* 
	*	�ĸ�����fd_set�Ĳ����꣺
	*			����ת���忴���߼�
	*		FD_ZERO�����������㡣				//ת�����壬���־����ü�����count = 0�������ͽ�ʡ����Դ�����س�ʼ�����ȵ�������+1���Ǽ���
	*		FD_SET:�򼯺������һ��socket		//ת�����壬�Ǹ�����ʵ�����������׿��ģ�����ѧϰ������ô�������
	*		FD_CLR��������ɾ���ƶ���socket
	*		FD_ISSET���ж�һ��socket�Ƿ�����ڼ����У����ڷ��ط�0�����ڷ���0��
	*/
	//3.3 select�������߼�����
	/*
	* select�������ã�
	*		����socket�ļ��ϣ���ĳ��socket�����¼����пͻ������ӻ��߷�����Ϣ��
	*		��Ὣ���socket��������ֵ���ظ�����
	* 
	* select����ʹ�ã�
	*		��Ӧ�����������ó�NULL������ʹ�ö�Ӧ�����Ĺ���
	*		����2�����ǽ������C/Sģ�ͣ�ɵ�������ĺ��Ĺ���
	* 
	* ����1��int           nfds,
	*		��0�ͺ��ˣ���Ϊ�˼�����ǰ��һ���汾Berkeley sockets
	*		���ں��񼸺�������
	* 
	* ����2�� fd_set        *readfds,
	*		����Ƿ��пɶ���socket�������пͻ��˵���Ϣ�ˣ������յ�����������ᱻ����
	*		��������ʱ���������е�socket���ϣ����Ǿ���������ϵͳ�Ὣ���¼���socket���ͻ������������
	*		�����ȵ��������ý��������������ֻʣ�������¼�������socket
	*		�����������select���ɵ�������Ĺؼ�����
	* 
	* ����3��fd_set        *writefds,
	*		����Ƿ��п�д��socket������������ӵĿͻ��ˣ���send������Ϣ��socket
	*		һ��ʼ������ʱ������socket�ļ��ϣ��Ⱦ���������ϵͳ�Ὣ�ܹ�����send��socketװ�����������
	*		�ȵ��������ý��������������ֻʣ�����ܹ�����send��socket
	*		��ʵ����ʵ�ã���ΪֻҪ�пͻ������ӣ������������Ҳ���ܹ�ֱ�ӷ��͵�
	*		����һ��ֱ���ں������send�ˣ���������õĲ�����
	* 
	* ����4��fd_set        *exceptfds,
	*		����׽����ϵ��쳣���󣬱��綪������Ϣ����������
	*		�÷��Ͳ���2��3һ���������쳣��socketװ����������У�����������
	*		�õ��쳣�׽����ϵľ��������
	*
	* ����5��const timeval *timeout
	*		һ���ṹ�����ͣ���Ա�ֱ��ǣ��롢΢��
	*		�������������ȴ�ʱ�䣬��������һ��socket�����ȶ��û��Ӧ���ż���������һ��socket
	*		�ڵȴ�ʱ���ڣ��ڼ�����socket���¼���������socket������Ȼ�������һ��socket
	*		�������ó�0��0 ���ǲ��ȴ���û���¼�ֱ��������������״̬
	*		��NULL�Ļ������ǲ�ʹ�����������Ҳ���ǲ�ʹ��������ܣ�
	*		����������������Ǽ���һ��socket��һֱɵ��
	* 
	* ����ֵ��
	*		0���ͻ����ڵȴ�ʱ����û�з�Ӧ������ʽ��continue�����ˣ�����ѭ����ͷ������
	*		>0:�пͻ�����������
	*		SOCKET_ERROR:�����˴���
	*		
	*
	*/
	//3.4 select��������accept��recv
	/*
	* ѭ����ʼ��ʱ��
	*		fd_set Tempsocket = Allsockets;	//���м��������¼���ϣ�����select�����ڲ���ı��������
	* 
	* ����Ƿ��������¼���
	*		��Ҫ��accept����������
	*		����Ͳ��ò���2��3��������ȡ��Ӧ�Ŀͻ�����Ϣ����getpeername()��getsockname()�������ɻ�ȡ
	*		���Ӻ�//�����ӵĿͻ��˵�socketװ�������У�FD_SET(socketclient, &Allsockets);
	*		��һ��д�ɰ�Tempsocket.fd_array[i]װ���������ˣ����Ǵ�ģ���Ϊ�µ�socket��û�ڼ�����
	* 
	* ����ǿͻ��˵��¼���
	*		ע�⣺���ǲ���ʱ�رտͻ��˵Ĵ��ڣ�����ǿ�����ߡ����������ߣ���һ�����󣬲��������ͻ������ߣ���������10054
	*		�������󣬻��߿ͻ����������ߣ�Ҫ����socket�Ӽ�����ɾ����Ҳ��select��ɾ�������������
	*		Ҫ���м�������������ʱ�Ὣselect�ж���ԭ��λ�õ�����socket���޳�
	*		
	*/
	//3.5 select����ͻ�����������
	/*
	* �ͻ����������ߣ�
	*		�ͻ�����ͷ��Ҫ����WSACleanup();���������ر�
	*		���������ڿͻ�����һ��Ľ�����������0��ʱ��͵���WSACleanup();
	*		���������ر���
	* ������
	*		����Ҫ����ɶ��
	*		����10054�������Ӧ��socket����
	* ѧϰ��
	*		��fd_set��ʵ�֣�F12�����ǾͿ��Ժܺõ�ѧϰ
	*		��ʦ˵����Լ�����涨���ģ����Ǻö�����ֵ��ѧϰ
	*/
	//3.6 select �������3����д��socket����
	/*
	* send��������ֵ��
	*		ֻ�����ַ���ֵ���ɹ����ط��͵��ֽ���
	*		ʧ�ܷ���SOCKET_ERROR������ͻ��������رգ��ͻ��˵���WSACleanup����Ҳ�Ƿ���SOCKET_ERROR
	*		������Щ�˾�������Ȼ����Ϊ�����ַ���ֵ����Ϊ�����رշ���0����ʵ�Ǵ��
	*		�����������������ϣ�Ҫ���ǵ���ʵ�ԣ�Ҫ�����Լ���֤
	* 
	* ����3��writesocket
	*		����3�ļ����п���ɾ���������������socket
	*		��ɾҲû��ϵ����Ϊ���������Լ����޷����ͣ���д���ģ����Թ���writesocket��û�з��ط�������socket��
	*		
	*/
	//3.7 select ����4 �쳣������
	/*
	* ����4��fd_set        *exceptfds,
	*		����׽����ϵ��쳣���󣬱��綪������Ϣ����������
	*		������ò��ԣ���Ϊ��Ҫ��������г��ֶ������
	*		
	* ������
	*		getsockopt ��socket��SOL_SOCKET��  SO_ERROR, buf , buflen��
	*		����1��Ҫ���м���socket��
	*		����2��SOL_SOCKET ����ʾ����1��һ��socket������������Ĺ��ܣ�
	*		����3: �õ�ͨ�Ź����е��쳣����Ĵ����루��MSDN�ϲ鿴��������һЩ��
	*		����4��������������ͨ���쳣����Ĵ������ �ַ�������
	*		����5������4���ֽڴ�С
	*		����ֵ���������������д��󣬷���SOCKET_ERROR
	* ע�ⷵ��ֵ������ͨ�ŵĴ����룬����������������Ƿ�ɹ���ѶϢ
	*/
	//3.8 select ģ���ܽ�
	/*
	* ����ṹ��
	*		selectģ�͵Ĵ��룬Ҳ����ϸ�ڵ��Ż����ṹ���б仯
	*		�����ҵĴ��룬�����ǵĴ���϶����в�һ���ĵط�
	*		���Ǻ��ĵ��߼���ԭ����һ����
	*		����д�����������˼·��򵥵ķ������������ģ��
	* 
	* �ṹ���ģ�
	*		���ǲ���2�������accept��recv ɵ������������
	*		�����select�ԱȻ���ģ����������
	*		���ϵ�һЩ���룬����������3������4��������NULL
	*		����3���п��ޣ���Ϊ��ʱ���ܹ�sand����һ��Ҫ����������send
	* 
	* �����ܽ᣺
	*		������socketװ��һ��������
	*		���������select���������������������¼���Ӧ�ģ�������Ӧ��socket����װ��һ�飬Ȼ�󰤸�������
	*		����ֵ == 0��û�м���������Ӧ��socket
	*		����ֵ >0 ������������Ӧ��socket
	*		����ֵ == SOCKET_ERROR select����ִ��ʧ��
	*		����Ӧ�ĸ��ݲ������У��ɶ�����д���쳣
	*		�ɶ���socket������������Ӧ����accept���ͻ������ӣ����ͻ��˵���Ӧ����recv���տͻ�����Ϣ
	* 
	* select�������ģ�
	*		select��ִ������������ִ��accept��recv��sendʱ�����������ݵĸ���ճ�������У��޷��������������ִ��
	*		�����Ļ������������ô���һ�࣬���Ŵ��ӳ٣���ú���
	*		����5Ϊ 0 0 ���ȴ�����ִ������
	*		����5Ϊ������ֵ����ִ������+��������Ҫ�ȴ�һ��ʱ��ż�����һ��socket��
	*		����5ΪNULL����ִ������+Ӳ���������ȣ�Ҫ�ȵ�����ӦΪֹ���ż�����һ��socket����ɵ�������е����ƣ�����һ����
	* 
	*����ģ��ѧϰ��
	*		���Ǿ���һ��һ������������ģ��������ȱ��
	*		Ȼ����ʲôģ�����������ȱ�㣬���ȫ��������������һ����������ȫ�˿�ģ����	
	*/
	//3.9 ����̨�ر��¼�
	/*
	* ѧϰ˼�룺
	*		��ʦ˵��Ҫ�����ڴ�����棬����Ҫ���Ǳ�̵�˼�룬����select�����ɵ������������
	*		˼��������ô����ģ���Щ˼��������ǹ���ѧϰҪ���۵ģ�Ҳ�Ǹ��������Լ��ķ���
	*		������̱߳ȽϺõĽ����ִ�����������⣬��������ȫ���
	* 
	* �ͷ�����socket���ر���·�⣺
	*		�������ʱҪ���socket������selectģ��һֱ����ѭ����
	*		��ʱ��Ҫ����socket�Ļ����͵���ѭ���еĴ�������break
	*		��ִ��ĳ�����󣬱������ǹرշ��������ڵ�ʱ�򣬻��д���ǿ�����ߣ�
	*		���������������break��Ȼ�����ִ�����socket���ر���·����
	*		�����Լ�����һ�������˳���������Ȼ��ִ��break
	*		
	* ���ߣ�
	*		�����ù��ӣ���ʦ˵�漰����ɫ�ش�����Ը��ི��
	*		�����ò���ϵͳ����ɣ�����ϵͳ���������һ�еĴ��ڲ���
	*		���ûص�����������д�õĺ�������ϵͳȥ���ã����ǻص�������
	*		�����ǹرշ������˿�ʱ���͵�������д�õĺ�����
	*		��������ڷ�������д�õ��ͷ�����socket������������
	* 
	* �ص�������
	*		SetConsoleCtrlHandler(fun, TRUE);
	*		����������˼������ϵͳ�ȵ����Լ���Ĭ�Ϻ�����Ȼ�����TRUE����FALSE�������Ƿ���������Լ�����ĺ���fun
	*		TRUE���ǲ���ϵͳ�������Լ���Ĭ�Ϻ������ͻ�ִ�������Լ�д�ĺ���fun��FALSE��ִ�����ǵ���д����
	*		��д�ĺ��������Ǿ�д���ͷ�����socket
	*	��д�����й̶��ĸ�ʽ��BOOL WINAPI fun(DWORD dwCtrlType);
	*	CTRL_CLOSE_EVENT�ǹرմ��ڵĲ���ִ���˾ͻ����������ֵ
	* 
	*	BOOL WINAPI fun(DWORD dwCtrlType)
	*	{
	*		switch (dwCtrlType)
	*		{
	*		case CTRL_CLOSE_EVENT:
	*			for (u_int i = 0; i < Allsockets.fd_count; i++)
	*				closesocket(Allsockets.fd_array[i]);
	*		}
	*		return TRUE;
	*	}
	*		
	*	
	*/