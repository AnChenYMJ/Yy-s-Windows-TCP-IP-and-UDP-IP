#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//创建socket、事件放到全局，方便点×退出函数调用，注意主函数里不要再写声明了，会有二义性
SOCKET SerVerSocket;//socket
WSAEVENT wsace;		//事件

//点×退出，处理函数
BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
	case CTRL_CLOSE_EVENT:
		closesocket(SerVerSocket);
		WSACloseEvent(wsace);
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

	//创建事件对象
	wsace = WSACreateEvent();
	if (WSA_INVALID_EVENT == wsace)
	{
		int a = WSAGetLastError();
		printf("创建事件对象 error: %d\n", a);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}
	//绑定socket与操作码，并投递
	int wsaes = WSAEventSelect(SerVerSocket, (HANDLE)wsace, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaes)
	{
		int a = WSAGetLastError();
		printf("绑定事件、socket并投递 error: %d\n", a);
		WSACloseEvent(wsace);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//等待信号然后进行分类处理
	while (1)
	{
		//等待信号（询问信号）
		DWORD wsawfmpe = WSAWaitForMultipleEvents(1, &wsace, TRUE, WSA_INFINITE, FALSE);
		//等待超时了
		if (WSA_WAIT_TIMEOUT == wsawfmpe)
		{
			printf("询问信号函数超时了\n");
			continue;
		}
		//函数出错，错误处理
		if (WSA_WAIT_FAILED == wsawfmpe)
		{
			int a = WSAGetLastError();
			printf("询问信号函数 error: %d", a);
			break;
		}

		//列举事件，获取响应事件的类型（具体触发操作），并将该信号重置
		struct _WSANETWORKEVENTS wsanet;
		int wsaene = WSAEnumNetworkEvents(SerVerSocket, wsace, &wsanet);
		if (SOCKET_ERROR == wsaene)
		{
			int a = WSAGetLastError();
			printf("询问信号函数 error: %d", a);
			break;
		}

		//事件分类处理
		//判断FD_READ
		if (wsanet.lNetworkEvents & FD_READ)
		{
			if (wsanet.iErrorCode[FD_READ_BIT] == 0)
			{
				//recvfrom接收
				struct sockaddr stclient;	//参数5，用于记录对方socket（客户端）的端口与地址
				int nlen = sizeof(stclient);//参数6，参5结构体大小
				char cbuf[548] = { 0 };		//参2，字符数组，存放接收到的信息
				int rnum = recvfrom(SerVerSocket, cbuf, 548, 0, &stclient, &nlen);
				if (SOCKET_ERROR == rnum)
				{
					int a = WSAGetLastError();
					printf("recvfrom error: %d\n", a);
				}
				printf(" %s\n", cbuf);
				//sendto发送信息
				int snum = sendto(SerVerSocket, "Server 收到\n", strlen("Server 收到\n"), 0, (const struct sockaddr*)&stclient, sizeof(stclient));
				if (SOCKET_ERROR == snum)
				{
					int a = WSAGetLastError();
					printf("recvfrom error: %d\n", a);
				}
			}
			else
			{
				printf("FD_READ_BIT error code:%d\n", wsanet.iErrorCode[FD_READ_BIT]);
				continue;
			}
		}
	
		//判断FD_WRITE
		if (wsanet.lNetworkEvents & FD_WRITE)
		{
			if (wsanet.iErrorCode[FD_WRITE_BIT] == 0)
			{
				printf("FD_WRITE已产生，可以发送了\n");
			}
			else
			{
				printf("FD_WRITE_BIT error code:%d\n", wsanet.iErrorCode[FD_WRITE_BIT]);
				continue;
			}
		}
	}

	//释放socket、关闭网络库
	WSACloseEvent(wsace);		//点×退出函数也要加上这个释放事件、句柄
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}


