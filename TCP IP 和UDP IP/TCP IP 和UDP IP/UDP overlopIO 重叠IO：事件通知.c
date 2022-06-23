#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//创建socket、事件放到全局，方便点×退出函数调用，注意主函数里不要再写声明了，会有二义性
SOCKET SerVerSocket;//socket
WSAOVERLAPPED wol;	//重叠IO的结构体，要与socket绑定的那个
struct sockaddr_in si;
char ReBuf[548] = { 0 };
//WSABUF recvBuf1;

//投递函数声明
int PostRecvFrom();
int PostSendTo(struct sockaddr_in* se);

//控制台点×退出，处理函数
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerVerSocket);
		WSACloseEvent(wol.hEvent);	//注意关闭的是结构体中的事件对象
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
	//创建SOCKET （UDP的重叠IO使用的socket，WSASocket来创建）（声明放到全局，方便点×退出函数调用）
	SerVerSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
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

	//创建事件对象，是结构体中的事件对象
	wol.hEvent = WSACreateEvent();
	if (WSA_INVALID_EVENT == wol.hEvent)
	{
		int a = WSAGetLastError();
		printf("创建事件对象 error: %d\n", a);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//绑定socket与操作码
	int wsaes = WSAEventSelect(SerVerSocket, (HANDLE)wol.hEvent, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaes)
	{
		int a = WSAGetLastError();
		printf("绑定事件、socket并投递 error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//投递recv,返回0则代表函数出错（自定义的）
	if (0 == PostRecvFrom())
	{
		int a = WSAGetLastError();
		printf("投递函数PostRecvFrom error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//投递send，要用到客户端IP与端口号，所以在recv下面，recv投递的函数能获取
	
	//等待信号然后进行分类处理
	while (1)
	{
		//等待信号
		if (WSA_WAIT_FAILED == WSAWaitForMultipleEvents(1, &wol.hEvent, FALSE, WSA_INFINITE, FALSE))
		{
			continue;
		}
		//获取事件（即获取响应事件的类型）
		DWORD tr;		//参3，会返回发送或接收到的字节数到这里
		DWORD Flags;	//参5，装WSARecv的参数5
		if (FALSE == WSAGetOverlappedResult(SerVerSocket, &wol, &tr, TRUE, &Flags))
		{
			int a = WSAGetLastError();
			continue;
		}
		//重置信号（看TCP对这个的笔记）
		WSAResetEvent(wol.hEvent);
		//分类处理
		if (tr >= 0)
		{
			//接收数组没有内容，说明是send（逻辑漏洞，这里还可能是recv，客户端敲回车发送，没有数据内容，但还是recv）
			if (0 == ReBuf[0])
			{
				printf("send发送成功，延迟发送 \n");
				PostSendTo(&si);	//避免客户端发送换行（回车）（分析在笔记里，总之这是逻辑漏洞）
				PostRecvFrom();		//这两行是recv的情况的补充，即客户端敲回车的情况，但是加了这两行，遇到真的延迟发送可能又会有问题
			}
			//接收数组有内容，说明recv
			else
			{
				printf("%s \n", ReBuf);
				PostSendTo(&si);			//投递send
				memset(ReBuf, 0, 548);		//清0
				PostRecvFrom();
			}

			//也可以用两个封装函数的返回值判断
		}
	}


	//释放socket、关闭网络库
	WSACloseEvent(wol.hEvent);		//点×退出函数也要加上这个释放事件、句柄，关闭事件对象
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}

int PostRecvFrom()
{
	//参数2：接收信息的数组的结构体
	//char ReBuf[548] = {0};	//数组需要单独创建
	WSABUF recvBuf1;
	recvBuf1.len = 547;		//空一个空间给\0
	recvBuf1.buf = ReBuf;
	//参数4：接收成功的话，这里装着成功接收到的字节数
	DWORD NumberOfBytesRecvd = 0;
	//参数5：有好几个类型，UDP填0即可,需要创建变量赋值0，然后传址
	DWORD Flags = 0;
	//参数6：装客户端的IP地址和端口号,参数7：参数6的大小
	//struct sockaddr_in si;
	int nLen = sizeof(si);
	//投递recv函数
	int nRse = WSARecvFrom(SerVerSocket, &recvBuf1, 1, &NumberOfBytesRecvd, &Flags, (struct sockaddr*)&si, &nLen, &wol, NULL);
	//立即完成
	if (0 == nRse)
	{
		printf("%s \n", recvBuf1.buf);
		PostSendTo(&si);			//投递send
		memset(ReBuf, 0, 548);		//清0
		PostRecvFrom();
	}
	//出错 或 延迟完成
	else
	{
		int a = WSAGetLastError();
		//延迟完成
		if (WSA_IO_PENDING == a)
		{
			//函数返回1则代表延迟完成
			return 1;
		}
	}
	//函数返回0则代表出错了
	return 0;
}
int PostSendTo(struct sockaddr_in* se)
{
	//参数2：接收信息的数组的结构体
	//char ReBuf[548] = { 0 };	//数组需要单独创建
	WSABUF recvBuf;
	recvBuf.len = 547;		//空一个空间给\0
	recvBuf.buf = "ok \n";
	//参数4：发送成功的话，这里装着成功发送的字节数
	DWORD NumberOfBytesRecvd = 0;
	//参数5：有好几个类型，UDP填0即可,需要创建变量赋值0，然后传址
	DWORD Flags = 0;
	//参数7：参数6的大小
	int ilen = sizeof(struct sockaddr_in);
	int nRse = WSASendTo(SerVerSocket, &recvBuf, 1, &NumberOfBytesRecvd, 0, (struct sockaddr*)se, ilen, &wol, NULL);
	//立即完成
	if (0 == nRse)
	{
		printf("send发送成功，立即发送 \n");
	}
	//出错 或 延迟完成
	else
	{
		int a = WSAGetLastError();
		//延迟完成
		if (WSA_IO_PENDING == a)
		{
			//函数返回1则代表延迟完成
			return 1;
		}
	}
	//函数返回0则代表出错了
	return 0;
}


void study()
{
	//6.1 重叠IO模型基础知识
	{
		/*
		* 基础知识：
		*		详情看课件，课件总结得很好，我这里就简单的回顾下我觉得重要的：
		*			1.意义：重叠IO是windows提供的一种异步读写文件的机制
		*					recv与send在将协议传输的内容复制到数组中的过程，是同步、阻塞的，3个recv要等第1个完成后才能进行剩下的
		*					而重叠IO则是将文件读写的过程，放到单独开辟的线程中去等待完成，达到异步的效果
		*					所以重叠IO是对C/S模型的直接优化
		*			2.本质：定义一个结构体（WSAOVERLAPPED）与socket绑定
		*			3.对比理解：
		*					异步选择模型把消息与socket绑一起，然后系统以消息机制处理反馈
		*					事件选择模型把事件与socket绑一起，然后系统以事件机制处理反馈
		*					重叠IO模型把重叠结构与socket绑一起，然后系统以重叠IO机制处理反馈
		*			重叠IO的两种实现方式：
		*					事件通知：文件读写操作完成后，线程里的结构体通过事件的机制来通知主线程
		*					完成例程（回调函数）
		*			整体逻辑：
		*					就是先投递，然后分成立即完成或延迟完成
		*					延迟完成就循环等信号，整体逻辑与TCP的相同
		*					但是由于UDP协议的特点，不需要投递AcceptEx，框架逻辑会更简单
		*			循环等信号：WSAWaitForMultipleEvents函数
		*					这个函数很好用，在等待时会将所在线程暂时挂起，不消耗CPU的时间，即暂时消失，不消耗资源
		* 补充：
		*			socket要用WSAsocket来创建重叠IO能使用的socket，一般的socket不行
		*			事件对象就采用结构体成员wol.hEvent即可，便无需单独创建
		*	
		*/
	}
	//6.2 WSASocket函数的讲解
	{
		/*
		* 此函数的参数类型很多，老师花了35分钟来介绍它的参数有哪些宏能填，属于拓展知识，不需强记
		*		详情看课件与MSDN来理解，课件总结得比较好
		*	参数：
		*		参数1、2、3是协议的 地址类型、套接字类型、协议类型，此处UDP，填：AF_INET、SOCK_STREAM、IPPROTO_TCP
		*		参数4 填NULL ，此处不使用，此参数为设置套接字属性（详情看课件与MSDN）
		*		参数5 填0 此参数是保留位（一组socket的组ID，大概是想一次操作多个socket）
		*		参数6 填写WSA_FLAG_OVERLAPPED，创建一个供重叠IO模型使用的socket，其它类型的课件有介绍
		*	返回值：
		*		成功返回可用的socket，创建出socket，不使用以后记得释放，用函数closesocket
		*		失败返回INVALID_SOCKET
		*	
		*	参数类型太多了，记不太住，但是TCP的课程学好了，再来看MSDN的参数介绍就简单得多
		*/
	}
	//6.3 WSARecvFrom函数详解，异步接收消息
	{
		/*
		*	本课主要讲WSARecvFrom的参数，与函数的使用，课件更加详细
		*	函数原型课件中有
		* 
		*	参数：
		*		参数1：客户端socket
		*		参数2：接收信息的数组的结构体，两个成员为：数组长度，数组地址，所以数组要单独创建然后传址
		*		参数3：接收参数2的个数，就是一次接几个包这样的
		*		参数4：接收成功的话，这里装着成功接收到的字节数
		*		参数5：有好几个类型，UDP填0即可，但不能直接写0，需要创建变量赋值0，然后传址
		*		参数6：装客户端的IP地址和端口号：sockaddr
		*		参数7：参数6的大小
		*		参数8：重叠结构
		*		参数9：回调函数使用的，事件通知填NULL即可
		*	返回值：
		*		立即发生，返回0
		*		出错，返回SOCKET_ERROR，其中WSA_IO_PENDING为延迟完成
		* 
		*	此函数的应用逻辑（投递该函数）要单独封装成一个函数，然后主函数中在绑定socket与结构体的步骤后投递
		*	投递函数的声明与TCP不同，TCP的含有参数，因为需要知道客户端的socket，而UDP没有客户端socket，就不写
		* 
		*	
		*/
	}
	//6.4 WSASendTo函数详解,发送消息
	{
		/*
		*	WSASendTo函数的参数与WSARecvFrom的参数基本一样，可看课件区别
		* 
		*	注意点：recv中，接收信息的数组，每次接受完都要清0，清0的方式很多，不要局限，比如：
		*		1.memset(ReBuf, 0, 548); 用函数挨个清0
		*		2.ReBuf[0] = 0;  或者给头元素一个特殊值，然后使用的时候识别即可
		*	
		*	Send的投递逻辑与recv的逻辑基本一致
		*	但是send需要知道客户端的IP与端口号才能发送信息，所以一般在recv的下方配套出现，因为recv的投递函数能获取客户端IP与端口号
		*			
		*	补充：recv目前是用递归，可改成循环的
		*/
	}
	//6.5 循环等信号
	{
		/*
		* 本课补充点：
		*		此课sendto我们填写了547一个包，那么客户端recv也要填写成547，否则会有10040错误
		*		由于客户端是一发一收，所以服务器也应该要满足客户端的一发一收
		*		客户端能够把输入 换行 给过滤掉，加上if('\0' == Buf[0]) 的条件然后continue即可，换行在数组中存储就是\0 
		* 
		* 纠错：
		*	一开始的10057错误码，是因为我的创建socket的参数写成TCP的协议了，是因为老师的课件错误
		*		应该写成WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP.....的才对，即UDP协议的
		* 
		*	为什么一开始客户端敲回车，就输入不了了呢，而且服务器也无反应
		*		因为我们是用tr >= 0 发送或接受的字符来判断，reBuf[0] == 0 来判断是发送还是接收，
		*		如果客户端敲回车，那么其实服务器是接收recv才对，但是没有接收到东西而已（reBuf[0] == 0），应该给客户端一个回馈，客户端才能继续发送，
		*		但因为我们的判断逻辑，导致它在分类处理判断成了send的延迟发送，就没有给客户端回馈，客户端卡在recv等回馈的状态
		*		所以我们在分类处理的send逻辑中，给客户端回馈，并重新投递recv即可，
		*		原因还是逻辑不明确，空的recv，误判成了send的延迟发送
		*/
	}


}
