#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>			//strlen的头文件
#include <WinSock2.h>		//网络头文件
#pragma comment(lib, "WS2_32.lib")


int main(void)
{
	//打开网络库
	WORD wsVersion = MAKEWORD(2,2);
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
	//创建SOCKET
	int socketserver = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socketserver == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//socket绑定端口与地址
	struct sockaddr_in st;
	st.sin_family = AF_INET;	//地址类型
	st.sin_port = htons(22258);	//端口		htons转换成网络字节序
	st.sin_addr.S_un.S_un_b.s_b1 = 127;	//输入IP地址的方式1，分4次输入（因为是联合）
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	//st.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");	//地址
	int brse = bind(socketserver, (const struct sockaddr*)&st, sizeof(st));
	if (brse == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//开启监听
	int lise = listen(socketserver, SOMAXCONN);
	if (lise == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//接受连接，（本质是创建客户端socket）
	struct sockaddr_in sockClient;
	int len = sizeof(sockClient);
	SOCKET socketclient = accept(socketserver, (struct sockaddr *)&sockClient, &len);
	if (socketclient == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(socketserver);
		WSACleanup();
		return 0;
	}
	//与客户端收发信息
	char buf[1500] = { 0 };
	while (1)
	{
		//接收信息
		int rec = recv(socketclient, buf, 1499, 0);
		if (rec == 0)
			printf("断开连接、客户端下线");
		else if (rec == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
			//不用关闭网络库，因为只是一个客户端断开
		}
		else
			printf("%d   %s\n", rec, buf);

		//给客户端发送信息
		int s = scanf("%s", buf);
		int sed = send(socketclient, buf, strlen(buf), 0);
		if (sed == SOCKET_ERROR)
		{
			int a = WSAGetLastError();
		}
	}
	

	//清理网络库
	closesocket(socketserver);
	closesocket(socketclient);
	WSACleanup();


	system("pause>0");
	return 0;
}