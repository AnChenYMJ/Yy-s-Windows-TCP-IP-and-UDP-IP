#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS				//scanf报错 ，加上这个宏就不报错了
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

//创建重叠结构体、套接字数组
#define MAX_COUNT 10240
SOCKET g_sockAll[MAX_COUNT];
OVERLAPPED g_olpAll[MAX_COUNT];
int g_count;
//完成端口全局变量，方便释放调用
HANDLE duankou;
//系统核心数，全局声明方便调用
int nProcessorsCount;
//创建多线程
HANDLE* pThread;
//创建字符数组
#define MAX_RECV_COUNT 1024
char g_str1[MAX_RECV_COUNT];	//给recv用
#define MAX_SEND_COUNT 1024
char g_str2[MAX_SEND_COUNT];	//给send用
//封装投递函数
int PostRecv(int Index);
int PostSend(int Index);
int PostAccept(void);
//创建多线程函数参数3，线程中具体要执行的内容，此函数的参数由创建函数参数4传递
DWORD WINAPI ThreadProc(LPVOID lpParameter);
//清理数组
void Clear(void);
//线程函数的循环控制变量
BOOL g_flag = TRUE;

//控制台函数（老师好像没讲）
BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//当关闭时，进行释放
	case CTRL_CLOSE_EVENT:	
		g_flag = FALSE;		//关闭线程的循环
		Clear();			//清除数组等
		free(pThread);		//释放申请的空间
		WSACleanup();		//关闭网络库
		//TerminateThread();//强杀线程，不知道参数是什么也不知道怎么用
		break;
	}
	return TRUE;
}

