#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#include <MSWSock.h>
#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSOCK.lib")

//创建socket数组、重叠IO结构体数组
#define MAX_COUNT 10240
SOCKET g_sockall[MAX_COUNT];	//socket数组
OVERLAPPED g_olpall[MAX_COUNT];	//重叠IO结构体数组
int g_count = 0;				//计数

//Recv的回调函数
void CALLBACK RecvCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags);
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
		//询问信号
		DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[0].hEvent, FALSE, WSA_INFINITE, TRUE);
		if (wfmte == WSA_WAIT_FAILED || wfmte == WSA_WAIT_IO_COMPLETION)
		{	//出错了  和  完成例程的完成信号
			continue;
		}
		//重置服务器信号
		BOOL ChongZhi = WSAResetEvent(g_olpall[0].hEvent);
		if (FALSE == ChongZhi)
		{
			int a = WSAGetLastError();
			continue;
		}

		//客户端接受连接
		//接受连接完成了（因为投递给操作系统，操作系统完成了）
		printf("accept 延迟客户端%d连接成功 \n", g_sockall[g_count]);
		//向系统投递recv，因为有了新连接
		PostRecv(g_count);		//使用g_count，不要用n，因为n是当前的服务器，而g_count才是新客户端的
		//根据情况投递send
		//PostSend(g_count);
		//计数++
		g_count++;
		//重新投递accept，因为进到这里就说明在之前的accept已经完成
		PostAccept();
	}
	

	//清理网络库与数组(socket 和 event(事件))
	Clear();
	WSACleanup();

	system("pause>0");
	return 0;
}

//recv的回调函数
void CALLBACK RecvCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	//参1是错误码，为10054则说明是强制下线
	//参2是得到recv获取的内容的字节数，为0则说明正常下线
	//参3是当前socket的重叠结构
	//参4是WSArecv的参数5（就像recv函数的参数5，指定协议缓冲区的读取方式，比如有读了不删除，读了就删等）
	
	//获取当前事件的下标(优化的写法，地址-首元素地址 == 下标的数值)
	int i = lpOverlapped - &g_olpall[0];
	//下线处理
	if (10054 == dwError || 0 == cbTransferred)
	{
		printf("客户端%dclose了\n", g_sockall[i]);
		//删掉下线客户端的socket与事件
		closesocket(g_sockall[i]);
		WSACloseEvent(g_olpall[g_count - 1].hEvent);
		//用末尾元素填充到删除的客户端的位置
		g_sockall[i] = g_sockall[g_count - 1];
		g_olpall[i] = g_olpall[g_count - 1];
		//计数-1
		g_count--;	
	}
	//接收消息
	if (cbTransferred != 0)
	{
		//打印信息
		printf("recv: %s\n", g_recv);
		//将全局变量str清零
		memset(g_recv, 0, MAX_RECV_COUNT);
		//根据情况投递send
		//调用自己，继续传递WSARecv，等待有信息发来
		PostRecv(i);
	}
}
//Send的回调函数
void CALLBACK SendCALL(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags)
{
	printf("WSASend的回调函数成功调用\n");
}
//PostAccept投递函数的配套逻辑封装
int PostAccept()	//正常则返回0，否则返回错误码
{
	while (1)
	{
		//参数2，客户端socket，需要手动创建
		g_sockall[g_count] = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (INVALID_SOCKET == g_sockall[g_count])
		{
			int a = WSAGetLastError();
			break;
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
			//接受连接完成了（因为投递给操作系统，操作系统完成了）
			printf("accept 立即客户端%d连接成功 \n", g_sockall[g_count]);
			continue;
		}
		//投递后没有立即完成，也可能出错了
		else
		{
			int a = WSAGetLastError();
			//暂时没有客户端连接,进行延迟处理
			if (a == ERROR_IO_PENDING)
			{
				break;
			}
			//出错了
			else
			{
				break;
			}
		}
	}
	return 0;
}
//WSArecv函数的配套逻辑封装
int PostRecv(int Index)
{
	while (1)
	{
		//参数2
		struct _WSABUF buff;
		buff.buf = g_recv;			//str已经在全局区定义了
		buff.len = sizeof(g_recv);

		DWORD count;		//参数4
		DWORD DwFlag = 0;   //参数5
		int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], RecvCALL);
		//立即发生
		if (0 == nrecv)
		{
			//打印信息
			printf("recv: %s", buff.buf);
			//将全局变量str清零
			memset(g_recv, 0, MAX_RECV_COUNT);
			//根据情况投递send
			continue;
		}
		//延迟处理或者出错了
		else
		{
			int a = WSAGetLastError();
			if (WSA_IO_PENDING == a)
			{
				break;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
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
	DWORD SeDwFlag = 0;		//参数5
	int nSend = WSASend(g_sockall[Index], &buff, 1, &Sendcount, SeDwFlag, &g_olpall[Index], SendCALL);
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
		closesocket(g_sockall[i]);	//socket数组
		WSACloseEvent(g_olpall[i].hEvent);	//重叠IO结构体数组
	}
	g_count = 0;
}

