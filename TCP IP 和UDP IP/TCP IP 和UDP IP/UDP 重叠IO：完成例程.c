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
//回调函数声明
void CALLBACK OnRecv(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
void CALLBACK OnSend(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);

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

	//等待信号然后进行分类处理
	while (1)
	{
		//循环等信号，完成例程尾参要填TRUE
		int nRse = WSAWaitForMultipleEvents(1, &wol.hEvent, FALSE, WSA_INFINITE, TRUE);
		//出错了
		if (WSA_WAIT_FAILED == nRse)
		{
			continue;
		}
		//一个完成例程成功执行完，则返回这个值
		if (WSA_WAIT_IO_COMPLETION == nRse)
		{
			continue;
		}
	}


	//释放socket、关闭网络库
	WSACloseEvent(wol.hEvent);		//点×退出函数也要加上这个释放事件、句柄，关闭事件对象
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}
//回调函数
void CALLBACK OnRecv(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	//重置信号
	WSAResetEvent(lpOverlapped->hEvent);
	//错误码不为0，则出错了
	if (0 != dwError)
	{
		printf("Recv回调函数 出错了\n");
		return;
	}
	//如果接收或发送字节数不为0,则为收到了信息
	if (0 != cbTransferred)
	{
		printf("%s \n", ReBuf);
		memset(ReBuf, 0, 548);
		PostSendTo(&si);
		PostRecvFrom();
		return;
	}
	//如果为0，可能是客户端输入了回车
	else if (0 == cbTransferred)
	{
		printf("%s \n", ReBuf);
		memset(ReBuf, 0, 548);
		PostSendTo(&si);
		PostRecvFrom();
		return;
	}
}
void CALLBACK OnSend(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	printf("send 回调函数success\n");
}
//投递函数逻辑封装
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
	int nRse = WSARecvFrom(SerVerSocket, &recvBuf1, 1, &NumberOfBytesRecvd, &Flags, (struct sockaddr*)&si, &nLen, &wol, OnRecv);
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
	int nRse = WSASendTo(SerVerSocket, &recvBuf, 1, &NumberOfBytesRecvd, 0, (struct sockaddr*)se, ilen, &wol, OnSend);
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
//笔记
void study()
{
	//6.6 重叠IO的完成例程
	{
		/*
		*	完成例程与事件通知的区别，请看课件，并且对比代码理解
		* 
		*	完成例程在事件通知的代码上做的修改：
		*		1.写回调函数给Recv与Send来使用，回调函数就是投递给系统后，检测到信号，就自动调用回调函数，无需我们分类处理
		*		2.WSARecvFrom与WSASendTo，投递函数的尾参数，从NULL修改为我们写好的回调函数
		*		3.主函数中的循环内将事件通知的逻辑全删掉，然后添加等信号函数的逻辑即可
		*		4.等信号函数尾参数要修改成TRUE，专给完成例程使用的
		* 
		*	知识点：
		*		1.对比回调函数与事件通知的获取事件函数，会发现两者的参数有一些是一样的
		*			说明回调函数其实是起到了类似于获取事件的功能，并且是自动调用的
		*		2.回调函数中要记得充值信号
		*		3.改成完成例程的逻辑之后，PostSendTo的逻辑上，就不走立即完成了，而是一直走回调函数这条路线
		*		4.回调函数定义时的CALLBACK是函数约定
		*		5.回调函数的参数详情看课件，更详细
		* 
		*	按上述修改完，成功能进行通信，但是依然回车问题没有解决
		*		我们在Recv的回调函数中添加一个逻辑即可，但尚不清楚是否有问题
		*
		*/
	}
}
