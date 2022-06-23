#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <ws2tcpip.h>
#include <Mswsock.h>		//AcceptEx的头文件	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <tchar.h>
#include <Winsock2.h>				//要在win32窗口的项目中，记得修改
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

//创建socket数组、重叠IO结构体数组
#define MAX_COUNT 10240
SOCKET g_sockall[MAX_COUNT];	//socket数组
OVERLAPPED g_olpall[MAX_COUNT];	//重叠IO结构体数组
int g_count = 0;				//计数

//PostAccept投递函数的配套逻辑封装
int PostAccept();
//WSArecv函数的配套逻辑封装
int PostRecv(int Index);
//WSASend函数的配套逻辑封装
int PostSend(int Index);
//WSArecv的参数2要使用的,recv读到的数据会存到这里面
#define MAX_RECV_COUNT 1024
char g_recv[MAX_RECV_COUNT];		//全局变量会自动初始化成0
//WSASend的参数2要使用的,send写的的数据会存到这里面
#define MAX_SEND_COUNT 1024
char g_send[MAX_SEND_COUNT];		//全局变量会自动初始化成0

//清理数组函数
void Clear();

int main(void)
{
	//打开网络库
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA WSsockMsg;
	int nrse = WSAStartup(WSVersion, &WSsockMsg);
	if (0 != nrse)
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
	//校验版本
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//创建socket
	SOCKET SerVerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定端口与地址
	struct sockaddr_in st;
	st.sin_family = AF_INET;
	st.sin_port = htons(22258);
	st.sin_addr.S_un.S_un_b.s_b1 = 127;
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	if (SOCKET_ERROR == bind(SerVerSocket, (const struct sockaddr*)&st, sizeof(st)))
	{
		int a = WSAGetLastError();
		closesocket(SerVerSocket);
		WSACleanup();
		return 0;
	}
	//开启监听
	if (SOCKET_ERROR == listen(SerVerSocket, SOMAXCONN))
	{
		int a = WSAGetLastError();
		closesocket(SerVerSocket);
		WSACleanup();
		return 0;
	}

	//重叠IO开始了
	//初始化我们的socket、结构体数组
	g_sockall[g_count] = SerVerSocket;
	g_olpall[g_count].hEvent = WSACreateEvent();	//一定要手动绑定事件，系统不会帮我们做这一步
	g_count++;
	//投递AcceptEx（使用我们自定义的函数来投递）
	int Aer = PostAccept();
	if (0 != Aer)
	{
		Clear();
		WSACleanup();
		return 0;
	}
	//没有立即完成，则进入循环等待信号
	while (1)
	{
		for (int n = 0; n < g_count; n++)
		{
			//询问信号
			DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[n].hEvent, FALSE, 0, FALSE);
			if (wfmte == WSA_WAIT_FAILED || wfmte == WSA_WAIT_TIMEOUT)
			{	//出错了  和  延迟处理
				continue;	//没等到，下一轮循环继续等
			}
			//等到了信号，获取socket具体情况
			DWORD dwState;	//参数3
			DWORD lpflags;	//参数5
			BOOL wolr = WSAGetOverlappedResult(g_sockall[n], &g_olpall[n], &dwState, TRUE, &lpflags);
			if (FALSE == wolr)
			{
				int a = WSAGetLastError();
				//强制下线，错误码为10054
				if (10054 == a)
				{
					printf("客户端%d强制close了\n", g_sockall[n]);
					//删掉下线客户端的socket与事件
					closesocket(g_sockall[n]);
					WSACloseEvent(g_olpall[g_count - 1].hEvent);
					//用末尾元素填充到删除的客户端的位置
					g_sockall[n] = g_sockall[g_count - 1];
					g_olpall[n] = g_olpall[g_count - 1];
					//循环控制变量
					n--;		//因为末尾元素到了当前的元素位置，所以我们要n-1 ,否则就跳过当前赋值过来的socket与事件了
					g_count--;	//计数-1
				}
				continue;
			}
			//重置信号（老师忘了加，出了BUG）
			BOOL ChongZhi = WSAResetEvent(g_olpall[n].hEvent);
			if (FALSE == ChongZhi)
			{
				int a = WSAGetLastError();
				continue;
			}
			//分类处理
			//n == 0 时有信号，0是服务器socket，那说明有连接请求了要accept
			if (0 == n)
			{
				//接受连接完成了（因为投递给操作系统，操作系统完成了）
				printf("accept 客户端%d连接成功 \n", g_sockall[g_count]);
				//向系统投递recv，因为有了新连接
				PostRecv(g_count);		//使用g_count，不要用n，因为n是当前的服务器，而g_count才是新客户端的
				//根据情况投递send
				PostSend(g_count);
				//客户端数量+1
				g_count++;
				//重新投递accept，因为进到这里就说明在之前的accept已经完成
				PostAccept();
				//重新循环，因为n为0的时候，还会向下执行，就会把客户端给关闭掉
				continue;
			}
			//客户端下线（这个是正常下线，强制下线在错误处理里）
			if (0 == dwState)
			{
				printf("客户端%d正常close了\n", g_sockall[n]);
				//删掉下线客户端的socket与事件
				closesocket(g_sockall[n]);
				WSACloseEvent(g_olpall[g_count - 1].hEvent);
				//用末尾元素填充到删除的客户端的位置
				g_sockall[n] = g_sockall[g_count - 1];
				g_olpall[n] = g_olpall[g_count - 1];
				//循环控制变量
				n--;		//因为末尾元素到了当前的元素位置，所以我们要n-1 ,否则就跳过当前赋值过来的socket与事件了
				g_count--;	//计数-1
			}
			//发送/接收成功了
			if (0 != dwState)
			{
				//判断recv是否存到了东西，存到了则说明recv成功，开始处理recv
				if (0 != g_recv[0])
				{
					//打印信息
					printf("recv: %s\n", g_recv);
					//将全局变量str清零
					memset(g_recv, 0, MAX_RECV_COUNT);
					//根据情况投递send
					//调用自己，继续传递WSARecv，等待有信息发来
					PostRecv(n);
				}
				//如果不是recv，则发送消息
				else
				{
					////根据情况投递send
					//PostSend(n);
				}
			}
		}
	}
	//清理网络库与数组(socket 和 event(事件))
	Clear();
	WSACleanup();

	system("pause>0");
	return 0;
}

