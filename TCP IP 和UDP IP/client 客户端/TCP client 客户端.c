#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			//strlen的头文件
#include <WinSock2.h>		//网络头文件
#pragma comment(lib, "WS2_32.lib")


int main(void)
{
	//打开网络库
	WORD wsVersion = MAKEWORD(2, 2);
	WSADATA wssockMsg;
	int nRse = WSAStartup(wsVersion, &wssockMsg);
	if (nRse != 0)
	{
		switch (nRse)
		{
		case WSASYSNOTREADY:
			printf("底层网络子系统尚未准备好进行网络通信, 系统配置问题，重启下电脑，检查ws2_32库是否存在（C盘查找ws2_32）");
			break;
		case WSAVERNOTSUPPORTED:
			printf("要使用的版本不支持,请检查版本，更新网络库");
			break;
		case WSAEPROCLIM:
			printf(" 已达到对Windows套接字实现支持的任务数量的限制，即65536个端口全用满了。请尝试关闭不必要的软件，为软件运行提供充足的资源");
			break;
		case WSAEINPROGRESS:
			printf(" 当前函数运行期间，由于某些原因造成阻塞，会返回在这个错误码。请重新启动软件");
			break;
		case WSAEFAULT:
			printf(" lpWSAData参数不是有效指针。程序员自己的问题，启动函数WSAStartup的参数2写错啦");
			break;

		}
	}
	//校验版本
	if (2 != HIBYTE(wssockMsg.wVersion) || 2 != LOBYTE(wssockMsg.wVersion))
	{
		printf("网络库版本不对");
		WSACleanup();
		return 0;
	}
	//创建SOCKET(服务端的吗)
	int socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketServer == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	//连接到服务器
	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	Server.sin_port = htons(22258);
	Server.sin_addr.S_un.S_un_b.s_b1 = 127;	//输入IP地址的方式1，分4次输入（因为是联合）
	Server.sin_addr.S_un.S_un_b.s_b2 = 0;
	Server.sin_addr.S_un.S_un_b.s_b3 = 0;
	Server.sin_addr.S_un.S_un_b.s_b4 = 1;
	//Server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	int co = connect(socketServer, (struct sockaddr*)&Server, sizeof(Server));
	if (co == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketServer);
		WSACleanup();
		return 0;
	}
	printf("服务器已连接 \n");
	//与服务器进行收发
	char ca[1500] = {0};
	/*int sen = send(socketServer, "信息已发送", sizeof("信息已发送"), 0);
	if (sen == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
	}*/
	while (1)
	{
		////接收消息
		//int rev = recv(socketServer, ca, 1499, 0);
		//if (rev == 0)
		//	printf("断开连接、客户端下线");
		//else if (rev == SOCKET_ERROR)
		//{
		//	int a = WSAGetLastError();
		//	//不用关闭网络库，因为只是一个客户端断开
		//}
		//else
		//	printf("%d   %s\n", rev, ca);


		//给服务器发送一个回馈
		int s = scanf("%s", ca);
		if (ca[0] == '0')	//输入 0 则退出循环，为的是select模型中测试正常关闭客户端
			break;
		int sen1 = send(socketServer, ca, strlen(ca), 0);
		if (sen1 == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
		}
		
	}
	



	closesocket(socketServer);
	WSACleanup();


	system("pause>0");
	return 0;
}