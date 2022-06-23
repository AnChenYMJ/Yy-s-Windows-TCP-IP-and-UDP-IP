#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//创建socket放到全局，方便点×退出函数调用，注意主函数里不要再写声明了，会有二义性
SOCKET SerVerSocket;

//点×退出，处理函数
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
	//创建SOCKET （UDP的）（声明放到全局，方便点×退出函数调用）
	SerVerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		printf("创建SOCKET error: %d\n", a);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//绑定端口号与地址
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

	//selcet处理
	while (1)
	{
		//1.socket创建 fd_set集合
		fd_set fd;					//结构体变量创建
		FD_ZERO(&fd);				//集合清零
		FD_SET (SerVerSocket, &fd);	//添加一个socket元素
		//2.select查询
		TIMEVAL st;			//结构体对象
		st.tv_sec = 5;		//秒
		st.tv_usec = 5;		//微秒
		int sele = select(0, &fd, NULL, NULL, &st);
		//3.查询到了，进行处理
			//超时处理，配合参数5
		if (0 == sele)
		{
			printf("等待超时了\n");
			continue;
		}
			//函数执行出错，错误处理
		if (SOCKET_ERROR == sele)
		{
			int a = WSAGetLastError();
			printf("select error: %d", a);
			continue;
		}
			//有事件响应，进行处理
		if (0 < sele)
		{
			//recvfrom接收信息
		struct sockaddr stclient;	//参数5，用于记录对方socket（客户端）的端口与地址
		int nlen = sizeof(stclient);//参数6，参5结构体大小
		char cbuf[548] = { 0 };				//参2，字符数组，存放接收到的信息
		int rnum = recvfrom(SerVerSocket, cbuf, 548, 0, &stclient, &nlen);
		if (SOCKET_ERROR == rnum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}

		printf("  %s\n", cbuf);
		//sendto发送信息
		int snum = sendto(SerVerSocket, "Server 收到\n", strlen("Server 收到\n"), 0, (const struct sockaddr*)&stclient, sizeof(stclient));
		if (SOCKET_ERROR == snum)
		{
			int a = WSAGetLastError();
			printf("recvfrom error: %d\n", a);
		}
		}
	}

	//释放socket、关闭网络库
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}


void studybook12(void)
{
	//3.1 UDP/IP的select模型 1
	{
		/*
			课前劝告：老师说我们写的这些模型是基于知识的最基本、最简练、最原始的代码模型
					是不能直接用到实际项目中去的，实际项目要根据需求来进行更多优化与修改、添加，这些基本模型有很多BUG
		 
			select模型流程：
				1.添加网络库，网络头文件
				2.打开网络库
				3.校验版本
				4.创建socket
				5.绑定端口号、地址
				6.select

			select对比C/S模型：
				解决基于tcp/ip协议的c/s模型中accept rcev 傻等的问题，让这俩函数不会傻等，有对应socket信号产生才调用相关函数
				但是UDP/IP协议的c/s模型不存在傻等的问题，不过能让recvfrom灵活一些
			select逻辑
				1、所有socket装进一个集合FD_SET，UDP只有一个socket
				2、通过select函数，检测1中的socket集合哪个有响应，也就是检查有没有消息来了
				3、有了，处理：recvfrom

				第一步：创建一个socket集合
					fd_set网络定义好的专门给咱们用的
						转定义看下fd_set的声明
							默认FD_SETSIZE是64, 大家可以在Winsock2.h前声明这个宏，给他更大的值，他就更大了，不要过大
							因为原理就是不听的遍历检测，越多肯定效率越低，延迟越大，所以大家尽量不要太大，几百个，1024就差不多了，当然大家不怕慢，可以设置更多~
							所以，select模型应用，就是小用户量访问量，几十几百，简单方便。
					四个操作fd_set的参数宏
						依次转定义看下逻辑：清零的逻辑是让计数归零即可，计数记录有效元素，让计数归零更高效简洁
							FD_ZERO：将集合清0，FD_ZERO(&clientSet);
							FD_SET：向集合中添加一个socket，当数量不足64，并且不存在的时候，FD_SET(socket, &setRead);
							FD_CLR：结合中删除指定socket，FD_CLR(socket, &setRead); 我们要手动释放
							FD_ISSET：判断一个socket是否在集合中，不在返回0，在返回非0，FD_ISSET(socket, &setRead)
		*/
	};
	//3.1 UDP/IP的select模型 2
	{
		/*
			select函数：

			函数原型：
					int WSAAPI select
					(
					  int           nfds,
					  fd_set        *readfds,
					  fd_set        *writefds,
					  fd_set        *exceptfds,
					  const timeval *timeout
					);

			作用：
				监视socket集合，如果某个socket发生事件（链接或者收发数据），通过返回值以及参数告诉我们

			参数1：忽略，填0，这个参数仅仅是为了兼容Berkeley sockets.
			参数2：检查是否有可读的socket，即客户端发来消息了，该socket就会被设置，就是这个参数会被返回有事件的socket，是一个socket集合
					不过UDP只有一个socket，就是服务器本身的socket
			参数3：检查是否有可写的socket，没有客户端，就不用了（因为是面向非链接，没有客户端的socket，此参数对UDP来讲就是无意义的了，填NULL就不用了）
			参数4：检查套接字上的异常错误，也不用了，我们就一个socket，填NULL就能不用
			参数5：最大等待时间，就是select遍历到当前socket，是否要等待事件产生，或者不等待
					可设置成两种，效果又分为三类：
						1.TIMEVAL 结构体，有两个成员，tv_sec：，tv_usec：微秒
							0 0，非阻塞状态，立刻返回，缓解了死等
							3 4，那就再无客户端响应的情况下等待3秒4微秒
						2.NULL  select完全阻塞，直到客户端有反应，我才继续
							此时跟recvfrom本来就一样了
					解决recvfrom的死等问题就设置成 0 0 就是不让它等（只有一个socket，所以能让recvfrom更灵活）
			返回值：
					0：客户端在等待时间内没有反应，处理：continue就行了
					>0：有客户端请求交流了
					SOCKET_ERROR：发生了错误，得到错误码WSAGetLastError()

			总结：
				1.等待状态更灵活，select是执行阻塞的，根据参数5有三类状态
					不等待 0 0 ：执行阻塞
					半等待 3 4 ：执行阻塞+软阻塞
					全等待 NULL：执行阻塞+硬阻塞，死等
				2.select模型对于基本UDP/IP模型没什么大加强
				3.但是要注意到select模型的使用，意义稍微与tcp/ip不一样了，看参数的意义
				4.学习知识在于熟练掌握知识的精髓，万变不离其宗
				5.参数5设置成有等待时间，然后超时处理中，能进行一些我们想要的操作，做点别的然后再去等待，可以很灵活
		*/
	};
}