void studybook13(void)
{
	//4.1 事件选择模型要点、逻辑分析
	{
			//1.windows处理用户行为的两种方式
		/*
			windows处理用户行为的两种方式：
					1.消息机制，核心：消息队列
					2.事件机制，核心：事件集合
			用户行为：比如我们按下键盘上的对应按钮，或者鼠标点击了，鼠标移动了，鼠标拖动了一个什么这些都是用户行为

			消息机制：
				核心：消息队列
				特点：有序、全面
					有序：
						消息队列是由操作系统创建并维护的，我们产生行为，被操作系统监视到并将这些行为依次装到消息队列中
						队列的特点是先进先出，那么监视时先触发的行为就会优先装到队列并处理，这就是有序
					全面：
						操作系统早就预先定义好了，比如键盘，鼠标等等行为的消息，比如我们要玩一个游戏用到wasd键
						但是玩的时候按下了yuio键，这与游戏不相干，但是依然会产生消息，并存放到消息队列中
						区别是我们从队列取出消息，并处理的时候，筛选过滤了我们想要的
			事件机制：
				核心：事件集合
				特点：无序、不全面
					无序：
						事件机制中，操作系统并没有创建好什么，我们要手动绑定行为与事件，并手动创建事件集合
						事件集合要遍历来获取有响应的事件，从下标小的开始遍历，那么遍历过程中，下标小的可能是后响应的，下标大的是先响应的
						但是因为遍历的局限，只能优先处理下标小的，所以事件机制是无序的
					不全面：
						操作系统并没有预先给行为绑定事件，而是我们手动创建一个事件，并绑定一个行为投递给操作系统
						所以是不全面的，没有创建事件并绑定行为，就没有这个对应行为的事件机制
						
			消息机制：核心：消息队列
					处理过程
						所有的用户操作，比如点鼠标，摁键盘，点软件上的按钮....等等，所有操作均依次按顺序被记录，装进一个队列
					特点
						消息队列由操作系统维护，咱们做的操作，然后把消息取出来，分类处理
						有先后顺序
					其他
						我们之后教大家win32,MFC课程，这部分都是基于这个消息队列，会给大家详细介绍，大家暂时从宏观理解
						我们下个模型，异步选择就是基于这个消息的

			事件机制：核心：事件集合
					处理过程
						根据需求，我们为用户的特定操作绑定一个事件，事件由我们自己调用API创建，需要多少创建多少
						将事件投递给系统，系统就帮咱们监视着，所以不能无限创建，太多系统运行就卡了
						如果操作发生了，比如用户按鼠标了，那么对应的事件就会被置成有信号，也就是类似1变2了，用个数标记aaa
						我们直接获取到有信号的事件，然后处理
					特点
						所有事件都是咱们自己定义的，系统只是帮咱们置有无信号，所以我们自己掌管定义....
						无序的
					其他
						咱们这个事件选择，就是应用这个
		*/
			//2.事件选择的逻辑
		/*
			事件选择的逻辑
				为我们的socket绑定上一个事件对象，咱们只有一个socket,投递给操作系统，他就帮我们异步监视着，当有客户端发来了消息，该事件就会被置成有信号，那么我们获取该有信号的事件，尽心处理
				第一步
					创建一个事件对象（变量），
						WSACreateEvent
				第二步
					将事件与socket绑定到一起，指定监视类型（recvfrom，sendto）并投递给系统，
						WSAEventSelect（监视类型就是像accept、close、recv、send等）
				第三步
					循环等待事件产生信号
						WSAWaitForMultipleEvents
						该函数等待过程将所在线程挂起，释放cpu时间（就是在等待的时候，主线程挂机等待，不占用cpu时间）
						对比select：select遍历等待的时候占用了主线程的时间的，是阻塞的，而这个函数是不占用时间的
				第四步
					有信号的话就分类处理
						WSAEnumNetworkEvents

			事件选择模型流程：
					1.添加网络库，网络头文件
					2.打开网络库
					3.校验版本
					4.创建socket
					5.绑定端口号、地址
					6.事件选择
		*/
	};
	//4.2 创建事件对象并绑定投递
	{
		/*
			创建事件对象：
				WSAEVENT WSAAPI WSACreateEvent();
					成功：返回一个事件
					失败：返回WSA_INVALID_EVENT
				关闭事件方式：
					::closesocket(socketServer);
					::WSACleanup();
				事件转定义：HANDLE：句柄：ID：内核对象
					由系统在内核申请，由操作系统访问，我们不能定位其内容，也不能修改
						类型是void*：通用类型指针
						同这个指针是对内核的保护，对规则的保护，从而使操作系统有序的平稳的，有效的运行，而不会随便出问题
					调用函数创建，调用函数释放
						如果我们没有调用释放，那么他可能就一直存在于内核，造成内核内存泄漏， 这种只能重启电脑
					内核对象有：socket，Kernel Objects
				几个相关函数：
					BOOL WSAAPI WSACloseEvent( WSAEVENT hEvent);
						关闭/释放事件句柄，不用就要释放
					BOOL WSAAPI WSAResetEvent( WSAEVENT hEvent);
						重置WSAEventSelect函数使用的事件对象状态的正确方法是将事件对象的句柄传递给hEventObject参数中的WSAEnumNetworkEvents函数。 这将重置事件对象并以原子方式调整套接字上活动FD事件的状态。
					BOOL WSAAPI WSASetEvent( WSAEVENT hEvent);
						将指定事件主动置成有信号
					后两个函数不常用，但是一定要知道，用的时候就能正确使用



			绑定并投递：
				函数原型：
					int WSAAPI WSAEventSelect
					(
					  SOCKET   s,
					  WSAEVENT hEventObject,
					  long     lNetworkEvents
					);
				功能：
					给事件绑上socket与操作码，并投递给操作系统
				参数1：
					被绑定的socket，我们要的效果是每一个socket都绑定上一个事件
				参数2：
					事件对象，逻辑：就是讲参数1与参数2绑定在一起
				参数3：
					具体的事件，多个事件用按位或 | 来并列，详情看课件或者TCP的事件选择模型笔记
						FD_READ：有客户端发来消息，
						FD_WRITE：可以给客户端发信，在accept后产生该信号
						FD_ACCEPT、FD_CLOSE、FD_CONNECT（客户端使用）等UDP用不到
						0：取消事件监视，WSAEventSelect（0）；
						FD_OOB，带外数据，不多说了，一般不使用
						FD_QOS：服务质量发生变化通知，比如下载时看电影，带宽的分配的记录
						FD_GROUP_QOS：保留用，暂时没有功效
						重叠I/O模型中用到的：FD_ROUTING_ INTERFACE_CHANGE与FD_ADDRESS_ LIST_CHANGE
				返回值：
					成功：返回0
					失败：返回SOCKET_ERROR
			总结：
				创建的事件一定要手动释放，点×退出函数里也加上，具体事件的操作码UDP是面向非链接的，没有客户端socket，没有链接，accept与close用不上
				我们就用	FD_READ、FD_WRITE收发的就可以了，TCP的服务器socket是绑定accept
				
				事件选择模型中 事件是核心，所以如果创建事件、绑定事件出错，要把所有socket、事件释放并关闭网络库
		*/
	};
	//4.3 等待信号并分类处理
	{
			//等待信号（也叫询问信号）
		/*
			函数原型：
				DWORD WSAAPI WSAWaitForMultipleEvents(
					  DWORD          cEvents,
					  const WSAEVENT *lphEvents,
					  BOOL           fWaitAll,
					  DWORD          dwTimeout,
					  BOOL           fAlertable
					);
		
			作用：
				等待事件的信号产生
			参数1：
				事件的个数，一次最高处理64个（WSA_MAXIMUM_WAIT_EVENTS），可以变大，在结尾总结再讲
			参数2：
				事件列表，可以是事件集合，也可以是数组，一个或多个的区别
			参数3：
				事件的等待方式：
					TRUE:所有处理的事件都产生信号才返回
					FALSE：任意一个事件产生信号，就立即返回，并且返回值是一个数，这个数 减去 WSA_WAIT_EVENT_0 == 数组中事件的下标
			参数4：
				超时间隔：与select参数5类似，
				123：等待123毫秒，超时返回WSA_WAIT_TIMEOUT
				0：立即返回，不论有无信号
				WSA_INFINITE：一直等待，直到有事件发生
			参数5：
				TRUE：重叠IO专门使用的
				FALSE：除重叠IO外，都填这个

			返回值
				成功：
					数组下标的运算值：
							参数3为true
								所有的事件均有信号
							参数3为false
								返回值减去WSA_WAIT_EVENT_0==数组中事件的下标
					WSA_WAIT_IO_COMPLETION：
							参数5为TRUE，才会返回这个值
					WSA_WAIT_TIMEOUT：
							超时了，continue即可
				失败：
					WSA_WAIT_FAILED


		*/
			//列举事件
		/*
			函数原型：
					int WSAAPI WSAEnumNetworkEvents(
						  SOCKET             s,
						  WSAEVENT           hEventObject,
						  LPWSANETWORKEVENTS lpNetworkEvents
						);
			作用：
				获取事件类型，并将事件上的信号重置（有重置信号功能的函数还有WSAResertEvent）
			参数1：
				对应的socket
			参数2：
				对应的事件
			参数3：
				触发的事件类型，由函数获取到，然后装到这个参数里
				是一个结构体指针：
						struct _WSANETWORKEVENTS{long lNetworkEvents;int  iErrorCode[FD_MAX_EVENTS];} 
						成员1：具体操作，一个信号可能包含两个消息，以按位或的形式存在
						成员2：错误码数组，FD_ACCEPT事件错误码在数组的值为FD_ACCEPT_BIT的下标里，为0则没有错误
			返回值：
				成功：返回0
				失败：返回SOCKET_ERROR
		
		
		*/
			//事件分类处理
		/*
			分类处理逻辑：
				可查看TCP的笔记，就是老师用多个if，书上用if else ，不能用switch
				用于解决当两个操作码同时产生并且按位或的情况

			关于事件信号在UDP的情况：
				TCP中，一有连接请求马上产生accept的信号，连接后立马产生write的信号
				UDP面向非链接，没有accept连接，那么write的产生，则是服务器一启动的时候就立马产生

			本课时总结：
				1）等待信号函数一次性最大处理64个，为了突破这个上限，有三种方式：
					1.一个一个的处理，在一个循环中，像遍历一般，从数组第1个到最后一个依次遍历处理，就能处理很多很多，且响应处理更公平了一点
					2.一组一组的处理，一次最大64，那就分多次，多次调用这个函数来解决，就是有点麻烦
					3.采用多线程，每个线程中给一组（64个）去处理
				2）有序处理
					1.TCP/IP中由于socket很多，我们要进行相关的有序处理
					2.UDP/IP就一个socket，就不用考虑顺序问题了
				3）增加事件数量
					1.TCP/IP中由于socket很多，每个socket要绑定一个事件，所以我们要多事件数量很多的情况进行逻辑处理
					2.UDP/IP就一个，没有事件可言
		*/
	};
}