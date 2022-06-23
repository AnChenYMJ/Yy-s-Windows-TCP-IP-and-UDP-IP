#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

//自定义结构体，用于记录要被绑定的事件与socket
struct FD_se_set
{
	unsigned short index;								//下标
	SOCKET		SocketAll[WSA_MAXIMUM_WAIT_EVENTS];		//socket
	WSAEVENT	EventAll[WSA_MAXIMUM_WAIT_EVENTS];		//事件
	///WSA_MAXIMUM_WAIT_EVENTS 是宏 为64 ，因为等待信号函数WSAWaitForMultipleEvents，一次最多只能传64个给它
};

//方法一：解决事件处理上限为64个
struct FD_se_set1
{
	unsigned short index;			//下标
	SOCKET		SocketAll[1024];	//socket
	WSAEVENT	EventAll[1024];		//事件
};

int main(void)
{
	//打开网络库 WORD MAKEWORD WSADATA WSAStartup()
	WORD WSversion = MAKEWORD(2, 2);	//版本格式转换
	WSADATA WSsockMsg;					//参数2，接住一些反馈信息
	int nrse = WSAStartup(WSversion, &WSsockMsg);
	if (nrse == 0)
	{
		switch (nrse)
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
	//校验版本	HIBYTE LOBYTE
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		printf("网络库版本不对");
		WSACleanup();
		return 0;
	}
	//创建SOCKET		socket()
	SOCKET Serversocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	//三个参数决定协议是哪种
	if (Serversocket == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定端口号，地址 bind
	struct sockaddr_in st;
	st.sin_family = AF_INET;		//地址类型
	st.sin_port = htons(22258);		//端口号转类型
	st.sin_addr.S_un.S_un_b.s_b1 = 127;	//地址
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	int brse = bind(Serversocket, (const struct sockaddr*)&st, sizeof(st));
	if (brse == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//开启监听 
	if (listen(Serversocket, SOMAXCONN) == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//创建事件
	WSAEVENT ServerEvent = WSACreateEvent();
	if (ServerEvent == WSA_INVALID_EVENT)
	{
		int a = WSAGetLastError();
		closesocket(Serversocket);
		WSACleanup();
		return 0;
	}
	//给事件绑定socket、操作码，投递给操作系统来监视
	if (SOCKET_ERROR == WSAEventSelect(Serversocket, ServerEvent, FD_ACCEPT | FD_READ | FD_CLOSE | FD_WRITE))
	{
		int a = WSAGetLastError();		//得到错误码
		closesocket(Serversocket);		//清除套接字
		WSACloseEvent(ServerEvent);		//清除句柄（一定要）
		WSACleanup();					//关闭网络库
		return 0;
	}
	//用结构体把要绑定的socket、事件，装到一起，用下标来记录
	//struct FD_se_set allES = { {0},{0},{NULL} };
	//allES.SocketAll[allES.index] = Serversocket;	//socket数组
	//allES.EventAll[allES.index]  = ServerEvent;		//事件数组
	//allES.index++;									//下标

	//循环来获取处理有信号产生的事件（没有进行有序优化的版本，我已弃用）
	{
		//while (1)
		//{
		//	//等待信号（询问信号）
		//	DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES.index, allES.EventAll, FALSE, WSA_INFINITE, FALSE);
		//	//如果失败了
		//	if (WSA_WAIT_FAILED == WSwaitforevent)
		//	{
		//		int a = WSAGetLastError();		//得到错误码
		//		printf("错误码：%d\n", a);
		//		break;
		//	}
		//	//如果超时了
		//	if (WSA_WAIT_TIMEOUT == WSwaitforevent)
		//	{
		//		continue;
		//	}
		//	//成功了，通过运算得到响应事件的下标
		//	DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

		//	//列举操作，获取有信号的事件的触发操作类型，并且重置事件信号
		//	WSANETWORKEVENTS NetWorkEvents;		//参3的结构体，用来接住函数传递的一些信息
		//	if (SOCKET_ERROR == WSAEnumNetworkEvents(allES.SocketAll[nIndex], allES.EventAll[nIndex], &NetWorkEvents))
		//	{
		//		int a = WSAGetLastError();		//得到错误码
		//		printf("错误码：%d\n", a);		//这个函数失败的错误码，与参3中的错误码数组无关
		//		break;
		//	}

		//	//分类处理
		//	//先判断FD_ACCEPT操作码（即事件触发了信号，我们来判断它触发的操作码是什么，然后进行处理）
		//	if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//	{	//筛选掉FD_ACCEPT可能按位或的结果，得到了自己的二进制码


		//		//如果没有发生错误
		//		if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//		{
		//			//接受连接		//参1就是Serversocket服务器的socket，也只有它能得到FD_ACCEPT的操作码
		//			SOCKET clientSocket = accept(allES.SocketAll[nIndex], NULL, NULL);	//后两参数是客户端的信息，可以单独用函数获取，这里填NULL
		//			if (INVALID_SOCKET == clientSocket)
		//				continue;
		//			//socket绑定事件（每个socket都要绑定事件）
		//			WSAEVENT clienteve = WSACreateEvent();
		//			if (WSA_INVALID_EVENT == clienteve)
		//			{
		//				closesocket(clientSocket);
		//				continue;
		//			}
		//			//投递给系统
		//			if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//			{
		//				closesocket(clientSocket);
		//				WSACloseEvent(clienteve);
		//				continue;
		//			}
		//			//将客户端的socket和事件添加到结构体集合中
		//			allES.SocketAll[allES.index] = clientSocket;
		//			allES.EventAll[allES.index] = clienteve;
		//			allES.index++;
		//		}
		//		//如果发生了错误
		//		else
		//			continue;
		//		printf("FD_ACCEPT\n");
		//	}
		//	//判断FD_WRITE	
		//	if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//	{
		//		//可以在这里初始化，（编程思想经验，在逻辑上产生的最前面，且只产生一次，可以进行初始化一些东西）

		//		//没有错误，能进行消息发送
		//		if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//		{
		//			if (SOCKET_ERROR == send(allES.SocketAll[nIndex], "connect success", strlen("connect success"), 0))
		//			{
		//				int a = WSAGetLastError();
		//				printf("send faild, error code:%d\n", a);
		//				continue;
		//			}
		//		}
		//		//有错误码
		//		else
		//		{
		//			printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//打印错误码
		//			continue;
		//		}

		//		printf("FD_WRITE\n");
		//	}
		//	//判断FD_READ
		//	if (NetWorkEvents.lNetworkEvents & FD_READ)
		//	{
		//		//没有错误，能读取消息
		//		if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//		{
		//			char read[1500] = { 0 };
		//			if (SOCKET_ERROR == recv(allES.SocketAll[nIndex], read, 1499, 0))
		//			{
		//				int a = WSAGetLastError();
		//				printf("recv error code:%d\n", a);	//打印错误码
		//				continue;
		//			}
		//			printf("recv data:%s\n", read);
		//			continue;
		//		}
		//		//有错误码
		//		else
		//		{
		//			printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//打印错误码
		//			continue;
		//		}
//	}
//	//判断FD_CLOSE
//	if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
//	{
//		//FD_CLOSE 当客户端有下线（不论是强制还是正常）都会有一个错误码：10053（所以与上面的其他操作码的处理不一样，这个不会有0的情况出现）
//		printf("close client\n");
//		printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//打印错误码
//		closesocket(allES.SocketAll[nIndex]);						//释放下线的客户端的socket
//		allES.SocketAll[nIndex] = allES.SocketAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
//		WSACloseEvent(allES.EventAll[nIndex]);
//		allES.EventAll[nIndex] = allES.EventAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
//		allES.index--;
//	}

//	////switch来判断（不推荐使用，无法进行 按位或 的分类处理）
//	//switch (NetWorkEvents.lNetworkEvents)
//	//{
//	//case FD_ACCEPT:
//	//	//如果没有发生错误
//	//	if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
//	//	{
//	//		
//	//		SOCKET clientSocket = accept(allES.SocketAll[nIndex], NULL, NULL);
//	//		if (INVALID_SOCKET == clientSocket)
//	//			continue;
//	//		WSAEVENT clienteve = WSACreateEvent();
//	//		if (WSA_INVALID_EVENT == clienteve)
//	//		{
//	//			closesocket(clientSocket);
//	//			continue;
//	//		}			
//	//		if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
//	//		{
//	//			closesocket(clientSocket);
//	//			WSACloseEvent(clienteve);
//	//			continue;
//	//		}
//	//		allES.SocketAll[allES.index] = clientSocket;
//	//		allES.EventAll[allES.index] = clienteve;
//	//		allES.index++;
//	//	}
//	//	//如果发生了错误
//	//	else
//	//		continue;
//	//	printf("FD_ACCEPT\n");
//	//	break;
//	//case FD_WRITE:
//	//	//没有错误，能进行消息发送
//	//	if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
//	//	{
//	//		if (SOCKET_ERROR == send(allES.SocketAll[nIndex], "connect success", strlen("connect success"), 0))
//	//		{
//	//			int a = WSAGetLastError();
//	//			printf("send faild, error code:%d\n", a);
//	//			continue;
//	//		}
//	//	}
//	//	//有错误码
//	//	else
//	//	{
//	//		printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//打印错误码
//	//		continue;
//	//	}
//	//	printf("FD_WRITE\n");
//	//	break;
//	//case FD_READ:
//	//	//没有错误，能读取消息
//	//	if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
//	//	{
//	//		char read[1500] = { 0 };
//	//		if (SOCKET_ERROR == recv(allES.SocketAll[nIndex], read, 1499, 0))
//	//		{
//	//			int a = WSAGetLastError();
//	//			printf("recv error code:%d\n", a);	//打印错误码
//	//			continue;
//	//		}
//	//		printf("recv data:%s\n", read);
//	//		continue;
//	//	}
//	//	//有错误码
//	//	else
//	//	{
//	//		printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//打印错误码
//	//		continue;
//	//	}
//	//	break;
//	//case FD_CLOSE:
//	//	printf("close client\n");
//	//	printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//打印错误码
//	//	closesocket(allES.SocketAll[nIndex]);						//释放下线的客户端的socket
//	//	allES.SocketAll[nIndex] = allES.SocketAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
//	//	WSACloseEvent(allES.EventAll[nIndex]);
//	//	allES.EventAll[nIndex] = allES.EventAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
//	//	allES.index--;
//	//	break;
//	//}
//}
	};

	//有有序优化的版本
	{
		//while (1)
		//{
		//	//等待信号（询问信号）
		//	DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES.index, allES.EventAll, FALSE, WSA_INFINITE, FALSE);
		//	//如果失败了
		//	if (WSA_WAIT_FAILED == WSwaitforevent)
		//	{
		//		int a = WSAGetLastError();		//得到错误码
		//		printf("错误码：%d\n", a);
		//		break;
		//	}
		//	//如果超时了
		//	if (WSA_WAIT_TIMEOUT == WSwaitforevent)
		//	{
		//		continue;
		//	}
		//	//成功了，通过运算得到响应事件的下标
		//	DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

		//	//有序优化来解决变态点击问题，
		//	for (int i = nIndex; i < allES.index; i++)
		//	{
		//		//从有信号的最小下标开始向后，依次一个一个的来判断执行，每次等待1个事件的，这个事件完了，按照标顺序往后遍历处理，这一组按顺序完了，才进行下一组
		//		DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES.EventAll[i], FALSE, 0, FALSE);	//此处参4高改成0，不要等待，不然某一个事件没响应就一直等了
		//		if (WSA_WAIT_FAILED == NeiSwaitforevent)
		//		{
		//			int a = WSAGetLastError();		//得到错误码
		//			printf("错误码：%d\n", a);
		//			break;
		//		}
		//		if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
		//		{
		//			continue;
		//		}

		//		//获取到信号后，进行操作
		//		//列举操作，获取有信号的事件的触发操作类型，并且重置事件信号
		//		WSANETWORKEVENTS NetWorkEvents;		//参3的结构体，用来接住函数传递的一些信息
		//		if (SOCKET_ERROR == WSAEnumNetworkEvents(allES.SocketAll[i], allES.EventAll[i], &NetWorkEvents))
		//		{
		//			int a = WSAGetLastError();		//得到错误码
		//			printf("错误码：%d\n", a);		//这个函数失败的错误码，与参3中的错误码数组无关
		//			break;
		//		}

		//		//分类处理
		//		//先判断FD_ACCEPT操作码（即事件触发了信号，我们来判断它触发的操作码是什么，然后进行处理）
		//		if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//		{	//筛选掉FD_ACCEPT可能按位或的结果，得到了自己的二进制码


		//			//如果没有发生错误
		//			if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//			{
		//				//接受连接		//参1就是Serversocket服务器的socket，也只有它能得到FD_ACCEPT的操作码
		//				SOCKET clientSocket = accept(allES.SocketAll[i], NULL, NULL);	//后两参数是客户端的信息，可以单独用函数获取，这里填NULL
		//				if (INVALID_SOCKET == clientSocket)
		//					continue;
		//				//socket绑定事件（每个socket都要绑定事件）
		//				WSAEVENT clienteve = WSACreateEvent();
		//				if (WSA_INVALID_EVENT == clienteve)
		//				{
		//					closesocket(clientSocket);
		//					continue;
		//				}
		//				//投递给系统
		//				if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//				{
		//					closesocket(clientSocket);
		//					WSACloseEvent(clienteve);
		//					continue;
		//				}
		//				//将客户端的socket和事件添加到结构体集合中
		//				allES.SocketAll[allES.index] = clientSocket;
		//				allES.EventAll[allES.index] = clienteve;
		//				allES.index++;
		//			}
		//			//如果发生了错误
		//			else
		//				continue;
		//			printf("FD_ACCEPT\n");
		//		}
		//		//判断FD_WRITE	
		//		if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//		{
		//			//可以在这里初始化，（编程思想经验，在逻辑上产生的最前面，且只产生一次，可以进行初始化一些东西）

		//			//没有错误，能进行消息发送
		//			if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//			{
		//				if (SOCKET_ERROR == send(allES.SocketAll[i], "connect success", strlen("connect success"), 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("send faild, error code:%d\n", a);
		//					continue;
		//				}
		//			}
		//			//有错误码
		//			else
		//			{
		//				printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//打印错误码
		//				continue;
		//			}
		//			printf("FD_WRITE\n");
		//		}
		//		//判断FD_READ
		//		if (NetWorkEvents.lNetworkEvents & FD_READ)
		//		{
		//			//没有错误，能读取消息
		//			if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//			{
		//				char read[1500] = { 0 };
		//				if (SOCKET_ERROR == recv(allES.SocketAll[i], read, 1499, 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("recv error code:%d\n", a);	//打印错误码
		//					continue;
		//				}
		//				printf("recv data:%s\n", read);
		//				continue;
		//			}
		//			//有错误码
		//			else
		//			{
		//				printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//打印错误码
		//				continue;
		//			}
		//		}
		//		//判断FD_CLOSE
		//		if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
		//		{
		//			//FD_CLOSE 当客户端有下线（不论是强制还是正常）都会有一个错误码：10053（所以与上面的其他操作码的处理不一样，这个不会有0的情况出现）
		//			printf("close client\n");
		//			printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//打印错误码
		//			closesocket(allES.SocketAll[i]);						//释放下线的客户端的socket
		//			allES.SocketAll[i] = allES.SocketAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
		//			WSACloseEvent(allES.EventAll[i]);
		//			allES.EventAll[i] = allES.EventAll[allES.index - 1];	//交换最后一个元素与我们释放掉元素的位置
		//			allES.index--;
		//		}
		//	}
		//}
	};

	//方法1：一个一个的，解决事件处理上限为64个
	{
		////为了方法1，而单独写的结构体，成员上限提高了
		//struct FD_se_set1 allES1 = { {0},{0},{NULL} };
		//allES1.SocketAll[allES1.index] = Serversocket;	//socket数组
		//allES1.EventAll[allES1.index] = ServerEvent;		//事件数组
		//allES1.index++;									//下标

		//while (1)
		//{
		//	for (int i = 0; i < allES1.index; i++)
		//	{
		//		//等待信号（询问信号）
		//		DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES1.EventAll[i], FALSE, 0, FALSE);
		//		if (WSA_WAIT_FAILED == NeiSwaitforevent)
		//		{
		//			int a = WSAGetLastError();		//得到错误码
		//			printf("错误码：%d\n", a);
		//			break;
		//		}
		//		if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
		//		{
		//			continue;
		//		}

		//		//获取到信号后，进行操作
		//		//列举操作，获取有信号的事件的触发操作类型，并且重置事件信号
		//		WSANETWORKEVENTS NetWorkEvents;		//参3的结构体，用来接住函数传递的一些信息
		//		if (SOCKET_ERROR == WSAEnumNetworkEvents(allES1.SocketAll[i], allES1.EventAll[i], &NetWorkEvents))
		//		{
		//			int a = WSAGetLastError();		//得到错误码
		//			printf("错误码：%d\n", a);		//这个函数失败的错误码，与参3中的错误码数组无关
		//			break;
		//		}

		//		//分类处理
		//		//先判断FD_ACCEPT操作码（即事件触发了信号，我们来判断它触发的操作码是什么，然后进行处理）
		//		if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
		//		{	//筛选掉FD_ACCEPT可能按位或的结果，得到了自己的二进制码

		//			//如果没有发生错误
		//			if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
		//			{
		//				//接受连接		//参1就是Serversocket服务器的socket，也只有它能得到FD_ACCEPT的操作码
		//				SOCKET clientSocket = accept(allES1.SocketAll[i], NULL, NULL);	//后两参数是客户端的信息，可以单独用函数获取，这里填NULL
		//				if (INVALID_SOCKET == clientSocket)
		//					continue;
		//				//socket绑定事件（每个socket都要绑定事件）
		//				WSAEVENT clienteve = WSACreateEvent();
		//				if (WSA_INVALID_EVENT == clienteve)
		//				{
		//					closesocket(clientSocket);
		//					continue;
		//				}
		//				//投递给系统
		//				if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
		//				{
		//					closesocket(clientSocket);
		//					WSACloseEvent(clienteve);
		//					continue;
		//				}
		//				//将客户端的socket和事件添加到结构体集合中
		//				allES1.SocketAll[allES1.index] = clientSocket;
		//				allES1.EventAll[allES1.index] = clienteve;
		//				allES1.index++;
		//			}
		//			//如果发生了错误
		//			else
		//				continue;
		//			printf("FD_ACCEPT\n");
		//		}
		//		//判断FD_WRITE	
		//		if (NetWorkEvents.lNetworkEvents & FD_WRITE)
		//		{
		//			//可以在这里初始化，（编程思想经验，在逻辑上产生的最前面，且只产生一次，可以进行初始化一些东西）

		//			//没有错误，能进行消息发送
		//			if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
		//			{
		//				if (SOCKET_ERROR == send(allES1.SocketAll[i], "connect success", strlen("connect success"), 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("send faild, error code:%d\n", a);
		//					continue;
		//				}
		//			}
		//			//有错误码
		//			else
		//			{
		//				printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//打印错误码
		//				continue;
		//			}

		//			printf("FD_WRITE\n");
		//		}
		//		//判断FD_READ
		//		if (NetWorkEvents.lNetworkEvents & FD_READ)
		//		{
		//			//没有错误，能读取消息
		//			if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
		//			{
		//				char read[1500] = { 0 };
		//				if (SOCKET_ERROR == recv(allES1.SocketAll[i], read, 1499, 0))
		//				{
		//					int a = WSAGetLastError();
		//					printf("recv error code:%d\n", a);	//打印错误码
		//					continue;
		//				}
		//				printf("recv data:%s\n", read);
		//				continue;
		//			}
		//			//有错误码
		//			else
		//			{
		//				printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//打印错误码
		//				continue;
		//			}
		//		}
		//		//判断FD_CLOSE
		//		if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
		//		{
		//			//FD_CLOSE 当客户端有下线（不论是强制还是正常）都会有一个错误码：10053（所以与上面的其他操作码的处理不一样，这个不会有0的情况出现）
		//			printf("close client\n");
		//			printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//打印错误码
		//			closesocket(allES1.SocketAll[i]);						 //释放下线的客户端的socket
		//			allES1.SocketAll[i] = allES1.SocketAll[allES1.index - 1];//交换最后一个元素与我们释放掉元素的位置
		//			WSACloseEvent(allES1.EventAll[i]);
		//			allES1.EventAll[i] = allES1.EventAll[allES1.index - 1];	 //交换最后一个元素与我们释放掉元素的位置
		//			allES1.index--;
		//		}
		//	}	
		//}
	};

	//方法2：一组一组的，我的完成版
		//用结构体把要绑定的socket、事件，装到一起，用下标来记录
		struct FD_se_set allES2[20];							//每个数组成员是一个结构体，每个结构体最多64个socket、事件
		memset(allES2, 0, sizeof(allES2));						//初始化
		allES2[0].SocketAll[allES2[0].index] = Serversocket;	//socket数组
		allES2[0].EventAll[allES2[0].index] = ServerEvent;		//事件数组
		allES2[0].index++;

		while (1)
		{
			for (int wai = 0; wai < 20; wai++)
			{
				//因为采用了结构体数组，所以一些元素结构体为0，不符合WSAWaitForMultipleEvents函数的参数规范，我们要加上判断
				if (0 == allES2[wai].index)
				{
					continue;
				}
				//等待信号（询问信号）
				DWORD WSwaitforevent = WSAWaitForMultipleEvents(allES2[wai].index, allES2[wai].EventAll, FALSE, 0, FALSE);
				//如果失败了
				if (WSA_WAIT_FAILED == WSwaitforevent)
				{
					int a = WSAGetLastError();		//得到错误码
					printf("错误码：%d\n", a);
					break;
				}
				//如果超时了
				if (WSA_WAIT_TIMEOUT == WSwaitforevent)
				{
					continue;
				}
				//成功了，通过运算得到响应事件的下标
				DWORD nIndex = WSwaitforevent - WSA_WAIT_EVENT_0;

				//有序优化来解决变态点击问题，
				for (int i = nIndex; i < allES2[wai].index; i++)
				{
					//从有信号的最小下标开始向后，依次一个一个的来判断执行，每次等待1个事件的，这个事件完了，按照标顺序往后遍历处理，这一组按顺序完了，才进行下一组
					DWORD NeiSwaitforevent = WSAWaitForMultipleEvents(1, &allES2[wai].EventAll[i], FALSE, 0, FALSE);	//此处参4高改成0，不要等待，不然某一个事件没响应就一直等了
					if (WSA_WAIT_FAILED == NeiSwaitforevent)
					{
						int a = WSAGetLastError();		//得到错误码
						printf("错误码：%d\n", a);
						break;
					}
					if (WSA_WAIT_TIMEOUT == NeiSwaitforevent)
					{
						continue;
					}

					//获取到信号后，进行操作
					//列举操作，获取有信号的事件的触发操作类型，并且重置事件信号
					WSANETWORKEVENTS NetWorkEvents;		//参3的结构体，用来接住函数传递的一些信息
					if (SOCKET_ERROR == WSAEnumNetworkEvents(allES2[wai].SocketAll[i], allES2[wai].EventAll[i], &NetWorkEvents))
					{
						int a = WSAGetLastError();		//得到错误码
						printf("错误码：%d\n", a);		//这个函数失败的错误码，与参3中的错误码数组无关
						break;
					}

					//分类处理
					//先判断FD_ACCEPT操作码（即事件触发了信号，我们来判断它触发的操作码是什么，然后进行处理）
					if (NetWorkEvents.lNetworkEvents & FD_ACCEPT)
					{	//筛选掉FD_ACCEPT可能按位或的结果，得到了自己的二进制码

						//如果没有发生错误
						if (0 == NetWorkEvents.iErrorCode[FD_ACCEPT_BIT])
						{
							//接受连接		//参1就是Serversocket服务器的socket，也只有它能得到FD_ACCEPT的操作码
							SOCKET clientSocket = accept(allES2[wai].SocketAll[i], NULL, NULL);	//后两参数是客户端的信息，可以单独用函数获取，这里填NULL
							if (INVALID_SOCKET == clientSocket)
								continue;
							//socket绑定事件（每个socket都要绑定事件）
							WSAEVENT clienteve = WSACreateEvent();
							if (WSA_INVALID_EVENT == clienteve)
							{
								closesocket(clientSocket);
								continue;
							}
							//投递给系统
							if (SOCKET_ERROR == WSAEventSelect(clientSocket, clienteve, FD_READ | FD_CLOSE | FD_WRITE))
							{
								closesocket(clientSocket);
								WSACloseEvent(clienteve);
								continue;
							}
							//将当前连接的客户端的socket和事件添加到结构体集合中
							//如果当前的结构体装满了，则装到下一个元素（结构体）中
							for (int n = 0; n < 20; n++)
							{
								if (allES2[n].index < 64)
								{
									allES2[n].SocketAll[allES2[n].index] = clientSocket;
									allES2[n].EventAll[allES2[n].index] = clienteve;
									allES2[n].index++;
									break;
								}
							}		
						}
						//如果发生了错误
						else
							continue;
						printf("FD_ACCEPT\n");
					}
					//判断FD_WRITE	
					if (NetWorkEvents.lNetworkEvents & FD_WRITE)
					{
						//没有错误，能进行消息发送
						if (0 == NetWorkEvents.iErrorCode[FD_WRITE_BIT])
						{
							if (SOCKET_ERROR == send(allES2[wai].SocketAll[i], "connect success", strlen("connect success"), 0))
							{
								int a = WSAGetLastError();
								printf("send faild, error code:%d\n", a);
								continue;
							}
						}
						//有错误码
						else
						{
							printf("socket error code:%d\n", NetWorkEvents.iErrorCode[FD_WRITE_BIT]);	//打印错误码
							continue;
						}
						printf("FD_WRITE\n");
					}
					//判断FD_READ
					if (NetWorkEvents.lNetworkEvents & FD_READ)
					{
						//没有错误，能读取消息
						if (0 == NetWorkEvents.iErrorCode[FD_READ_BIT])
						{
							char read[1500] = { 0 };
							if (SOCKET_ERROR == recv(allES2[wai].SocketAll[i], read, 1499, 0))
							{
								int a = WSAGetLastError();
								printf("recv error code:%d\n", a);	//打印错误码
								continue;
							}
							printf("recv data:%s\n", read);
							continue;
						}
						//有错误码
						else
						{
							printf("FD_READ_BIT error code:%d\n", NetWorkEvents.iErrorCode[FD_READ_BIT]);	//打印错误码
							continue;
						}
					}
					//判断FD_CLOSE
					if (NetWorkEvents.lNetworkEvents & FD_CLOSE)
					{
						//FD_CLOSE 当客户端有下线（不论是强制还是正常）都会有一个错误码：10053（所以与上面的其他操作码的处理不一样，这个不会有0的情况出现）
						printf("close client\n");
						printf("close faild code:%d\n", NetWorkEvents.iErrorCode[FD_CLOSE_BIT]);	//打印错误码
						closesocket(allES2[wai].SocketAll[i]);						//释放下线的客户端的socket
						allES2[wai].SocketAll[i] = allES2[wai].SocketAll[allES2[wai].index - 1];	//交换最后一个元素与我们释放掉元素的位置
						WSACloseEvent(allES2[wai].EventAll[i]);
						allES2[wai].EventAll[i] = allES2[wai].EventAll[allES2[wai].index - 1];	//交换最后一个元素与我们释放掉元素的位置
						allES2[wai].index--;
					}
				}
			}
			
		}
	
	

	//释放句柄、套接字（适用于提高上限的方法2）
	for (int m = 0; m < 20; m++)
	{
		for (int n = 0; n < allES2[m].index; n++)
		{
			//释放事件句柄
			WSACloseEvent(allES2[m].EventAll[n]);	//返回值是BOOL类型
			//释放套接字
			closesocket(allES2[m].SocketAll[n]);
		}
	}
	
	////释放事件句柄
	//WSACloseEvent(ServerEvent);	//返回值是BOOL类型
	////释放套接字
	//closesocket(Serversocket);
	//关闭网络库
	WSACleanup();
	

	system("pause>0");
	return 0;
}


void study()
{
	if (4.1 - 4.7)
	{
		//4.1 windows事件机制与消息机制
	/*
	* windows处理 用户行为 的两种方式：（很重要）
	*		消息机制：核心：消息队列
	*		事件机制：核心：事件集合
	* 老师说这俩，消息机制更多是整体结构的层面
	*			事件机制更多的是局部实现的层面
	*			这俩可以混和着来使用
	*
	* 消息机制：
	*		核心：消息队列
	*		处理过程：
	*			所有的用户操作，比如点击鼠标，按键盘，滚轮滚动，
	*			都存作一个数据，按照顺序存放进一个队列中（先进先出）
	*			队列的创建是由操作系统来完成的
	*		特点：
	*			消息队列由操作系统来维护，我们做的操作就是把消息取出来，进行处理和分类
	*			简单说就是我们不用管这个消息队列，只要知道怎么取里面数据，这个数据对应着我们用户的一些操作
	*			然后利用这个数据（消息）来执行相应的函数处理，这是程序员的工作
	*			消息队列有先后顺序
	*		其它：
	*			老师说接下来会教Win32，MFC等课程，这俩都是基于消息队列，先宏观理解
	*			我们下一个学习的模型，异步选择就是基于这个消息队列的
	*
	* 事件机制：事件集合	（软件实现的单片机中的中断）
	*		处理过程：
	*			根据需要，我们为用户的特定操作绑定一个事件，事件由我们自己调用API创建，要多少就能建多少
	*			将事件传递给系统，由系统帮我们监视，所以不能太多，不然系统就卡了
	*			如果操作发生了，比如按鼠标，按键，对应事件就会转换成相应的信号，用一个数来标记着
	*			然后我们获取到有信号的事件，对其进行处理
	*		特点：
	*			所有事件都是由我们定义的，系统只是帮我们放置信号
	*			事件的排序是无序的
	*			这次课的事件选择，就是基于事件机制来完成的
	*/
	//4.2 事件选择模型的处理逻辑
	/*
	* 消息队列：
	*		由操作系统创建，并且由操作系统监管，我们只要取出数据
	*		然后用这个数据进行处理
	* 事件机制：
	*		我们要创建事件，为事件绑定操作，然后由触发后系统给事件一个信号
	*		我们利用这个信号来进行处理
	*		我们要做的事比消息队列多
	*
	* 事件选择模型：（我还是觉得要用中断来看，更透彻，或者Qt中的事件）
	*		整体逻辑和select差不多，名字叫WSAEventSelect ，就是select的进阶版
	*	第1步：创建一个事件对象（变量）
	*	第2步：为每个事件对象绑定一个socket以及操作（accept、read、close）并且投递给系统
	*			投递给系统后，就不用管了，系统自己来管，我们就可以做别的操作了
	*	第3步：查看事件是否有信号
	*	第4步：有信号的话就分类处理
	*
	* 创造这些的前辈们总结了的精华，所以尽管复杂，但是我们使用前辈整理好的东西就比较简单
	*
	* 异步：在做一件事的时候，也能去做另一件事，就是解决执行阻塞的问题
	*/
	//4.3 创建事件对象，以及内核对象的简单讲解
	/*
	* 事件选择模型的流程：
	*		1.网络头文件，网络库
	*		2.打开网络库
	*		3.校验版本
	*		4.创建SOCKET
	*		5.绑定端口号，地址
	*		6.开始监听
	*		7.事件选择
	*
	* 事件选择：
	*		创建一个事件对象
	*		该对象绑定socket并投递给操作系统
	*
	* 创建事件对象：
	*		WSAEVENT WSAAPI = WSACreateEvent();
	*		WSAEVENT是一个句柄（可看作ID，唯一标识符）类型是viod*（无法确认内容与类型）
	*		这个创建对象是内核函数，内核是操作系统的空间
	*		我们的所有程序，都是在操作系统上完成的，可以看作一个更大的程序
	*		那我们如VS这些软件都是在这个大程序内完成运行的，那就可以简化看作是一个函数在主函数内运行
	*		所以我们VS申请的空间相当于局部变量，而内核函数创建的对象（空间）就相当于主函数内的变量，
	*		当VS的程序结束时，会释放掉它本身申请的空间，但是内核对象的空间是属于操作系统的，无法一起被释放掉
	*		所以我们要手动加上一个释放函数，释放它，内核函数的申请与释放的函数是成对出现的
	*		socket就是一个内核函数，所以需要手动调用closesocket（）来释放
	*		否则程序结束，内核对象还存在，就一直存在，导致操作系统内存越来越高
	*
	*		成功：返回一个事件
	*		失败：返回WSA_INVALID_EVENT，可以获取错误码，然后失败处理要释放掉socket，关闭网络库
	*		我们对WSAEVENT（句柄，在操作系统中常用到） F12得到类型是HANDLE，再深入得到void* （无法确认类型，内容，很适合当作操作系统函数的类型）
	*
	* 内核对象：
	*		由操作系统在内核申请
	*		由操作系统访问
	*		void*（通用类型指针）：我们无法定位其内容，也不能修改，出于对内核的保护，对规则的保护，从而使操作系统有序的、平稳的、有效的运行，不会随便出问题
	*		调用函数创建，调用函数释放：由于是操作系统的，所以不会自动在程序里释放，所以创建、释放是配套使用且要主动用的
	*		如果我们没有调用释放，那么他可能就一直存在于内核，造成内核内存泄漏， 这种只能重启电脑
	*		内核对象有：socket，Kernel Objects（去MSDN上可以查看）
	*
	* 几个函数：
	*		BOOL WSAAPI WSACloseEvent(WSAEVENT hEvent);	关闭释放事件句柄，不用就要释放，否则一直存在
	*		BOOL WSAAPI WSAResetEvent(WSAEVENT hEvent);	重置，我们下一课要学的内容，系统会将事件置成有信号的，而这个函数就能将句柄重置为无信号的
	*		BOOL WSAAPI WSASetEvent(WSAEVENT hEvent);	将指定事件主动设置成有信号
	*
	*/
	//4.4 WSAEventSelect 函数讲解（事件绑定socket与操作码，投递给操作系统）
	/*
	* WSAEventSelect：
	*		作用：给事件绑定上socket与操作码，并投递给操作系统
	*				用 | 或 能同时输入多个操作码
	*				系统根据事件是否有对应操作，给事件添加一个信号
	*
	*	参数1：被绑定的socket，  我们最后的目的是让每个socket都绑定上一个事件
	*	参数2：事件，就是事件对象，让参1 、参2绑定到一起
	*	参数3：要判断的具体事件（下面写的类型，我们主要关注学习前四个）
	*	返回值：成功返回 0、 失败返回SOCKET_ERROR，失败处理要释放句柄。
	*
	*		参数3是监视对应事件的，根据不同操作码选择，监视的内容也不一样，对应的内容发生了变化或者满足条件
	*		则会产生相应的信号，并返回来，监视的过程是由操作系统来完成的
	*
	* 参数3：（操作码）	（前4个是主要学习使用的）
	*		FD_ACCEPT	：有客户端连接，连接时就会产生该信号，与客户端socket绑定，与accept有关，accept有执行，操作系统就检测到，给事件一个信号
	*		FD_READ		：头客户端发来消息，有收到消息，则会产生该信号，与客户端socket绑定，与recv有关
	*						可以多个属性并列，比如： FD_READ | FD_ACCEPT 用或‘|’就可以整合出一个参数了
	*		FD_CLOSE	：客户端下线，与客户端socket绑定，在这里强制下线与正常下线都是一样的，与客户端socket绑定
	*		FD_WRITE	：可以给客户端发信息，会在accept后立即主动产生该信号，收到该信号可以说明客户端已连接成功，随时能send。，与客户端socket绑定
	*
	*		FD_CONNECT	：客户端来使用的，（客户端也能用事件选择模型），服务器绑定时，收到信号
	*		0			：取消事件监视，比如我们已经给socket绑定了一个事件，我们再WSAEventSelect（0）就会覆盖掉原先的，而0则是不监视，就取消掉原来的事件监视了
	*						所以多个事件监视连续写，后面的会将前面的覆盖掉
	*		FD_OOB		：外带数据，就是一段数据间插入一个神秘字节，一般不使用
	*		FD_QOS		：套接字服务质量的状态发生变化，则会产生信号，进行消息通知
	*						比如我们同时下载文件，看电影，听歌，浏览网页，那带宽的分配是不均衡的，可能某一个行为的优先级高
	*						给它分配的带宽就高，这时的服务质量发生改变，就会进行消息通知
	*						函数是：WSAIoctl（）老师说很强大，以后介绍
	*		FD_GROUP_QOS：保留的操作码类型，就像ASCII码有一些编码是没有对应字符的，是为了保留作用，以后更新换代，可能就用上了，老师说这种情况很普遍
	*
	*		重叠IO模型中介绍的：（这俩参数网上介绍比较少，后面会讲一下，比较偏门）
	*			FD_ROUTING_ INTERFACE_CHANGE：就是服务器与客户端之间的路由路径发生了改变，则会产生对应信号
	*											要通过WSAIoctl注册之后，才能使用
	*			FD_ADDRESS_ LIST_CHANGE		：想要接收套接字地址族的本地地址列表更改通知。就是我们服务器连接着很多客户端，就会有一个表来记录着这些客户端信息
	*											当客户端发生变化，比如多一个少一个，就会被监视产生返回相应信号
	*											要通过WSAIoctl注册之后，才能使用
	*/
	//4.5 逻辑完善优化
	/*
	* 完善优化思路：
	*		创建一个结构体
	*		成员是一个数（下标）、俩数组，数组分别记录着对应的socket和事件
	*		目的是根据下标，就能知道，这两个是要被绑定到一起，并投递给操作系统的
	*
	*		服务器能同时处理的socket，默认是64个，有一个宏：WSA_MAXIMUM_WAIT_EVENTS  他就是64这个数，是为了好辩认出是默认值，才定义的宏（我总结为：编程思想）
	*		那我们申请的数组大小也就定义为64
	*		不像select模型，能改变能处理的个数，因为select本身就是个数组，通过遍历，比较直接，
	*		事件选择是异步投放，由系统管理，咱们就不能随便修改了，要按照规则来
	*		数index用于当作下标，要在socket、事件装进去以后才 +1 ，否则数组就不是从0开始的啦，0被跳过啦
	*
	*		初始化时 事件数组因为句柄是void* 类型的，所以初始化成NULL，其实socket的数组也能初始化成NULL
	*/
	//4.6 等待信号（询问信号）函数，WSAWaitForMultipleEvents
	/*
	*	我们学习时要注意，其实网络编程，后面学的一些函数与这个函数的参数都差不多，都是那几种类型或作用
	*	所以应该是越学越简单的
	*
	* WSAWaitForMultipleEvents：
	*	要配合上一课写的结构体来使用，不用结构体也可以，就是麻烦很多，三个成员要单独定义管理
	*	我们不需要知道函数返回值具体内容是什么，类型是整形
	*	我们只要知道，返回值 减去 一个宏 == 得到对应事件的下标  就可以了
	*
	*	这个函数在重叠IO模型中有使用，就是说重叠IO模型与我们的事件机制是有关联的，
	*	我们学习的模型的路线整体思路：
	*		C/S模型 ：最垃圾，很多阻塞无法解决，要大量优化 --> 为了解决傻等阻塞，有了select模型，
	*		select模型本身也有缺点，比如执行阻塞没解决，客户端一多延迟就明显 --> 为了优化分成了两个思想：1.用windows的事件机制 2.用消息机制
	*		事件机制 --> 就演化了本课的事件选择模型 --> 再向事件机制的方向更深入优化，得到了重叠IO模型
	*		消息机制 --> 演化出了异步选择模型		 --> 再向消息机制的方向优化深入，得到了完成端口模型
	*
	*	作用：
	*		获取发生信号的事件（经过操作系统监控，有信号发生的事件就返回）
	*		这个函数也可以叫询问信号，反正与等待信号发生的意思是一样的
	*
	*	参数1：
	*		要询问的事件的个数，最大个数为WSA_MAXIMUM_WAIT_EVENTS （64）个，上一课的笔记有讲这个
	*		因为是操作系统来监视统计的，所以我们不能像select模型一样去修改它，这是固定的
	*
	*	参数2：
	*		事件列表，就是我们写好的事件的数组（集合）
	*
	*	参数3：
	*		事件等待的方式：TRUE 和 FALSE
	*		TRUE	:所有的事件都产生了信号，才会有函数返回（重叠IO模型使用，先不讲）
	*					（延迟很高，这个函数内的所有事件都有信号才返回，那提前有信号响应的事件，必须得等到其它事件有信号才能返回）
	*		FALSE	:任何一个事件产生了信号，就马上返回。
	*					函数返回值是一个数，函数返回值 减去 WSA_WAIT_EVENT_0（宏：为0） == 下标（不知道为啥要减）
	*					得到的下标表示事件对象的索引（通过下标能找到对应的事件），
	*					如果调用函数期间，有多个时间信号产生，则会返回下标最小的那一个
	*					不会按照响应信号的先后顺序，不论什么时候先还是后，都只返回这一堆响应信号中下标最小的事件
	*
	*	参数4：
	*		超时间隔，（最大等待时间），以毫秒为单位，与select的参数5一样的意义
	*		123		：表示等待123毫秒，超时返回WSA_WAIT_TIMEOUT
	*		0		：检查事件对象的状态后，立即返回，不论有没有信号
	*		WSA_INFINITE	：等待，直到有事件发生
	*		这个函数不是select遍历数组的方式来监视，挨个判断等待，这个函数是由操作系统统一监控，一起监控，不存在遍历的情况
	*		所以这个参数是以整个函数为单位的超时间隔，这一组事件没有一个响应，则超时，或者继续等
	*		而select是以每个客户端为单位，这两者是不一样的，注意区分
	*		所以我们用WSA_INFINITE就好了，反正函数外都是对响应的时间进行处理的，一直等着也没关系，不影响事件响应的返回延迟
	*
	*	参数5：
	*		TRUE	:重叠IO模型再讲
	*		FALSE	:事件选择模型我们填写FALSE即可
	*
	*	返回值：
	*		数组下标的运算值：根据参数3为TRUE 还是 FALSE来决定
	*				TRUE:所有信号均有响应
	*				FALSE：返回值 - WSA_WAIT_EVENT_0 == 响应事件在数组中的下标
	*		WSA_WAIT_IO_COMPLETION：
	*				参数5，填写TRUE，才会返回这个值
	*		WSA_WAIT_TIMEOUT：
	*				超时了，得到这个我们continue返回循环开头继续即可
	*				与参数3的选择有关，如果参数3填写为WSA_INFINITE，直到有响应才行，那就没有机会返回这个值
	*		失败了：返回WSA_WAIT_FAILED
	*/
	//4.7 列举操作信号
	/*
	* 细节不注意的话很容易出BUG，因为理解不到位，本课的参数3就是很细节的地方，容易出错
	*
	* 操作系统在我们投递给它事件后，就和我们判断等待啥的信号代码无关了，它监视它的，然后给事件信号
	* 我们要不要得到这个对应的事件是我们的事，不影响操作系统的监视
	*
	* 作用：
	*		1.获取事件的操作类型，就是事件响应后对应的操作，如 accept recv close等等
	*		2.将事件上的信号重置，当我们等待信号函数得到响应的事件后，要将这个事件的信号重置
	*
	* 参数1：
	*		对应的socket
	*
	* 参数2：
	*		对应的事件
	*
	* 参数3：（重要的细节）
	*		struct _WSANETWORKEVENTS{ long lNetworkEvents; int iErrorCode[FD_MAX_EVENTS];}
	*		触发的操作的类型都在这里装着，函数将对应事件的信息装到这个结构体中
	*		是一个结构体指针： LPWSANETWORKEVENTS ( 去掉LP就不是指针了，加上LP是指针类型的重命名)
	*			成员1：对应事件的具体的操作
	*						比如我们在投递时，用了 | 投递了多个操作码，那有可能两个操作同时触发时，
	*						两个操作码就进行了 按位 或的情况，那按位或得到的结果就不知道是哪个数值了(这种情况概率低但是还是存在的)
	*			成员2：错误码的数组，
	*						就是对应事件的触发操作，比如连接，发送，接收 这些操作没有成功，那些错误码就装到这里面
	*						装的顺序是这样的，我们对投递时的操作码F12能看到FD_READ_BIT 1、2、3、4都对应着相应的操作码
	*						而操作码由对应的操作来触发，所以，1、2、3的下标的空间来存储对应的操作错误码
	*						就能用来区分标记错误码是什么了，下标对应着操作码，操作码对应着触发操作，
	*
	* 返回值：
	*		成功：返回 0
	*		失败：返回 SOCKET_ERROR	，进行失败处理
	*
	*		注意：失败处理获取的错误码是本函数失败的错误码，参3成员2的错误码数组，是对应事件的触发操作（如recv等）失败的错误码
	*/
	}

	if (4.8)
	{
		//4.8 事件分类处理逻辑 1
		/*
		*
		* 为什么要判断：
		*			即事件触发了信号，我们来判断它触发的操作码是什么，然后进行处理
		*			比如是FD_ACCEPT操作码，那我们就知道了是有连接请求，那我们就进行accept接受连接，如果有错误码，就不连接
		*
		* 接4.7 的参数3，处理FD_ACCEPT
		*		每个操作码，F12可以看到，有唯一对应的两个宏（数值）FD_ACCEPT_BIT  和  FD_ACCEPT这样类似的
		*		FD_ACCEPT_BIT是1、2、3、4这样的有理数，每个操作码有唯一对应的，所以能够用来当作4.7中的参数3的数组下标
		*		只要这个下标对应的空间不为0，则是有错误码，如果下标对应的空间为0，则没有错误，下标对应着操作码唯一的标识
		*
		* 先进行FD_ACCEPT的按位与判断（分类处理）：
		*		筛选掉了按位或的意外，利用按位与本身，又得到了本身
		*		按位与运算，如果产生了4.7课中的两个操作码按位或了，那再按位与就回到原来的二进制了
		*		比如A B按位或了， 然后得到的二进制再跟 A进行按位与， 结果又得到了A得二进制（逻辑推理的结果，记住结论就好了）
		*/
		//4.9 事件分类处理逻辑 2
		/*
		* FD_ACCEPT 与 FD_WRITE:
		*		在有客户端连接的时候，操作系统会立马先创建一个FD_ACCEPT
		*		然后紧接着马上创建一个FD_WRITE，且只创建一次
		*		我们可以在对应的处理中加入打印，查看
		* 
		*		由于FD_WRITE在逻辑上在客户端连接过程的最前面，
		*		所以往往利用处理FD_WRITE时，进行初始化（很广泛的编程思想，逻辑上在前面的，可用来初始化）
		* 
		* FD_WRITE_BIT的错误码：
		*		是代表客户端连接过程中出现的错误，是一种公共错误的描述
		*		不是函数执行的错误码
		* 
		* strlen 与 sizeof的区别：
		*		strlen统计的字符数不包含字符串结尾的\0
		*		sizeof统计的字符数是包含了\0的
		*		同样的字符串，sizeof比strlen统计的数值大了1个
		* 
		* FD_CLOSE：
		*		发现不论怎么下线，都是有错误码10053
		*		释放的时候，因为数组空间是有限的，所以被删掉的元素，他原先的位置要被其它元素填补上
		*		我们直接用末尾元素来填补他的位置，（先删后填补）
		*		末尾元素的下标是[allES.index - 1]，这点要注意，思考一下就知道了，因为下标指向末尾元素的下一个
		*/
		//4.10 时间分类处理逻辑 3
		/*
		* 关于上节课FD_CLOSE操作码的错误码10053的问题：
		*		FD_CLOSE 用MSDN查看，有3种错误码，
		*		其中：WSAECONNABORTED是客户端下线（不论是强制还是正常下线），终止，超时，就会返回的
		*		所以FD_CLOSE是不存在返回0的情况的，无论如何都会返回一个错误码
		*		知道了这个，我们就不进行错误码为0的判断就好了
		* 
		* 测试1：
		*		我们在WSAWaitForMultipleEvents（等待信号处）下断点，运行
		*		然后登上客户端，发送数据给服务器，然后服务器F10查看运行情况
		*	结果
		*		此时服务器，因为断点原因，FD_WRITE和FD_READ会同时收到
		*		就会导致这俩进行了按位或的情况，按位或后的结果是十进制的3
		*		我们就能够观测到，我们写的分类判断是否有效
		*		且发现进行了FD_WRITE判断操作后，接着又进行了FD_READ的判断操作
		*		这是我们用 if else 的优点，else if 和 switch 都只执行一次，然后剩下的判断都不执行了
		*	总结
		*		我们已经把socket和事件绑定投递给操作系统监视了，所以我们程序调试卡住
		*		但是操作系统是正常运行监视产生信号的
		*		协议栈也是任然在工作，这也是封装的好处（有点不清楚）
		*		操作系统监视事件时，产生信号，在等待信号函数处将两个信号进行了按位或成了一个数值
		*		我们下面的判断就是将这个信号解析（分类处理：按位与的形式），然后判断执行对应操作
		* 
		* 测试2：
		*		用测试1的方式下断点，运行，开启客户端发送消息
		*		但是我们客户端在服务器断点的时候，连续发送几条消息
		*		然后继续运行服务器观测服务端收到消息的情况
		*	结果
		*		服务端收到了消息，但是并不是分几条来收的，而是整合成了一条
		*		这就是协议缓冲区的作用，客户端发送的东西，在服务器的协议栈（协议缓冲区）里存着
		*		然后连续发送，就都连续存在协议缓冲区，服务器继续运行，得到FD_READ消息，就进行recv的操作
		*		recv去取的时候，就把协议缓冲区的都取出来了，所以造成了整体打印出来的现象
		*	结论
		*		协议栈（协议缓冲区）在我们断点调试时，仍在工作，且把消息存在协议缓冲区
		* 
		* 测试3：
		*		使用else if 来进行测试1
		*		使用shitch来进行测试1
		*	else if:
		*		当判断完一个并执行后，就不执行剩下的判断了
		*		所以出现操作码按位或的情况，只能处理逻辑上在前面的那一个
		*	switch：
		*		无法进行操作码的解析，因为case 后无法进行运算
		*		所以就无法对操作码按位或进行处理，比如FD_CLOSE和FD_READ按位或以后得到3
		*		那没有操作码是3，那它无法解析（分类处理），也就无法执行任何操作
		*	总结
		*		书上推荐出现操作码按位或使用else if ，但它只执行一个，另一个不执行，不过效率高了一点，因为只用判断一次
		*		我们用的if ，逻辑上更完善
		*		switch无法处理操作码按位或的情况，（不要使用）
		*		操作码按位或的处理就叫做：事件分类处理逻辑，就是我们这3节课的内容
		*		
		*/
		//4.11 有序优化变态点击
		/*
		*	老师说Windows网络编程这本书，都是直译MSDN的内容，没有什么原创的东西
		* 
		* 变态点击问题：
		*		等待信号函数WSAWaitForMultipleEvents，会返回有信号产生的事件，
		*		但返回的是集合中（参数2）下标最小的
		*		那问题来了：如果在集合中，下标5的事件先响应，下标1的事件后相应，那根据这个函数的特性，是先返回下标1的事件，然后进行后续处理
		*		那在后续处理下标1的事件过程中，下标1的事件又响应了，那再次循环时，经过等待信号函数，又返回了下标1的事件
		*		那排在下标1后面的事件就要等到1完全没响应了才能处理，而且还是按照下标的大小逻辑来返回处理
		*		这样就不公正了，因为不按照先后顺序的话，先响应的不一定先处理，延迟就可能会很高
		*	解决方法
		*		所以我们在逻辑上进行一个有序处理优化，让整个过程更公平
		* 
		* 有序处理：
		*		第一次等待事件，然后从返回的下标开始用for循环依次向后一个一个的遍历判断处理
		*		这样下标1的处理完当前的操作，就到下标2的开始，然后3、4、5，直到遍历完一组，再从头开始
		*		并不是完全的公平、有序，但是比上面的要好很多
		*	书上的做法
		*		不是从下标0开始，而是外层循环第一次等待信号，然后从有信号的最小下标开始
		*		就比从0开始效率更高了一点
		*	
		*		不绝对的公平，只是相对公平，相对有序
		*		让大家在一轮循环下都能得到处理
		*		但是并不能完全解决顺序问题，只是达到相对公平
		*		所以，事件选择模型不能用于大用户，多访问
		* 
		* 注意：
		*		WSAWaitForMultipleEvents函数在内层循环中，要修改参数
		*		尤其是第4个参数，不要用宏，改用0或者有效数字（毫秒为单位）的等待事件
		*		用宏的话，就非要等到一个事件有信号才能继续，就出现了傻等阻塞
		*		
		*		
		*/
		//4.12 增加事件处理的数量上限 （重要）
		/*
		*	//描述比较简单，我们还是看代码结构比较理解深刻
		*	  这一课没有说线程的处理方式，但重要的是方法（编程思想），我们要理解深刻一点，这一课的内容
		* 
		* 事件处理的上限：
		*		等待信号函数WSAWaitForMultipleEvents，一次最多只能传64个给它，而不是操作系统只能投64个
		*		所以这就限制了我们的客户端数量，我们要想办法突破这个限制
		* 
		* 方法1：
		*		用大数组（修改结构体集合的上限），一次给等待信号传递1个，然后循环，每次循环只传一个，然后要多少，就循环几次，那就解决了一次只能传64个的上限
		*		结合线程池（暂不清楚啥意思）
		* 
		* 方法2：
		*		一组一组的来，不用改结构体，采用结构体数组与循环的结合，每次循环用一个结构体（加上一个循环，看代码理解）
		*		下次循环用另一个结构体，就解决了上限问题，但还分单线程与多线程
		*	单线程：
		*		单线程，一组一组顺序处理就好了
		*	多线程
		*		每个结构体都用一个线程，那每个线程都是最大64，就可以了
		*		创建多个线程，每个线程处理一个事件表，最大64
		*	
		*		
		*	注意：
		*		采用方法2时，有客户端连接事件发生，添加客户端的socket、事件时
		*		要注意当前结构体是否满了，我们用一个小循环添加到有空闲的结构体中
		*		因为采用了结构体数组，所以一些元素结构体为0，不符合WSAWaitForMultipleEvents函数的参数规范，
		*		所以我们要加上判断，在WSAWaitForMultipleEvents函数前
		* 
		*	不同的方法释放socket、句柄的方式是不一样的
		*		方法1要用循环
		*		方法2要用双层循环
		*	注意
		*		我为了把各种方法分开，用了{}
		*		但是要特别注意，用了{}，在{}内的变量都是局部变量
		*		那我们释放时，也只能在{}内释放，这点要注意，在{}外无法识别局部变量
		* 
		* 
		*/
		//4.13 select模型与事件选择模型的区别
		/*
		* 老师有一张图，比较清晰，我们可以看一下对比，第42课时
		* 
		* select与事件选择的共同点
		*		在分类处理以后都是一样的
		*		都是调用相关函数来进行接受连接、发送、接收消息、客户端下线处理
		* 
		* select与事件选择的不同点：
		*		
		*	事件选择：
		*		将事件与socket绑定投递给操作系统，由操作系统监视给信号
		*		然后事件选择投递后，因为异步的，操作系统运行不影响我们程序
		*		我们可以在程序内做任何事
		*		直到我们等待信号函数得到事件的下标
		*		然后开始处理，用WSAEnumNetworkEvents得到对应事件的操作码，让我们知道应该做什么
		*		再然后就进行分类处理，调用相关函数进行连接、发送等操作了
		*		执行send recv 在向协议缓冲区进行复制读写的过程中，是阻塞的
		*	select：
		*		将socket通过select函数传递给操作系统（不清楚，上网查），系统会挨个遍历检索，将有响应的socket装到参数2、3中
		*		执行select的过程中是同步的，程序不可以做其他事情，是阻塞的
		*		然后得到参数2、3中的有响应的socket，然后进行分类处理
		*		执行send recv 在向协议缓冲区进行复制读写的过程中，是阻塞的
		*		select参数2：将传进去的socket集合中，有收到连接请求的则返回到这个参数，我们就知道要进行accept了
		*		select参数3：将传进去的socket集合中，有收到信息请求的则返回到这个参数，我们就知道要进行recv了
		*/
	}
}
	
	
	