//PostAccept投递函数的配套逻辑封装
//正常则返回0，否则返回错误码
int PostAccept()
{
	//参数2，客户端socket，需要手动创建
	g_sockall[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == g_sockall[g_count])
	{
		int a = WSAGetLastError();
		return 0;
	}
	g_olpall[g_count].hEvent = WSACreateEvent();	//此时不让下标+1，因为还没确认客户端连接成功，并且要接收了消息才+1
	//参数3，不能填NULL，参数4填0可取消参数3功能
	char str[1024];
	//参数7，可以填NULL，也可以填DWORD
	DWORD DwRecvCount;
	//t投递服务器socket，异步的接收链接
	BOOL acptex = AcceptEx(g_sockall[0], g_sockall[g_count], &str, 0, sizeof(struct sockaddr_in) + 16, sizeof(struct sockaddr_in) + 16, &DwRecvCount, &g_olpall[0]);
	//投递后立刻完成了
	if (acptex == TRUE)
	{
		//投递WSArecv，接收客户端消息
		PostRecv(g_count);
		//根据情况投递send
		//客户端数量++
		g_count++;		//只有确定了客户端连接成功并且接收了消息才让计数下标有效（老师的方法,老师的Recv函数要传客户端的下标，所以现在才+1）
		//投递AcceptEx（要继续连接客户端就必须重新投递）
		PostAccept();	//递归的方式，可改用非递归（循环，立刻完成则continue继续循环，延迟处理或出错则break）
		return 0;
	}
	//投递后没有立即完成，也可能出错了
	else
	{
		int a = WSAGetLastError();
		//暂时没有客户端连接,进行延迟处理
		if (a == ERROR_IO_PENDING)
		{
			return 0;
		}
		//出错了
		else
		{
			return a;	//出错则返回错误码
		}
	}
}
//WSArecv函数的配套逻辑封装
int PostRecv(int Index)
{
	//参数2
	struct _WSABUF buff;
	buff.buf = g_recv;			//str已经在全局区定义了
	buff.len = sizeof(g_recv);

	DWORD count;				  //参数4
	DWORD DwFlag = MSG_PARTIAL;   //参数5
	int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], NULL);
	//立即发生
	if (0 == nrecv)
	{
		//打印信息
		printf("recv: %s", buff.buf);
		//将全局变量str清零
		memset(g_recv, 0, MAX_RECV_COUNT);
		//根据情况投递send
		//调用自己，继续传递WSARecv，等待有信息发来
		PostRecv(Index);
		return 0;
	}
	//延迟处理或者出错了
	else
	{
		int a = WSAGetLastError();
		if (WSA_IO_PENDING == a)
		{
			return 0;
		}
		else
		{
			return a;
		}
	}
}
//WSASend函数的配套逻辑封装
int PostSend(int Index)
{
	//参数2
	struct _WSABUF buff;
	buff.buf = g_send;			//str已经在全局区定义了
	buff.len = sizeof(g_send);
	//输入要发送内容
	int s = scanf("%s", buff.buf);

	DWORD Sendcount;					//参数4
	DWORD SeDwFlag = MSG_PARTIAL;		//参数5
	int nSend = WSASend(g_sockall[Index], &buff, 1, &Sendcount, SeDwFlag, &g_olpall[Index], NULL);
	//立即发生
	if (0 == nSend)
	{
		//打印信息
		printf("已向客户端发送\n");
		//将全局变量str清零
		memset(g_send, 0, MAX_RECV_COUNT);
		//不用继续投send了，需要的时候再send
		return 0;
	}
	//延迟处理或者出错了
	else
	{
		int a = WSAGetLastError();
		//延迟处理
		if (WSA_IO_PENDING == a)
		{
			//打印信息
			printf("已向客户端发送\n");
			//将全局变量str清零
			memset(g_send, 0, MAX_RECV_COUNT);
			//不用继续投send了，需要的时候再send
			return 0;
		}
		else
		{
			return a;
		}
	}
}
//清理数组
void Clear()
{
	for (int i = 0; i < 20; i++)
	{
		closesocket( g_sockall[i]);	//socket数组
		WSACloseEvent(g_olpall[i].hEvent);	//重叠IO结构体数组
	}
	g_count = 0;
}