void studybook()
{
	{
		//7.1 完成例程基本原理介绍
		/*
		* 本质：
		*		将socket与一个函数绑定起来，然后重叠IO模型的特性，投递到操作系统异步完成如WSARecv等操作的时候
		*		系统会异步调用各自绑定的函数，进行自动分类处理
		*		我们就在函数中进行相应处理
		* 事件通知的模型：
		*		投递后，要调用WSAGetOverlappedResult函数获取信号的对应的操作，然后手动根据逻辑进行分类处理
		*		自己分类，分类的逻辑比较多
		* 对比：
		*		对比下，完成例程会更好一点，因为是自动调用函数，也就是自动分类
		*		且事件通知需要一个个按顺序遍历，询问信号，效率比起完成例程慢了一点
		*		除了WSARecv、WSASend的参数有一些区别（完成例程要加上回调函数），
		*		其它的代码包括AcceptEx函数都是一样的，整体逻辑也一样
		* 代码逻辑：
		*		创建事件数组，socket数组，重叠结构体数组：下标相同的绑定成一组
		*		创建重叠IO模型使用的socket：WSASocket
		*		投递AcceptEx：
		*					立即完成：
		*							对客户端套接字投递WSARecv，WSARecv也分立即完成与延迟完成，相当于递归
		*							根据需求对客户端套接字投递WSASend
		*							对服务器套接字继续投递AcceptEx
		*					延迟完成
		*							循环等信号：
		*									等信号：WSAWaitForMultipleEvents
		*									有信号：接受连接，投递AcceptEx
		*									根据需求对客户端套接字投递WSASend
		* 
		*		
		*/
		//7.2 完成例程回调函数介绍
			/*
			* 回调函数：
			*	需要对比WSAGetOverlappedResult函数学习。
			*	函数原型：
			*		void CALLBACK funnny
			*		(
			*		DWORD dwError, 
			*		DWORD cbTransferred, 
			*		LPWSAOVERLAPPED lpOverlapped,
			*		DWORD dwFlags
			*		)
			*	返回值
			*			void
			*	调用约定
			*			CALLBACK	（调用约定类型并不多，记得写上）
			*	函数名字
			*			随意
			*	参数1
			*			错误码		（对应上面得到信号具体操作函数的错误码，用WSAGetLastError获取的错误码）
			*	参数2
			*			发生或者接收到的字节数（对应得到信号具体操作函数的参数3，就是正常下线为0的那个参数）
			*	参数3
			*			重叠结构		（对应得到信号具体操作函数的参数2）
			*	参数4
			*			函数执行的方式（对应WSArecv的参数5）
			*			对照WSArecv参数5
			*
			*	处理流程：
			*			dwError == 10054：强行退出，先判断，然后确定了再删除客户端
			*			cbTransferred == 0：正常退出
			*			else：处理数据
			*			继续投递接收请求
			*	
			*	WSAGetOverlappedResult函数:	获取信号触发事件的具体操作
			*			BOOL WSAAPI WSAGetOverlappedResult
			*			(
			*			  SOCKET          s,
			*			  LPWSAOVERLAPPED lpOverlapped,
			*			  LPDWORD         lpcbTransfer,
			*			  BOOL            fWait,
			*			  LPDWORD         lpdwFlags
			*			);
			* 
			*			对比这个事件通知需要的函数
			*			正好对上咱们回调函数的参数意义
			*			事件通知通过该函数得到这些信息
			*			同样的信息，完成例程也需要，所以通过参数传递
			*		
			*/
		//7.3 完成例程事件分类处理
			/*
			* 
			* 
			*	需要对事件通知代码添加或修改的地方：
			*		1.	DWORD wfmte = WSAWaitForMultipleEvents(1, &g_olpall[0].hEvent, FALSE, WSA_INFINITE, TRUE);
			*			参数4 从0 改为 WSA_INFINITE，参数5从FALSE改为TRUE TRUE是给完成例程使用的，下面会介绍它的意义
			*			下标换成服务器的，我们只要一直等待服务器的就好了，就是等待accept，其它的交给回调函数来判断处理
			*			注意改成WSA_INFINITE（一直等待）后，函数返回值的延时判断要修改成 等待事件函数返回WSA_WAIT_IO_COMPLETION
			*			我就在这里卡了半天，原来是没有将延时处理的判断换成WSA_WAIT_IO_COMPLETION，导致没有连接的情况下，客户端发送消息
			*			还是继续向下执行，导致g_count不断的++ ，其它命令也不断执行，就是因为没有WSA_WAIT_IO_COMPLETION的判断
			*			就没有执行continue ， 没有过滤掉，导致一直在执行while循环
			* 
			*		2.	添加回调函数，recv的和send的
			*			//参1是错误码，为10054则说明是强制下线
			*			//参2是得到recv获取的内容的字节数，为0则说明正常下线
			*			//参3是重叠结构
			*			//参4是WSArecv的参数5（就像recv函数的参数5，指定协议缓冲区的读取方式，比如有读了不删除，读了就删等）
			* 
			*		3. WSARecv 与 WSASend 的最后一个参数，从NULL改为我们定义好的回调函数的名字，如：
			*			int nrecv = WSARecv(g_sockall[Index], &buff, 1, &count, &DwFlag, &g_olpall[Index], RecvCALL);
			* 
			*		4.	大部分分类处理都删掉，就留下PostRecv(g_count);  PostAccept();  g_count++;这几个或者打印信息
			*			或者Postsend，这部分就是延迟等待那条线路处理的，只要延迟等待，另一条线路是函数内的立即完成，
			*			立即完成那条线路也有重新投递的命令，所以立即完成、延迟等待的线路都要重新投递一次
			* 
			*		5.  下线处理就交给Recv的回调函数来分类处理，只要一下线，对应客户端的事件就会触发信号，然后马上调用回调函数
			*			然后就在回调函数中进行下线处理，
			* 
			*		最后注意我的那个问题，因为询问信号函数的参数4改变，原来的超时处理要替换成WSA_WAIT_IO_COMPLETION
			*		不然客户端发送消息，就会执行一遍延时等待那条线路的所有函数调用，就会出BUG
			*		
			* 
			*	询问信号函数参数5换成WSA_INFINITE：因为只用询问服务器的事件，那就有信号再走完全没问题
			*	参数6设置成TRUE的意义：
			*				将等待事件函数与完成例程的机制结合到一起，TRUE就是为完成例程设置的
			*				是为了能实现：等待事件函数与完成例程函数的异步执行，等执行完完成例程函数，就会给等待事件函数一个信号
			*				等待事件函数返回WSA_WAIT_IO_COMPLETION
			*				即WSAWaitForMultipleEvents不仅能获取事件的信号通知，还能获取完成例程的执行通知
			* 
			* 
			*	
			* 总结：一定要对流程熟悉，如果不能确认问题所在的话，就从确定的位置开始，逐条逐条的分析，别盲眼摸象	
			*/
		//7.4 重叠IO模型简单总结
			/*
			*	课时61：老师有一个流程图，我们对比着去看我们之前学过的流程的区别
			*	老师说重叠IO比较难，但完成端口模型有70%与重叠IO一样，学的好的话，完成端口模型就跟闹着玩一样
			*	所以好好理解重叠IO模型，完成端口是重叠IO的优化
			*	
			* 本课对代码的修改：
			*		将递归的写法修改成循环的方式，立即完成就continue，延迟处理就break
			*		循环是更好的，递归是不能层数太多的（会爆栈），循环则不用担心，
			*				一般来讲，网络模型不会有太多层递归的情况
			*		
			*		修改我们在RECV的回调函数中的获取下标的方式，对它进行优化：
			*			循环获取下标的问题：就是如果客户端数量太多，循环就会效率低下
			*			那么我们另辟捷径，因为重叠结构体的成员是数组，数组的特点是空间连续，地址连续
			*			那我们用当前元素的地址，减去首元素的地址，再转换成int，就获得了差值
			*			而这个差值经过推算，就正好是下标的数值
			*		这种优化很好，但还有更好的，在完成端口模型中讲解
			* 
			*	事件通知与完成例程的区别：
			*		事件通知的特性是将成千上万的用户 分组分线程去等待处理，然后挨个遍历询问信号
			*		完成例程，我们就监视一个服务器端的就可以了，有信号了就自动调用回调函数
			*			所以完成例程要比事件通知要更好一点
			* 
			*		事件通知是咱们自己分配任务，我们waitfor，所以顺序不能保证，循环次数多，下标越大的客户端延迟会越大
			*		完成例程是咱们写好任务处理代码，系统根据具体事件自动调用咱们的代码，自动分类
			*		完成实例调用回调函数也是在线程中调用并执行的，所以调用执行回调函数的过程也是异步的
			*			所以完成例程比事件通知的性能稍好一些
			* 
			* 之前学的所有模型包括重叠IO的问题：
			*		在一次处理过程中，客户端产生多次send，服务器会产生多次接收消息，第一次接收消息会收完所有信息
			*		就是执行一个操作的过程中，客户端连续发了好几条，那处理完之前的，处理这个客户端时，就会将连续发送的信息，整合成一条读出来
			* 
			*	CS模型：监视客户端的过程是阻塞的，执行recv， send复制的过程也是阻塞的
			*	重叠IO：等待的过程交给操作系统，是异步的，recv、send复制的过程也是由操作系统来完成（WSASend、WSARecv会交给操作系统完成），我们只用接受信号分类就好了
			*		这样对比就知道我们学习的模型的进步了
			*	
			*/
	};
}