int main(void)
{
	//打开网络库
	WORD wsvresion = MAKEWORD(2, 2);
	WSADATA wsgetverMsg;
	int nres = WSAStartup(wsvresion, &wsgetverMsg);
	if (0 != nres)
	{
		//switch用参数依次与case比较，相等则执行对应case的代码，（没遇到break的话）然后继续比对，都没有的话则执行default的代码（有default的话）
		switch (nres)
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
	if (2 != HIBYTE(wsgetverMsg.wVersion) || 2 != LOBYTE(wsgetverMsg.wVersion))
	{
		WSACleanup();
		return 0;
	}
	//创建SOCKET,专用于重叠IO的socket
	SOCKET SerVerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == SerVerSocket)
	{
		WSACleanup();
		return 0;
	}
	//绑定端口号与地址
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
		return;
	}
	//初始化数组
	g_sockAll[g_count] = SerVerSocket;
	g_olpAll[g_count].hEvent = WSACreateEvent();
	g_count++;
	//创建一个完成端口
	duankou = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (0 == duankou)
	{
		int a = WSAGetLastError();
		printf("创建端口错误码：%d", a);
		Clear();
		WSACleanup();
		return;
	}
	//完成端口与socket绑定	//这里的参数3是下标的含义,此处的0，是服务器的下标，与创建完成端口的0不一样
	HANDLE tegether = CreateIoCompletionPort((HANDLE)SerVerSocket, duankou, 0, 0);
	if (0 == tegether)
	{
		int a = GetLastError();
		printf("端口与socket绑定错误码：%d", a);
		Clear();
		WSACleanup();
		return;
	}
	//开启监听
	if (SOCKET_ERROR == listen(SerVerSocket, SOMAXCONN))
	{
		int a = WSAGetLastError();
		Clear();
		WSACleanup();
		return;
	}
	//投递AcceptEx
	if (0 != PostAccept())
	{
		Clear();
		WSACleanup();
		return;
	}
	//获取核数（用来当作最佳线程数）
	SYSTEM_INFO systemProcessorsCount;		//对象
	GetSystemInfo(&systemProcessorsCount);	//对象初始化（当前操作系统的信息）
	nProcessorsCount = systemProcessorsCount.dwNumberOfProcessors;//获取到系统信息中的核心数量
	//创建多线程
	pThread = (HANDLE*)malloc(sizeof(HANDLE) * nProcessorsCount);	//句柄数组
	if (0 == pThread)
	{
		Clear();
		WSACleanup();
		return 0;
	}
	for (int i = 0; i < nProcessorsCount; i++)
	{
		pThread[i] = CreateThread(NULL, 0, ThreadProc, duankou, 0, NULL);			//将新开的线程句柄装到数组中
		//失败处理
		if (NULL == pThread[i])
		{
			int a = WSAGetLastError();
			printf("端口与socket绑定错误码：%d", a);
			Clear();
			CloseHandle(duankou);		//释放完成端口
			WSACleanup();
			return;
		}									
	}
	

	//卡住，不让程序结束，阻塞
	while (1)
	{
		Sleep(1000);//暂时挂起1000毫秒，然后继续执行循环，然后再挂一会，挂起时此程序不消耗CPU资源
	}
	
	//释放
	free(pThread);
	Clear();
	WSACleanup();

	system("pause>0");
	return 0;
}
//创建多线程函数参数3，线程中具体要执行的内容，此函数的参数由创建函数参数4传递
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	while (g_flag)
	{
		//从通知队列拿通知，			队列没有通知时，会暂时将创建的多线程挂起，不占用CPU资源
		HANDLE port = (HANDLE)lpParameter;	//参数1，完成端口，也可以直接用全局声明好的完成端口，不用这个参数来强转
		DWORD NumberOfBytes;				//参数2  接收或者发送的字节数会返回到这个参数里
		ULONG_PTR index;					//参数3	会通过函数执行将正在处理的通知的socket的数组下标传进到这个参数里
		LPOVERLAPPED lpOverlapped;			//参数4 （注意这是一个指针类型，而填写参数时要加上&，即参数要填写二级指针）会将当前通知的重叠结构地址返回到这个参数
		BOOL bFlag = GetQueuedCompletionStatus(port, &NumberOfBytes, &index, &lpOverlapped, INFINITE);
		//失败处理、强制下线处理
		if (FALSE == bFlag)
		{
			int a = GetLastError();
			if (64 == a)
			{
				printf("客户端强制下线：%d\n", g_sockAll[index]);
			}
			else
			{
				printf("error 错误码: %d\n", a);
			}
			continue;
		}

		//成功了，分类处理
		//是服务器，则是有连接请求，需要accept
		if (0 == index)
		{
			//新客户端绑定完成端口,参数3传递客户端socket的下标，参数2是完成端口，可用全局的duankou，也可用port（线程函数参数强转而来的，也是完成端口）
			HANDLE clientio = CreateIoCompletionPort((HANDLE)g_sockAll[g_count], duankou, g_count, 0);
			if (0 == clientio)
			{
				//此处客户端绑定出错，无需关闭网络库与删除数组
				int a = GetLastError();
				printf("端口与socket绑定错误码：%d\n", a);
				closesocket(g_sockAll[g_count]);
				continue;
			}
			//投递客户端recv
			PostRecv(g_count);
			printf("延时连接，已连接客户端：%d\n", g_sockAll[g_count]);
			//计数++，为了重新投递accept让下一个新客户端能连接
			g_count++;
			//重新投递accept
			PostAccept();
		}
		//是客户端
		else
		{
			//发送或接收的字节数为0，则是客户端下线
			if (0 == NumberOfBytes)
			{
				//客户端正常下线处理，强制下线在错误处理里面
				printf("客户端正常下线：%d\n", g_sockAll[index]);
				//清除对应客户端socket、事件
				closesocket(g_sockAll[index]);
				int wsc = WSACloseEvent(g_olpAll[index].hEvent);
				//将数组中的socket、事件，设为特殊值，不能交换因为完成端口已经与对应下标绑定了（笔记里有详细原因）
				g_sockAll[index] = 0;
				g_olpAll[index].hEvent = NULL;
			}
			//有接受或发送的字节数，则进行recv与send的分类处理
			else
			{
				//recv处理
				if (0 != g_str1[0])
				{
					//打印、然后清空字符数组
					printf("recv:%s\n", g_str1);
					memset(g_str1, 0, sizeof(g_str1));
					//重新投递
					PostRecv(index);
				}
				//send处理
				else
				{
					printf("send ok\n");
					memset(g_str2, 0, MAX_RECV_COUNT);
				}
			}
		}
	}	
}
int PostRecv(int Index)
{
	struct _WSABUF buf;			//参数2
	buf.buf = g_str1;
	buf.len = sizeof(g_str1);
	DWORD count;		//参数4
	DWORD DwFlag = 0;   //参数5
	int wsrec = WSARecv(g_sockAll[Index], &buf, 1, &count, &DwFlag, &g_olpAll[Index], NULL);
	//函数执行错误处理
	int wsa = WSAGetLastError();
	if (ERROR_IO_PENDING != wsa)
	{
		printf("recv error:%d\n", wsa);
		return 0;
	}
	return 0;
}
int PostSend(int Index)
{
	WSABUF buf;
	buf.buf = g_str2;
	buf.len = sizeof(g_str2);
	DWORD Sendcount;		//参数4
	DWORD SeDwFlag = 0;		//参数5
	int s = scanf("%s", buf.buf);
	int nsend = WSASend(g_sockAll[Index], &buf, 1, &Sendcount, SeDwFlag, &g_olpAll[Index], NULL);
	//函数错误处理
	int wsa = WSAGetLastError();
	if (ERROR_IO_PENDING != wsa)
	{
		printf("send error:%d\n", wsa);
		return 0;
	}
	return 0;
}
int PostAccept(void)
{
	
	//创建客户端socket，AcceptEx不会主动创建,参数2
	g_sockAll[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == g_sockAll[g_count])
	{
		int a = WSAGetLastError();
		return 1; 
	}
	g_olpAll[g_count].hEvent = WSACreateEvent();
	//参数3，不能填NULL，参数4填0可取消参数3功能
	char str[1024];
	//参7,可以填NULL，也可以填DWORD,参8是重叠结构
	DWORD DwRecvCount;
	BOOL nacce = AcceptEx(g_sockAll[0], g_sockAll[g_count], &str, 0, sizeof(struct sockaddr_in)+16, sizeof(struct sockaddr_in)+16, &DwRecvCount, &g_olpAll[0]);
	//函数错误处理
	int wsa = WSAGetLastError();
	if (ERROR_IO_PENDING != wsa)
	{
		printf("Accept error:%d\n", wsa);
		return 0;
	}
	return 0;
}
void Clear(void)
{
	//释放socket、事件数组
	for (int i = 0; i < g_count; i++)
	{
		if (0 != g_sockAll[i])
		{
			closesocket(g_sockAll[i]);
			WSACloseEvent(g_olpAll[i].hEvent);
		}
	}
	//释放线程句柄数组
	for (int i = 0; i < nProcessorsCount; i++)
	{
		CloseHandle(pThread[i]);
	}
	//释放完成端口
	CloseHandle(duankou);
	g_count = 0;
}