//学习笔记
void studybook()
{
	//老师说这个代码有点难（不是说简单的吗？！可能是逻辑上简单）
	//总之我就遇到很多问题，耽误了很多时间
	//老师说网上的代码比他的复杂，因为做了很多优化，这里只是写了最基本的功能，方便我们学习，还有一些BUG

	//6.1 重叠IO模型知识简介
	{
/*
			* 重叠IO模型意义：
			*		重叠IO是windows提供的一种异步读写文件的机制
			* 作用：
			*		socket的本质就是文件操作
			*		正常读写文件，如recv、send执行时是阻塞的，不能执行别的recv、send
			*		recv要等待协议缓冲区的内容全部复制到我们的buf（自定义数组）中，函数才能结束，并返回复制的个数
			*		send也是一样的，执行send的过程中，别的读写操作都不能执行
			*
			*		重叠IO：将读（recv）的指令和我们的buf（自定义数组）投递给操作系统，然后函数直接返回
			*		操作系统会独立开一个线程，将数据复制到我们的buf中，这个复制过程是操作系统执行的，与我们的程序无关
			*		这样的话，我们就能在操作系统复制过程中做其他事，两个不会耽误，就是让 读写 变成异步，能同时投递多个读写操作
			*
			*		就是将：accept、recv、send优化成了异步操作，被函数AcceptEx WSARecv WSASend代替了
			*		这个IO模型就是对C/S基本模型的直接优化
			*
			* 本质：
			*		定义一个结构体的变量，让变量与socket绑定
			*
			*		结构体：WSAOVERLAPPED
			*			成员：前四个成员1、2、3、4是操作系统来使用的，与我们无关，是服务提供商的开发人员（函数设计者）来保留使用的
			*			最后一个成员5：事件对象，是我们要关注的，操作完成后它就会被置成有信号
			*
			* 使用：
			*		异步选择模型：就是把socket与消息绑定在一起，操作系统以消息机制来处理并反馈
			*		事件选择模型：就是把socket与事件绑定在一起，操作系统以事件机制来处理并反馈
			*		重叠IO模型  ：就是把socket与重叠结构（结构体）绑定在一起，操作系统以重叠IO机制来处理并反馈
			*	三种都是socket绑定后投递给操作系统
			*
			* 重叠IO的反馈两种方式：
			*		1：事件通知
			*		2：完成例程，回调函数
			*
			* 重叠IO的基本逻辑：	两种方式，可以单独使用，也可以配合使用
			*		事件通知：
			*				调用AcceptEx WSARecv WSASend投递
			*				被完成的操作，被置成有信号的
			*				获取信号，调用WSAWaitForMultipleEvents获取事件信号
			*		回调函数/完成例程：	（回调函数也叫完成例程）
			*				调用AcceptEx WSARecv WSASend投递
			*				完成操作，被置成有信号后就会自动调用回调函数
			*
			* 性能：
			*		网上有人测试：用一台6000元左右电脑，20000个客户端同时请求连接，发送信息，
			*		使用重叠IO模型，系统的CPU使用率上升40%左右，
			*		使用完成端口模型，系统的CPU使用率上升2%左右
			*		所以完成端口模型才是真正的完善的，很好用的模型，最好用的模型
			*	其它的模型也能完成这么多客户端，但是因为同步（阻塞），延迟很高
			*
			* 补充：
			*		重叠IO是将每个执行阻塞的操作（recv等）都绑定一个线程，但问题是CPU线程不是无限的
			*		比如有一个阈值5000 ，线程低于5000时，效率是会提升的，但是大于5000后，效率就会变慢
			*		因为线程也是需要切换等操作的
			*
			*		成千上万的客户端，看来也就重叠IO和完成端口能比其它模型好得多了
			*		但重叠IO占用CPU使用率有点高
			*
			*		回调函数也叫完成例程
			*		重叠IO的基本逻辑：种方式，可以单独使用，也可以配合使用 （事件通知和回调函数）
			*/
	}
	//6.2 事件通知实现逻辑
	{
/*
			*		C3说这节课先了解大体逻辑，代码上手后能理解的更快
			*		通过文字不太好理解，其实很简单的
			*
			* 创建数组：
			*		创建事件数组、socket数组、重叠结构体数组
			*		下标相同的绑定到一起，  本课就不加事件数组了
			*		网上很多教程是创建了事件数组的，后面会讲要不要的区别
			*
			* 创建重叠IO模型专门使用的socket：
			*		用WSASocket，专门用于异步操作的SOCKET
			*
			* 整体逻辑：
			*		1.投递AcceptEx
			*		2.循环等待信号
			*
			*	投递AcceptEx：
			*			立即完成：
			*					立即完成的话，就马上对客户端socket投递WSARecv，这个投递后也有立即完成与延迟完成
			*					然后根据需求是否投递WSASend，这个投递后也有立即完成与延迟完成
			*					最后要重新对服务器socket投递AcceptEx	（因为之前投递的AcceptEx已经完成了，还要继续使用就必须重新投递）
			*			延迟完成
			*					就是投递后没有立即完成，那操作系统会开辟一个线程出来，用这个线程来帮你等待AcceptEx的信号完成
			*					这样的话，等待信号完成交给操作系统的线程，我们呢就到循环中等待信号完成了然后接收处理
			*	循环等待信号：
			*		之前投递的AcceptEx没有立即完成，交给操作系统去监管后，我们回到循环中等待信号完成
			*			等信号
			*					用WSAWaitForMultipleEvents函数来接收等待信号
			*			有信号
			*					收到信号后，获取重叠及结构上的信息，用WSAGetOverlappedResult函数
			*					然后开始分类处理，这个信号是什么，怎么处理：
			*						如果是客户端退出，我们就删除对应客户端信息
			*						如果是接受连接，我们就投递AcceptEx（投递后的逻辑还是我们的整体逻辑）
			*						如果是接收消息，处理消息后，继续对客户端socket投递WSARecv（还是整体逻辑）
			*						如果是发送消息，根据需求对客户端socket投递WSASend
			*
			*
			*/
	}	
	//6.3 WSASocket函数详细介绍
	{
		/*
			* 补充点：
			*		我们不需要手动绑定socket与重叠IO结构体，当我们调用函数时，函数会自动将传递的参数进行绑定
			*		我们不要光是学习函数、模型的使用，而是积累这样的好处，它的优秀点是什么
			*		那以后我们工作就能很好的仿照这种思想去编程
			*
			* 基本流程：
			*		1.打开网络库
			*		2.校验版本
			*		3.创建socket（这一步与C/S不一样），要用WSAsocket创建
			*		4.绑定端口号与地址
			*		5.开启监听
			*		6.重叠IO开始了
			*
			* 创建事件数组、重叠IO的结构体数组：
			*		可设置成上万个元素上限
			*
			* 创建异步操作的SOCKET：	WSAsocket
			*		作用：
			*			创建一个用于异步操作的SOCKET
			*			WSASocket： windows专用，WSA的函数都是windows专用的
			*			WSA: Windows  socet  async  都是用于支持异步操作的函数
			*
			*	参数1、2、3：
			*			与socket函数的参数是一样的：地址类型、套接字类型、协议类型
			*			参3可以填成0，让系统自己判断用哪种协议，但是有风险，因为可能多个协议有相同的前两参数类型
			*			这样系统判断的，和我们想要的协议就不一样了
			*	参数4：
			*			本质是：指向WSAPROTOCOL_INFO结构的指针
			*			设置套接字详细的属性：
			*				比如：发送数据是否需要链接
			*					是否保证数据完整到达（即传输丢包是否被允许）
			*					如果参数3填了0，那么可以在这里指定为哪个协议
			*					设置传输接收字节数
			*					设置套接字权限
			*					还有许多保留字段，供拓展使用
			*			我们先知道有这个操作，以后有用到就能知道怎么去学习使用了
			*			不使用的话填 NULL
			*	参数5：
			*			填0即可，保留的参数，暂未使用，供拓展,
			*			老师测试是一组socket的组ID，大概是想同时操作多个socket
			*	参数6：
			*			指定套接字的属性。
			*			填写：WSA_FLAG_OVERLAPPED ： 创建一个供重叠IO模型使用的socket
			*			其它的：
			*				用于多播协议：WSA_FLAG_MULTIPOINT_C_ROOT、WSA_FLAG_MULTIPOINT_C_LEAF、 WSA_FLAG_MULTIPOINT_D_ROOT、 WSA_FLAG_MULTIPOINT_D_LEAF
			*				配合参数4，设置套接字操作权限：WSA_FLAG_ACCESS_SYSTEM_SECURITY，，可以对套接字send设置权限，这样在send的时候就会触发相关权限设置，会提醒
			*				套接字不可被继承，用于多线程开发：WSA_FLAG_NO_HANDLE_INHERIT
			*					多线程开发中，子线程会继承父线程desocket，就像主函数变量在局部能使用。
			*					子线程使用父线程socket的两种方式：
			*						1.直接用父线程的socket，即共享
			*						2.子线程将父线程的socket复制一份，自己用，这两个socket内容完全一样，即继承
			* 返回值：
			*			成功：则返回可用的socket，不用了就需要销毁套接字，用closesocket
			*			失败：返回INVALID_SOCKET，进行失败处理要关闭网络库
			*
			*/

	}	
	//6.4 AcceptEx函数使用详解
	{
/*
			* 初始化socket、结构体数组：
			*		重叠IO结构体数组的成员5是我们能用的（其它的是函数开发人员保留的）
			*		我们一定要给成员5手动初始化一个事件，即用函数WSACreateEvent创建一个事件返回给他
			*		因为系统不会创建事件，那我们就无法确定它的内容，也就无法使用
			* 学习方法：
			*		一些函数的参数很多，不要害怕，MSDN上有详细介绍
			*		慢慢来一个个理解，总能学会的
			* 本课操作：
			*		初始化了socket、结构体数组
			*		写了自定义函数，PostAccept投递函数的配套逻辑封装
			*		注意看if else的结构，正常返回0，错误返回错误码
			* 函数原型：
			* BOOL AcceptEx(
			*				  SOCKET       sListenSocket,
			*				  SOCKET       sAcceptSocket,
			*				  PVOID        lpOutputBuffer,
			*				  DWORD        dwReceiveDataLength,
			*				  DWORD        dwLocalAddressLength,
			*				  DWORD        dwRemoteAddressLength,
			*				  LPDWORD      lpdwBytesReceived,
			*				  LPOVERLAPPED lpOverlapped
			*				);
			*
			* AcceptEx函数：
			*	功能：	投递服务器socket，异步的接收链接（就是我们程序不用管它，用操作系统的线程去处理链接的过程，我们可以干别的事）
			*
			*	参数1：
			*			服务器的socket
			*	参数2：
			*			要链接到服务器的 客户端的socket
			*				注意：这个socket需要我们手动创建，用socket或WSASocket都可以，但WSASocket更符合逻辑
			*				我们不创建的话，系统是不会创建的，需要手动创建，再传给参数2
			*				系统在开辟线程进行连接时，就会自动将这个socket与客户端的端口号、IP地址绑定在一起，相当于自动调用了bind函数
			*	参数3：
			*			缓冲区指针，接收在新连接上发送的第一组数据
			*			char str[1024]：是一个字符串数组类型。我们手动创建数组然后传到参数3即可。
			*			不能设置成NULL，下面可以通过参数4来取消这个功能。
			*				意义讲解：就是如果我们启用这个参数3的功能，那么客户端连接到服务器后，第一次send发送的消息，不是由WSARecv来接受
			*						 而是由这个参数3来接收；第二次以后，就是由WSARecv来接收了
			*						 所以这个参数3是一个用于接收客户端第一次发送的数据的数组，是一个字符串数组，也可以叫数组的指针（地址）
			*	参数4：
			*			0  设置成0：取消参数3的功能
			*			1024， 设置成参数3的数组元素最大值，那么就会开启参数3的功能
			*					此时，客户端链接并发送了一条消息，服务器才产生信号
			*					所以会有阻塞状况，光链接不行，必须等客户端发来一条消息，他才完事儿，有信号
			*			所以设置成0就好了，参数3、4的功能不常用，一会这个接收，那个接收的，逻辑就复杂了
			*	参数5：
			*			sizeof(struct sockaddr_in) + 16
			*			本地地址信息 的保留的字节数，此值必须比使用的传输协议的最大地址长度要长16个字节
			*				意义讲解：我们得到本地地址信息，这个地址信息是在内存中处理的
			*					但是处理完了以后，要把它保存到文件位置（可能相当于备份暂存一样）
			*					这就是保留到文件位置的预留的大小
			*	参数6：
			*			sizeof(struct sockaddr_in) + 16
			*			远程地址信息 的保留的字节数，此值必须比使用的传输协议的最大地址长度要长16个字节
			*				意义讲解：同参数5一样，但这是远程地址的信息
			*	参数7：
			*			可以填写 NULL、也可以填写DWORD变量的地址
			*			是配合参数3、4来使用的，就是如果是在刚好客户端连接到服务器的时候，客户端发送了一个消息
			*			那参数3就会马上接收到这个消息，并且参数7装着参数3接收到的字符数
			*				讲解：就是只能在刚好连接上的时候，即整体逻辑里的立马完成，同步的时候发送了消息，参数7才会有用
			*					 如果是异步完成，就是投递此函数没有立刻完成，且客户端发送消息，那这个参数就没用，没有内容
			*	参数8：
			*			我们的重叠结构体
			*	返回值：
			*			TRUE:立即完成返回，刚执行，就有客户端就连接了
			*			FALSE：出错了，用int a = WSAGetLastError()来获得错误码
			*				如果a == ERROR_IO_PENDING，则是异步等待，暂时没有客户端连接
			*				其它的错误码：根据具体错误码解决
			*/
	}
	//6.5 WSARecv函数详解
	{
/*
			* 抱怨： 我上节课手打了一遍AcceptEx函数参数，费了很多时间，脑子也抑郁了，也没记住多少
			*		这节课的WSARecv函数参数的详解我就复制粘贴过来吧
			*
			* 总结：
			*		函数原型很重要，比如某个参数，我们不能光靠记忆去记住，
			*		函数原型会告诉我们这个参数的类型啊，函数返回值的类型啊，还能提高我们学习新函数的能力
			*		参数的类型带LP的都是指针类型（地址类型）
			*		递归的方式，可改用非递归（循环，立刻完成则continue继续循环，延迟处理或出错则break）
			*		只有确定了客户端连接成功并且接收了消息才让计数下标有效（老师的方法,老师的Recv函数要传客户端的下标，所以现在才+1）
			*
			* WSARecv函数原型：
			*		int WSAAPI WSARecv(
			*							SOCKET                              s,
			*							LPWSABUF							lpBuffers,
			*							DWORD                               dwBufferCount,
			*							LPDWORD								lpNumberOfBytesRecvd,
			*							LPDWORD								lpFlags,
			*							LPWSAOVERLAPPED						lpOverlapped,
			*							LPWSAOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine);
			* 作用：
			*		异步接收信息
			* 参数1：
			*		客户端socket
			* 参数2：
			*		接收后的信息存储buffer（一个结构体，成员分别为：字符数组，字符数组的字节数（这俩往往一块出现的，有字符数组，就有它的大小））
			*		struct _WSABUF {ULONG len; CHAR  *buf;} 成员1：字节数、成员2：指向字符数组的指针
			* 参数3：
			*		参数2的结构体WSABUF 的的个数
			*		填1即可
			* 参数4：
			*		接收成功的话，这里装着成功接收到的字节数
			*		参数6重叠结构不为NULL的时候，此参数可以设置成NULL
			* 参数5：
			*		指向用于修改WSARecv函数调用行为的标志的指针（就像recv函数的参数5，指定协议缓冲区的读取方式，比如有读了不删除，读了就删等）
			*		MSG_PEEK：协议缓冲区信息复制出来，不删除，跟recv参数5一样
			*		MSG_OOB：带外数据（很冷门）
			*		MSG_PUSH_IMMEDIATE：通知传送尽快完成
			*					比如传输10M数据，一次只能传1M，这个包要拆成10分发送，第一份发送中，后面9份要排队等着，指定了这个标记，
			*					那么指示了快点儿，别等了，那么没被发送的就被舍弃了，从而造成了数据发送缺失
			*				该参数不建议用于多数据的发送：聊天的那种没问题，发个文件什么的就不建议了
			*				提示系统尽快处理，所以能减少一定的延迟
			*		MSG_WAITALL：呼叫者提供的缓冲区已满。连接已关闭。请求已取消或发生错误。
			*		MSG_PARTIAL：传出的（上面的都是传入的），表示咱们此次接收到的数据是客户端发来的一部分，接下来接收下一部分（老师说这个很实用）
			* 参数6：
			*		重叠结构
			* 参数7：
			*		回调函数：完成例程形式使用
			*		事件形式可以不使用，不使用就设置NULL
			*
			* 返回值：
			*		0 ：立即发生，计算机比较闲，立刻就发出去了
			*		SOCKET_ERROR：获取到错误码
			*			如果错误码是：WSA_IO_PENDING，表示重叠结构成功了，但不是立即完成，而是延迟处理
			*			其它错误码则表示函数运行出错了
			*
			*
			*/
		}
	//6.6 获取异步处理的具体事件
	{
/*
		*		上两节课是讲立即完成，这节课是讲延迟等待的
		* 
		* 清理网络库与数组:
		*		循环清理，请看代码
		* 
		* 循环等待信号：
		*		每次询问信号函数只处理一个（它最多同时64个），然后用for循环依次遍历整个我们的socket数组，就突破了64的限制
		*		这样也是相对的公平了，因为询问信号函数，返回有响应的下标最小的事件，让其每次询问一个然后遍历，就不会出现不公平的现象
		*		但也只是相对公平
		*	
		*		n == 0 时有信号，0是服务器socket，那说明有连接请求了要accept
		*
		*		WSAGetOverlappedResult函数的参数3，会将发送或者接收到的实际字节数传递到在这个变量中，返回0则说明客户端下线了
		*		且它的参数5，就是WSARecv的参数5
		*		
		* 
		* 获取socket信号的具体情况：
		*	函数原型：
		*			BOOL WSAAPI WSAGetOverlappedResult(
		*					  SOCKET          s,
		*					  LPWSAOVERLAPPED lpOverlapped,
		*					  LPDWORD         lpcbTransfer,
		*					  BOOL            fWait,
		*					  LPDWORD         lpdwFlags );
		* 功能
		*		获取对应socket上的具体情况
		* 参数1
		*		有信号的socket
		* 参数2
		*		对应的重叠结构
		* 参数3
		*		DWORD类型，函数会将发送或者接收到的实际字节数传递到在这个变量中	
		*		如果是0 表示客户端下线		
		* 参数4
		*		仅当重叠操作选择了基于事件的完成通知时，才能将fWait参数设置为TRUE。
		*		填TRUE，选择事件通知	我们这个模型就是事件通知
		* 参数5
		*		1、装WSARecv的参数5 lpflags
		*		2、不能是NULL
		* 返回值
		*		true执行成功
		*		false执行失败
		*			
		*/
		}	
	//6.7 重叠IO模型事件分类处理
	{
			/*
		* //模型纠错
		*	AcceptEx的末尾参数，是重叠IO的结构，要与我们触发信号的socket下标一样，就是0
		*	客户端强制下线会有错误码10054，正常下线不会有错误码，所以下线处理要分两部分来完成
		*	信号需要进行重置，否则就一直是那个信号了，用WSACL
		*	我之前没解决的问题：无法解析的外部符号 _AcceptEx@32 是因为没加库，我只加了头文件，（下次看MSDN要注意头文件和库了）
		*	在进行分类判断执行接收连接操作的时候，结尾要加continue重新循环，因为n为0的时候，还会向下执行,
		*	此时得到信号类型的参数3dwState为0（因为是服务器socket），就会把客户端给关闭掉
		* 
		*	其他的看代码就好了，这节课的代码就是把while循环里的分类处理给写完了
		*	很多注释都在循环里，我出了一个错就是上面纠错笔记的最后两条，我测了半天都不知道错在哪里
		*	因为我对流程根本不熟悉，要加强一下理解的代码了
		*/
		}	
	//6.8 WSASend函数使用
	{
		/*
		* 上节课问题总结：
		*		1、accept 
		*				参数有误、位置有误。AcceptEx最后参数的下标要与参数1的下标一致
		*		2、recv
		*				参数有误
		*		3、close 
		*				检测有误
		*				强制退出用错误码10054检测
		*				优雅退出用参数3==0检测
		*		4、信号置空
		*				WSAResetEvent(g_allOlp[i].hEvent);
		* 
		* 这节课内容：
		*		写了WSASend的发送函数，由于不能一直一直发送，所以WSASend往往不用自己调用自己，在需要的时候调用就好了
		*		如果要用scanf输入，请不要在执行WSASend后输入，要在WSASend函数前就输入，请看PostSend代码
		*		接收端也需要添加recv函数才能接到
		*	本重叠IO：事件模型存在的问题：
		*		在一次处理过程中，客户端产生多次send，服务器会产生多次接收消息，第一次接收消息会收完所有信息
		* 
		* WSASend函数：
		*			除了参数5不是指针类型，其它的与WSARecv一样
		*		函数原型：
		*				int WSAAPI WSASend(
		*					  SOCKET                             s,
		*					  LPWSABUF                           lpBuffers,
		*					  DWORD                              dwBufferCount,
		*					  LPDWORD                            lpNumberOfBytesSent,
		*					  DWORD                              dwFlags,
		*					  LPWSAOVERLAPPED                    lpOverlapped,
		*					  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine  );
		*		作用
		*			投递异步发送消息
		*		参数1
		*			客户端socket
		*		参数2
		*			接收后的信息存储buffer（结构体，与recv的一样）
		*						成员1
		*							字节数
		*						成员2
		*							指向字符数组的指针
		*		参数3
		*			参数2是个WSABUF 的的个数
		*			1个即可
		*		参数4
		*			接收成功的话，这里装着成发送的字节数
		*			参数6重叠结构不为NULL的时候，此参数可以设置成NULL
		*		参数5
		*			函数调用行为的标志的
		*		参数6
		*			重叠结构
		*		参数7
		*			回调函数
		*				完成例程形式使用
		*					事件形式不使用
		*					设置NULL
		*/
		}
		
	
}