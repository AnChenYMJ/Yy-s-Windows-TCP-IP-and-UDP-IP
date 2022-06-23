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
HANDLE hPort;		//完成端口创建声明

//投递函数声明
int PostRecvFrom();
int PostSendTo(struct sockaddr_in* se);
//线程函数的声明。创建线程时使用
DWORD WINAPI ThreadProc(LPVOID lpParameter);
//控制线程函数循环的变量
BOOL THread = TRUE;

//控制台点×退出，处理函数
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		THread = FALSE;				//控制线程函数内的循环
		closesocket(SerVerSocket);
		WSACloseEvent(wol.hEvent);	//注意关闭的是结构体中的事件对象
		CloseHandle(hPort);			//关闭完成端口，（属于系统对象）
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

	//创建完成端口
	hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == hPort)
	{
		int a = WSAGetLastError();
		printf("完成端口创建失败 error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//绑定完成端口
	hPort = CreateIoCompletionPort((HANDLE)SerVerSocket, hPort, SerVerSocket, 0);
	if (0 == hPort)
	{
		int a = WSAGetLastError();
		printf("完成端口绑定socket失败 error: %d\n", a);
		WSACloseEvent(wol.hEvent);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//创建线程
		//获取cpu核数
	SYSTEM_INFO gsi;	//创建一个结构体
	GetSystemInfo(&gsi);//获取到cpu的一些信息，返回到这个结构体中
	int cpuNum = gsi.dwNumberOfProcessors;	//从结构体中获取核数
		//开始创建线程
	for (int i = 0; i < cpuNum; i++)
	{
		DWORD ThreadID;	//线程ID，尾参数
		if (NULL == CreateThread(NULL, 0, &ThreadProc, 0, 0, &ThreadID))
		{
			//如果当前这跟线程创建失败,让i--即可
			int a = WSAGetLastError();
			printf("创建线程失败 error: %d\n", a);
			i--;
		}	
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
	
	//循环卡住，主线程阻塞
	while (1)
	{
		//每次执行，都将所在线程挂起2000ms
		Sleep(2000);	
	}


	//释放socket、关闭网络库
	WSACloseEvent(wol.hEvent);		//点×退出函数也要加上这个释放事件、句柄，关闭事件对象
	closesocket(SerVerSocket);
	CloseHandle(hPort);				//关闭完成端口
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
	//走线程函数了
	if (0 != nRse)
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
	//完成端口无论如何都走线程函数，所以立即完成那条线舍弃掉了
	if(0 != nRse)
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
//线程函数，创建线程时的参数3
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	DWORD NumberOfByte;			//参数2：接收或者发送得字节数
	ULONG_PTR CompletionKey;	//参数3：完成端口函数的参数3会传递值到这个参数里
	WSAOVERLAPPED* lpwol;		//参数4：是重叠结构地址的地址，二级指针，会将当前通知的重叠结构地址返回到这个参数
	int a = 0;
	//用一个死循环来一直向通知队列取通知来处理
	while (THread)
	{
		//如果失败了
		if (FALSE == GetQueuedCompletionStatus(hPort, &NumberOfByte, &CompletionKey, &lpwol, INFINITE))
		{
			continue;
		}
		//给结构体里的事件置空
		WSAResetEvent(lpwol->hEvent);
		//成功了，进行分类处理
			//客户端发送回车，或者是我们send给客户端成功
		if (ReBuf[0] == 0)
		{
			printf("send回调函数 ok\n");

		}
			//接收到了客户端信息
		else
		{
			printf("%s \n", ReBuf);
			PostSendTo(&si);			//投递send
			memset(ReBuf, 0, 548);		//清0
			PostRecvFrom();

		}
	}
	return 0;
}

void study()
{
	//7.1 完成端口知识回顾
	{
		/*
		* 知识回顾：
		*		详细的知识点看课件
		*		完成端口也是windows的一种机制，
		*		完成端口的特点：
		*			1.模仿消息队列，创造一个通知队列，系统来创建维护
		*			2.创建最佳效率的线程，即核心数，充分利用CPU性能
		*		与完成例程类似，有回调函数的机制，就不走立即完成的那条线路，而是走回调函数的线路
		*		而完成端口也是一样的，不走事件通知分类中的立即完成，而是走通知队列的路线
		*	
		*		投递的函数如RecvFrom等，有了信号后产生事件通知，将通知存储到通知队列中，然后依次取出来用最优线程数来处理这些事件
		*			解决了顺序，效率，线程数量瓶颈等
		*		完成端口就能理解为这个队列的队列头
		*		
		* 代码部分：
		*		本课是对UDP重叠IO事件通知的改动
		*		主要添加修改：
		*			创建完成端口
		*			绑定完成端口
		*			创建线程
		* 
		*			主函数中的循环用Sleep来卡住，此函数执行时，会自动将所在线程挂起，不占用cpu时间，提高效率
		*		
		*/
	}
	//7.2 创建完成端口并绑定
	{
		/*
		*	知识点：
		*		完成端口也是windows的一种机制，不是网络专属，文件操作都能用
		*		创建与绑定完成端口的函数是同一个，只是参数不同，详情看课件
		*		参数忽略或者默认，一般是填0
		*		创建出的完成端口要记得释放，使用CloseHandle函数释放
		*		
		*	返回值：
		*		成功：	参2为NULL：返回新的完成端口
		*				参2不为NULL：返回自己
		*				参1为socket：返回绑定后的完成端口
		*/
	}
	//7.3 创建线程
	{
		/*
		*	创建线程函数：
		*		详情看课件，各个参数的作用与分类都有介绍
		*		参数5的一个宏的作用很大，用的最多，就是将这跟线程不用时挂起的那个宏
		*		线程ID要了解,线程ID与线程句柄是一个意义
		*		参数4是给线程函数（参3）传递值的作用，本次暂时用0代替
		*		返回值：
		*			成功返回线程句柄（内核对象，结尾需要释放，用CloseHandle函数）
		*			失败返回NULL
		*			
		*	知识点补充：
		*		栈的保留大小能够修改，默认为1M
		*		创建线程前要先获取最佳线程数
		*		线程函数 是 创建线程函数的参数3 ， 同时，线程函数是回调函数的一种
		*		只要是回调函数，写法都是固定的，包括调用约定，参数，返回值等
		*/
	}
	//7.4 线程函数内部（就相当于回调函数内部）
	{
		/*
		*	要点：
		*		因为UDP 与 TCP的模型结构是一样的，只是对应函数的参数有所区别
		*		所以我就没有写太多笔记，大多在TCP都已经记过一遍了，而且课件写的很详细，看课件更好
		*		与完成例程一样，只要写了回调函数（就是线程函数），那就一定会走回调函数这条路线，事件通知模型的立即完成那条线可以舍弃掉了
		*			因为先走了立即完成，然后又走了线程函数
		*		因为我们用了死循环卡住，所以要关闭，就在控制台窗口关闭函数，来添加一个线程函数的死循环的控制，比如从true变为false
		* 
		*	线程函数内部：
		*		尝试从指定的I / O完成端口出列I / O完成数据包：GetQueuedCompletionStatus函数
		* 
		*/
	}
	


}
