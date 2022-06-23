#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//创建socket放到全局，方便点×退出函数调用，注意主函数里不要再写声明了，会有二义性
SOCKET SerClient;

BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerClient);
		WSACleanup();
		break;
	}
	return 0;
}

int main(void)
{
	//向系统投递一个监视（点×退出的监视）
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);
	//打开网络库
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
	//校验版本
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("网络库版本不正确 error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//创建SOCKET （UDP的）
	SerClient = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == SerClient)
	{
		int a = WSAGetLastError();
		printf("创建SOCKET error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//创建服务器的端口号、IP地址结构体
	struct sockaddr_in stServer;
	stServer.sin_family = AF_INET;
	stServer.sin_port = htons(22258);
	stServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");


	//接收与发送
	while (1)
	{
		//sendto发送信息
		char str1[548] = {0};
		gets(str1);
		int snum = sendto(SerClient, str1, strlen(str1), 0, (const struct sockaddr*)&stServer, sizeof(stServer));
		if (SOCKET_ERROR == snum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		//recvfrom接收信息
		struct sockaddr stServer;	//参数5，用于记录对方socket（客户端）的端口与地址
		int nlen = sizeof(stServer);//参数6，参5结构体大小
		char cbuf[547] = {0};		//参2，字符数组，存放接收到的信息
		int rnum = recvfrom(SerClient, cbuf, 547, 0, &stServer, &nlen);
		if (SOCKET_ERROR == rnum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		printf("  %s\n", cbuf);
	}

	//释放socket、关闭网络库
	closesocket(SerClient);
	WSACleanup();

	system("pause>0");
	return 0;
}