void studybook()
{
	{
		//8.1 重叠IO模型问题总结

		/*
		*	完成端口：
		*			完成端口也是windows的一种机制，在重叠IO模型基础上的优化（所以重叠IO学好了，这个就跟闹着玩似的）
		* 
		*	一根线程有6个客户端是比较合理的，下节课会讲最优线程数量
		*	
		*	重叠IO模型的局限：
		*				事件通知：
		*						1.无序的，并不按照客户端响应的先后顺序来进行处理，而是按照遍历询问的顺序
		*						2.循环询问：延迟高，做了很多无用功（比如3万个用户，只有俩响应，那还是要遍历3万次，优化后能减少，但还是有可能遍历3万次）
		*						3.采用了多线程：如果客户端数量太多，线程就很多，那效率就会下降，且线程分配也有问题
		*										比如每根线程有300个用户，那3万就要100根线程，如果只有10个用户响应，且都在一根线程中，那99根线程就是闲置浪费了的
		*				完成例程：
		*						不需要去遍历询问，但还是有问题
		*						1.线程数量太多，比事件通知还多，因为每个客户端都需要一个线程去调用回调函数，并在在这根线程中完成回调函数
		* 
		*	线程过多的坏处，并讲解线程的一些基本常识：
		*				单核多线程：
		*						在一个时间内，每根线程执行一样的时间（就像上学LINUX的那门课老师说的那样）
		*						单核是伪多线程，骗人的
		*						
		*									1.假设一个时间片周期是1毫秒，有100个线程在单核上运行，那么这些线程一个时间周期内分得0.01毫秒执行时间，
		*									时间到，不管执行到什么位置，立刻切换下一个线程执行，如此，这些线程就在不停的切换中执行，由于速度太快，咱们人分辨不出来，
		*									所以就展现出同时运行的效果
		* 
		*									2.同一个应用程序，在单核中执行多个线程，理论上还是单线程的执行效率，假设一条线程执行完需要1秒，那么10个线程，总共执行完需要10秒。
		*									单核上运行10条，还要加上线程切换的时间，所以大于10秒
		*						线程切换会消耗大量cpu资源/时间
		*								比如一个程序不用多线程需要15秒完成
		*								如果用单核多线程来执行，这个程序需要的时间大于15秒
		*								只是能有异步这一个好处
		*									1.这根线程处理一个小周期，时间到了，就停止这根线程的执行，跳转到下一根线程去执行同样小周期的时间
		*									2.那这个跳转的过程，它势必要记住：
		*											1.线程的一些信息（比如地址，执行到哪了等）
		*											2.跳转到线程后要读取再执行
		*									3.那这样切换线程就会浪费一些时间，相对于计算机的运算能力，这个时间就浪费的比较多
		*									
		*				多核多线程：
		*						多个核的线程，达到了真正的同时运行
		*								所以假设咱们电脑是8核cpu，那么咱们的程序创建8条线程，操作系统会自动把8条线程分给八个核，从而达到了最好的效率
		*								正常情况下，电脑运行着很多的软件，有几千个线程同时运行，大家保证咱们自己软件的线程是这么多就可以
		*								系统首先是以软件为单位进行线程分配执行的
		*									线程分配不是按照先后顺序分配，是平均分配，以软件为单位
		*						因为每个核都是独立的运行计算器，
		*								比如一个正常要12秒的程序，用单核多线程执行需要超过12秒的时间，只是提供了异步操作这一个好处
		*								但是用多核多线程，比如4核，那就真的是让个独立的计算器来同时执行，结果是3秒就完成了（可能比3秒多一点）
		*								因为单核只是在每根线程上执行一点，然后马上跳到下一个线程，切换速度太快，雨露均沾，就营造出同时执行的假象
		*								
		*						
		*	我们打开电脑的任务管理器-性能就能看到CPU线程，核心数，句柄等信息
		*				句柄（ID）：就是一个标识，就像人的名字，窗口有它的句柄，socket有句柄，案例有句柄
		*	
		*	操作系统是如何执行这个单核、多核的多线程的，我们是看不到的
		*	老师这个描述是大白话版本，我们理解了后，就知道了大概，然后以书上官方描述为准
		*/

		//8.2 最优线程数量

		/*
		* 最优线程数量：
		*		理论上跟cpu核数一样是最好的
		*		网上有几种答案
		*					CPU核数
		*					CPU核数*2
		*					CPU核数*2+2	（这是网上的一些人通过实践得来的经验）
		*	CPU有个重要参数就是n核m线程
		*		比如我这个i7 700K：4核8线程
		*		
		*		4核指物理四核，也就是cpu真实核数
		*			理论上最佳的线程数是4个，但是因特尔在此使用了超线程技术，使得物理单核可以模拟出双核得性能，所以这就是8的来历，所以在应用中跟8核没啥两样
		*			这应该就是 CPU核数*2 的来历
		* 
		*		i7 9700K：8核8线程
		*			这个就是没有使用超线程技术，单核单线程
		*			所以线程数 CPU核数 
		*		
		*		CPU核数*2+2  这个数量，是根据实际应用中的经验得来
		*		线程函数中可能会调用Sleep(),WSAWaitForMultipleEvents()...这类函数，会使线程挂起（不占cpu时间片），从而使得CPU某个核空闲了，
		*		这就不好了，所以一般我们多建个两三根，以解决此类情况，让CPU不停歇，从而在整体上保证程序执行效率
		*		
		*	引申理解：
		*		考虑切换时间，数量就会有个瓶颈。可用系统性能分析工具来帮忙分析
		*			比如我们给一个程序100个线程，再假设1秒为一个时间周期，那此时一跟线程上执行了1/100秒
		*			然后我们给另一个程序200根线程，那此时每根线程执行了1/300秒
		*			然后线程切换得越来越频繁，浪费的时间也就越来越多，这样的话效率就会下降很多
		*			也就是说，开辟新线程会占用其它软件的运行效率
		*	
		*	完成端口模型的特点：
		*			1.模仿消息队列，创建一个通知队列，系统来创建，将响应的事件放到里面，
		*					能保证有序，不做无用功（队列的特性是先进先出），谁先响应就先存放到里面，然后也先取出来处理
		*					不会有只有2个用户响应，就遍历很多次来找出这2用户的情况
		*			2.创建最佳数量的线程，用这最佳性能数量的线程来依次处理队列取出来的事件，充分利用了CPU的性能
		*		
		*		结合以上两点：完成端口做到了：公平（队列的先进先出），谁先响应，就先存到里面先处理
		*					高效，利用最佳性能的线程数来处理队列中的事件，
		*		
		*		100根线程与8根线程对比：肯定是8根线程的更高效，因为线程一多，就要考虑到线程切换带来的效率浪费
		*		8根线程或者10跟线程这些是经过验证与逻辑合理性的推敲的。
		*/

		//8.3 完成端口代码逻辑
	
		/*
		*	大部分代码与重叠IO模型一摸一样
		*	我们学习网络模型，很大程度上也是学习这种编程思想，这是宝贵的财富，不要只会用
		* 
		* 完成端口代码逻辑：
		*		原理：
		*			1.重叠结构与socket绑定，然后绑定后的重叠套接字再与一个完成端口绑定在一起（一种类型的变量）
		*			2.使用AcceptEx, WSARecv, WSASend 投递请求，（就是让系统监视有无连接等的信号）
		*			3.当系统异步的完成请求（就是监视到响应产生了信号），就会把通知存放进一个队列，这个队列我们叫他通知队列
		*				这个队列由操作系统来创建、维护
		*			4.完成端口 就可以理解为这个队列的头（出口），通过GetQueuedCompletionStatus从这个队列头向外拿通知
		*				一个一个的拿出来，然后用创建好的最佳数量线程来处理
		* 
		*		代码：
		*			创建完成端口：
		*						CreateIoCompletionPort
		*			将完成端口与socket（重叠套接字）绑定：
		*						CreateIoCompletionPort
		*						创建与绑定都由这一个函数来完成，一个函数两个功能
		*			创建指定数目的线程：
		*						创建CPU核数一样的线程：CreateThread
		*						线程内部：GetQueuedCompletionStatus 获取到通知然后分类处理
		*			处理完，继续使用AcceptEx WSARecv WSASend投递请求
		*			主线程阻塞
		* 
		*/

		//8.4 创建完成端口

		/*
		* 释放完成端口: CloseHandle(duankou);		
		* 
		* 一个函数两种功能：
		*			就是一个函数根据参数的不同，在不同的位置有不同的功能
		*			比如：	CreateIoCompletionPort函数
		*				功能一：创建一个完成端口
		*				功能二：将完成端口与SOCKET绑定到一起
		*		函数原型：	
		*			HANDLE CreateIoCompletionPort(
		*				  HANDLE    FileHandle,
		*				  HANDLE    ExistingCompletionPort,
		*				  ULONG_PTR CompletionKey,
		*				  DWORD     NumberOfConcurrentThreads
		*				);
		* 
		* 参数也要依据不同功能有不同的填法：
		*		创建一个完成端口：
		*				参数1：	INVALID_HANDLE_VALUE  ，此时参数2必须为NULL。参数3忽略（填0）
		*				参数2：	NULL
		*				参数3：	0
		*				参数4：	填CPU的核数即可，可填写0（表示默认是cpu核数）
		*						此参数表示允许此端口上最多同时运行的线程数量，CPU核数可通过GetSystemInfo函数获取
		*		将完成端口与SOCKET绑定到一起：
		*				参数1：服务器socket
		*				参数2：完成端口变量
		*				参数3：再次传递socketServer，也可以传递一个下标做编号
		*					   与系统接收到的对应的数据关联在一起
		*					   代码中服务器绑定完成端口的参数3是下标的含义,填了0，是服务器的下标，与创建完成端口的0不一样
		*				参数4：忽略此参数，填0就行了
		*					   当说忽略的时候，咱们填啥都是没有作用的，我们一般填0
		* 
		*			返回值：
		*				成功：
		*					当参数2为NULL，返回一个新的完成端口	
		*					当参数2不为NULL，返回自己
		*					当参数1为socket，返回与socket绑定后的端口
		*				失败：
		*					失败返回0，完成端口也是windows的一种机制，不是网络专属，文件操作均可以
		*		
		*/
		
	};

	{
		//8.5 创建线程
		/*
		* 
		* 获取CPU核数：GetSystemInfo
		*		SYSTEM_INFO systemProcessorsCount;				//创建系统信息的对象（成员是一些系统的相关信息）
		*		GetSystemInfo(&systemProcessorsCount);			//从当前的操作系统获取到信息，装到之前的对象中（给对象初始化），这样对象的成员就是当前操作系统的信息了
		*		int nProcessorsCount = systemProcessorsCount.dwNumberOfProcessors;//再获取到系统信息中，CPU的核数量
		* 
		* 获取线程数量（核数）
		* 创建线程	//要在投递之后才是符合逻辑的，没有投递就没有任务，创建线程干什么
		* 当线程函数代码里有sleep waitformuiltEvents这种函数，就多创建几个
		*	函数原型：
		*			HANDLE CreateThread(
		*			LPSECURITY_ATTRIBUTES      lpThreadAttributes,
		*			SIZE_T                                           dwStackSize,
		*			LPTHREAD_START_ROUTINE  lpStartAddress,
		*			LPVOID                                         lpParameter,
		*			DWORD                                        dwCreationFlags,
		*			LPDWORD                                    lpThreadId
		*			);
		*	功能：创建一根线程
		*	参数1：	线程句柄是否能被继承，NULL不继承	（就像父窗口与子窗口的关系那样，这样句柄有共享与复制两种使用方式）
		*			指定线程的执行权限，NULL默认的		（权限就像windows一些文件或文件夹需要权限才能运行打开）
		*	参数2：	线程栈大小，填0，系统使用默认大小（堆区每个变量空间默认为1M大小）
		*			不填0，自己指定，则字节为单位，比如5M要填：1024*1024*5
		*	参数3：	线程函数地址，参数，就是参数4传递进来的数据，用DWORD WINAPI ThreadProc(LPVOID lpParameter);来创建
		*	参数4：	外部给线程传递数据
		*	参数5：	0 ：线程立即执行
		*			CREATE_SUSPENDED ：线程创建完挂起状态，调用ResumeThread启动函数（就像电脑睡眠一样，只是暂停了，随时能激活启动，与未开机不一样，与关机也不一样，更高效）
		*			STACK_SIZE_PARAM_IS_A_RESERVATION：	设置了，参数2就是栈保留大小，虚拟内存上栈得大小，1M
		*												未设置，就是栈提交大小，物理内存上栈得大小，4Kb
		*												栈保留大小能在项目右击-属性-链接器-系统中设置修改
		*	参数6：	线程ID，可以填NULL
		*	返回值：	成功：返回线程句柄，句柄属于内核对象，最后要释放（通过CloseHandle函数）
		*			失败：返回NULL，通过GetLastError得到错误码，进行错误处理
		* 
		*	总结：
		*			创建多线程函数的参数3，是开辟的线程中具体要执行的内容，此函数的参数由创建函数参数4传递
		*			参数4老师填的是完成端口，我们在全局区声明变量，这样就能在函数中直接使用而不传递参数，就像Clear()函数中用的那两个
		*			参数4的完成端口，如果有响应，则会将完成端口传给给参数3：线程函数，然后线程函数内通过另一个函数从完成端口取通知队列内容进行分类处理
		*	
		*	虚拟内存与物理内存：
		*			物理内存就是内存条的那种物理实物，早期计算机是直接操作物理内存的。但问题是，一旦地址偏移，可能就到了另一个软件，这样危险系数高。
		*			虚拟内存就是为了安全性而生的，比如我们平常操作的指针，或者空间地址等都是虚拟内存
		*			虚拟内存映射着物理内存的实际位置，我们操作指针就是通过虚拟内存映射操作物理内存
		*			
		* 完成端口与句柄要在程序结尾释放
		*			我写在Clear()函数中了
		*/
		//8.6 操作通知队列，（线程函数内执行的函数）
		/*
		* 我们因为主函数里没有循环，会直接执行到程序结束，所以我们要加上一个死循环卡住
		* 采用Sleep的原因：Sleep是暂时挂起的功能，挂起的时候不消耗CPU资源，对CPU更友好，即挂起时，此程序不消耗CPU线程资源
		* 但从我们程序内来看，不加Sleep的while与加了是没有明显差异的
		* 
		* GetQueuedCompletionStatus
		* 函数原型：
		*		  BOOL WINAPI GetQueuedCompletionStatus(
		*		  HANDLE       CompletionPort,
		*		  LPDWORD      lpNumberOfBytes,
		*		  PULONG_PTR   lpCompletionKey,
		*		  LPOVERLAPPED *lpOverlapped,
		*		  DWORD        dwMilliseconds
		*		);
		* 操作通知队列函数：
		*		就是从通知队列里向外拿通知（数据包），然后执行后续分类处理
		*	功能：
		*		没有接收到事件通知的时候，将线程挂起，不消耗CPU线程资源（就是这跟线程就当暂时没有一样），不占用CPU时间，很棒
		*		尝试从指定的I/O完成端口出队列I/O完成数据包
		*		就是通过完成端口来取通知队列内的通知，然后进行分类处理（完成端口可以看成通知队列的门）
		* 
		*	参数1：完成端口
		*	参数2：接收或者发送的字节数
		*	参数3：完成端口绑定socket函数的参数3是同一个，通过那个参数3传过来
		*	参数4：重叠结构（因为我们完成端口绑定socket时，socket就已经绑定了一个重叠结构）
		*	参数5：等待时间，填INFINITE，一直等，闲着也是闲着
		*		   当没有客户端响应时候，通知队列里什么都没有，咱们这里也get不到什么东西，那么等一会儿，还是一直等
		*	返回值：成功返回TRUE,失败返回FALSE
		*		
		* 
		*/
		//8.7 分类处理，处理accept通知
		/*
		* 获取到线程函数当前通知的socket的下标：
		*	BOOL bFlag = GetQueuedCompletionStatus(duankou, &NumberOfBytes, &index, &lpOverlapped, INFINITE);
		*		三种方式：
		*				1.通过参数4，lpOverlapped（当前通知的重叠结构地址会返回到这个参数），这个参数地址 减去 首元素地址 强转成int就得到了下标（重叠IO模型里的方法）
		*				2.参数4强转成结构体，然后结构体就能访问socket了，老师说这种不好理解且有BUG，这种需要数组放到结构体中
		*				3.参数3直接获取下标，（很方便，就是我们现在用的方法）但是需要绑定完成端口与socket时参数3传递下标，如：
		*						 CreateIoCompletionPort(&g_sockAll[g_count], duankou, g_count, 0);
		* 分类处理逻辑：
		*		 0 == index 则当前通知是服务器的，那只能是有连接请求，需要accept，需考虑立即完成时的绑定完成端口
		*		 (0 == NumberOfBytes) 发送或接收的字节数为0（取通知函数的参数2），则是客户端下线
		*		 0 != g_str1[0] 接收recv的全局字节数组不为0，则是有收到信息，否则就是send
		* 
		*  总结：请看代码更容易理解
		*		客户端绑定出错，无需关闭网络库与删除数组
		*		我们现在写的代码都是基于熟悉前面学的东西上的，不熟悉就不知道要干嘛，比如处理accept就不知道为什么投递recv后要g_count++，
		*		还有一些参数在函数间的传递，要对函数熟悉了才知道哪些函数参数对应哪个函数的参数，谁传给谁
		*		网上大多是用获取通知函数参数4来获取下标或socket的，我们用参数3更直接方便
		*		需考虑立即完成时的绑定完成端口，就是PostAccept立即完成的那条线路需要加上绑定完成端口
		*		需考虑立即完成时的绑定完成端口，就是PostAccept立即完成的那条线路需要加上绑定完成端口，线程函数中是延时处理那条线路
		*/
		//8.8 重要**** 分类处理recv、send
		/*
		* 先谈我的错误！！！：很重要
		*		1.首先，我为了方便，完成端口、多线程数组等都在全局区声明了，但是主函数中我懒得删掉定义时的声明，比如HANDLE duankou 主函数中我就没删掉
		*				结果出大错了，线程函数中，绑定新客户端我使用了duankou，但是recv一直无法收到通知，无法处理，折腾了几个小时，
		*				最后发现使用：HANDLE port = (HANDLE)lpParameter; 的port就没事，然后删掉主函数的完成端口类型名 HANDLE，再使用duankou就可以了
		*			反思：全局变量有声明，主函数或者其它函数就不要再加声明了，直接赋值就好，应该是调用时产生了二义性，不知道用主函数的还是全局的
		*		2.此完成端口是在重叠IO的事件通知基础上，没有用到完成例程的回调函数，而我居然把回调函数用上了，
		*				改回事件通知时，要把WSARecv的尾参数从回调函数改成NULL，且PostRecv和send的循环删掉
		*		3.CreateIoCompletionPort((HANDLE)g_sockAll[g_count], duankou, g_count, 0); 绑定完成端口的函数
		*				参数1要加上类型转换(HANDLE)，否则不能识别，绑定服务器的那个也是一样
		*		4. PULONG_PTR index; 要把P删掉，因为P或LP代表指针，删掉，然后参数里加&，这才是函数原型的看法用法
		*		
		* 本课的注意点：
		*		1.客户端下线处理，不能与其它模型一样交换尾元素然后计数-1，因为完成端口与socket还有下标绑定在一起的，交换完，下标就乱了
		*			正确处理方式：将那个下标的空间设为一个特殊值比如0和NULL，然后在Clear清理socket时加上 != 0 的过滤即可
		*		2.主函数里的while(1)是让主线程就是主函数（本程序）不结束，与我们的线程函数无关，因为不在一个线程上，我们分类处理都是在单独开辟出的线程里循环完成
		*		3.获取通知这些函数，得到错误码使用GetLastError(); 不加WSA的，因为他们不是网络库的函数，网络函数都带有WSA
		*			所以客户端强制下线也不是10054了，而是64
		*		4.buf.buf = g_str1;这一步我一直以为是将g_str1的值赋给buf.buf，recv是装在buf.buf里的，然后线程函数里使用g_str1判断我就想不通
		*			认为recv不是把信息传递给g_str1，而是buf.buf，
		*			但是我错了，这很基础的问题，那是把g_str1的地址传递给buf.buf，不是赋值
		*			赋值的话要使用*g_str1，这里是传址，所以函数将信息装到buf.buf就等于装到了g_str1。
		*		5.因为与重叠IO的整体逻辑类似，有两条线路，一条是立即完成，一条是延迟处理，
		*			所以立即完成里也要加上绑定完成端口
		*		6.C语言，函数不要参数的话，函数声明与定义要加上void，表明没有参数，如果使用函数时加了参数，就会有警告
		*			不加void，我们不小心加了参数，是不会报错提示的。C++就不用加void
		* 
		* 总结：
		*		这节课我犯的错误就是一些很基础的问题，整理完这些问题，我觉得基础应该再深入理解一下，比如全局与主函数都加声明，这是否是二义性的问题等等
		*		然后整体流程我也应该熟悉，不然用了回调函数自己都不知道不能用（因为回调函数是完成例程的延迟处理），完成端口的延迟处理就是线程函数
		*		亦或是各个线程分不清哪个是哪个，主函数里的while我还以为会对延迟处理的线程运行有影响，其实只是让本程序不要结束而已（本程序在不同的线程里）
		*/
		//8.9 完成端口的总结
		/*
		* P70课时老师有流程图，我们可以对比着看一下差别
		*		与重叠IO的完成例程差别不大，完成例程是线程空间去等待信号，等到了信号自动调用回调函数执行
		*		完成端口也是先投递，然后等到有信号就在开辟的多线程的线程函数里分类处理，二者差不多
		*		完成端口好处是用了最优的线程数，就省下了CPU负担与整体效率
		* 
		* 本课对代码进行一些优化：
		*		PostRecv与PostSend里原先有分为两条线，立即完成与延迟完成，现在可以把它们删掉
		*		只用线程函数的那一条线路即可
		*		因为只要投递了，操作系统监视产生信号，就会把事件通知存到通知队列中等待处理
		*		重叠IO需要询问等待信号来进行分类处理，这里不用询问，直接把有信号的事件存到通知队列中
		* 
		*		我们的线程是一个死循环，想要把线程给关掉，两种方法：正常关闭、强杀
		*			正常关闭：用BOOL g_flag = TRUE; 来当作循环的条件，当我们改变g_flag为FALSE时，就能关闭循环了
		*					我们放到控制台函数里，这种方式在多线程编程里很常用
		*			强杀：用Terminatethread函数来强制关闭（老师说不太好，且不常用，就没教）
		* 线程池：比如8根线程（最优数字），存到一个类似于栈的数据结构中，有一个通知，就从栈顶取一个线程去处理，线程处理完再存回栈中，
		*			这种方式比较高效
		* 
		* 优化：
		*		整体逻辑没什么优化得了，就是这样，细节还能优化
		* 
		* 对比：
		*		对比查看这些模型的区别，这样才能更好理解，老师有流程图，可以对比着看
		* 
		* 老师说我们不要只是学习怎么使用，而是学习它的实现逻辑，思想等。等有一天我们要实现一个大型软件，就能使用这种思想框架来提高软件运行效率
		*		网上有一些人写的代码很花哨，但整体逻辑不变的，可以自己去探索着学	
		*/
	};
}