#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

fd_set Allsockets;				//创建集合，用于装socket


BOOL WINAPI fun(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//释放所有socket
	case CTRL_CLOSE_EVENT:
		for (u_int i = 0; i < Allsockets.fd_count; i++)
			closesocket(Allsockets.fd_array[i]);
		//关闭网络库
		WSACleanup();
	}
	return TRUE;
}

int main(void)
{
	//回调函数，为了让程序结束能释放所有socket，最后一课知识点：控制台关闭事件
	SetConsoleCtrlHandler(fun, TRUE);

	//打开网络库
	WORD wdVersion = MAKEWORD(2,2);
	WSADATA wssocket;
	int wssock = WSAStartup(wdVersion, &wssocket);
	if (wssock != 0)
	{
		switch (wssock)
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
	if (2 != HIBYTE(wssocket.wVersion) || 2 != LOBYTE(wssocket.wVersion))
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//创建SOCKET
	SOCKET Serversock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Serversock == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//绑定端口和地址
	struct sockaddr_in socad;
	socad.sin_family = AF_INET;
	socad.sin_port = htons(22258);
	socad.sin_addr.S_un.S_un_b.s_b1 = 127;
	socad.sin_addr.S_un.S_un_b.s_b2 = 0;
	socad.sin_addr.S_un.S_un_b.s_b3 = 0;
	socad.sin_addr.S_un.S_un_b.s_b4 = 1;
	int  ace = bind(Serversock, (const struct sockaddr *)&socad, sizeof(socad));
	if (ace == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//打开监听
	int list = listen(Serversock, SOMAXCONN);
	if (list == SOCKET_ERROR)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//接收连接（本质：创建客户端的socket）
	struct sockaddr_in cliesock;
	int add = sizeof(cliesock);
	SOCKET clientsock = accept(Serversock, (struct sockaddr*)&cliesock, &add);
	if (clientsock == INVALID_SOCKET)
	{
		int a = WSAGetLastError();
		closesocket(Serversock);
		WSACleanup();
		return 0;
	}
	//select模型
	FD_ZERO(&Allsockets);			//清空集合
	FD_SET(Serversock,&Allsockets);	//向集合中添加一个socket，添加为本服务器的socket
	//FD_CLR(clientsock, &Allsockets);	//从集合中删除指定socket
	FD_ISSET(clientsock, &Allsockets);
	//循环执行select模型的特性
	while (1)
	{
		fd_set Readsockets = Allsockets;	//参数2，用中间变量来记录集合，否则select函数内部会改变这个集合
		fd_set writesockets = Allsockets;	//参数3
		fd_set errorsockets = Allsockets;	//参数4
		struct timeval tim;	//参数5，设置最大等待时间，时间为结构体两成员相加
		tim.tv_sec = 3;		//秒
		tim.tv_usec = 0;	//微秒
		int nREs = select(0, &Readsockets, &writesockets, &errorsockets, &tim);
		if (nREs == 0)	//没有socket发生事件
		{
			continue;	//回到循环开头继续循环
		}
		else if (nREs > 0)//socket集合有发生事件
		{
			//参数4，用来处理通信过程中的通信异常错误
			for (u_int i = 0; i < errorsockets.fd_count; i++)
			{
				char opt[1500] = {0};		//参4
				int len = sizeof(opt);		//参5
				//得到异常套接字上的具体错误码
				if (SOCKET_ERROR == getsockopt(errorsockets.fd_array[i], SOL_SOCKET, SO_ERROR, opt, &len))
				{
					int a = WSAGetLastError();
					printf("getsockopt函数本身执行错误，无法得到通信错误信息\n");
				}
				printf("通信异常错误码：%s\n", opt);
			}
			//参数3 用来处理有可写的客户端
			for (u_int i = 0; i < writesockets.fd_count; i++)
			{
				//printf("服务器%d 客户端%d\n", Serversock, writesocket.fd_array[i]);	//启动客户端就一直在无限打印，因为有连接的客户端，一直满足参数3的writesocket
				
				if (SOCKET_ERROR == send(writesockets.fd_array[i], "ok", sizeof("ok"), 0))
				{
					int a = WSAGetLastError();
				}
			}
			//参数2 用循环来处理响应的事件socket,i刚好对应着下标
			for (u_int i = 0; i < Readsockets.fd_count; i++)
			{
				//如果是服务器的事件，证明有客户端链接请求
				if (Readsockets.fd_array[i] == Serversock)
				{
					//接受链接
					SOCKET socketclient = accept(Serversock, NULL, NULL);
					//这里就不用参数2、3，如果想获取对应的客户端信息，用getpeername()和getsockname()函数即可获取
					if (socketclient == INVALID_SOCKET)	//发生错误
					{
						int a = WSAGetLastError();		//获取错误码
						continue;
					}
					//将连接的客户端的socket装到集合中
					FD_SET(socketclient, &Allsockets);
				}
				//如果是客户端的事件，证明是有信息传递
				else
				{
					//接收消息
					char sad[1500] = { 0 };
					int nrec = recv(Readsockets.fd_array[i], sad, 1499, 0);
					//如果发生错误
					if (nrec == SOCKET_ERROR)
					{
						int a = WSAGetLastError();
						//注意：我们测试时关闭客户端的窗口，属于强制下线、非正常下线，是一个错误，
						//不是这里的客户端下线
						SOCKET temclose = Readsockets.fd_array[i];	//记录这个socket
						//要用中间变量，否则清除时会将select中顶替原来位置的其它socket给剔除
						switch (a)
						{
						case 10054:
							//将该socket从集合中删掉，也从select中删掉，并且清除
							FD_CLR(Readsockets.fd_array[i], &Allsockets);//从集合中删掉
							FD_CLR(Readsockets.fd_array[i], &Readsockets);//从select中删掉
							closesocket(temclose);						//清除该socket
							break;
						}	
					}
					//如果客户端下线
					else if (nrec == 0)
					{
						//客户端下线
						//注意：我们测试时关闭客户端的窗口，属于强制下线、非正常下线，是一个错误，
						//不是这里的客户端下线
						
						//将该socket从集合中删掉，也从select中删掉，并且清除，
						//要用中间变量，否则清除时会将select中顶替原来位置的其它socket给剔除
						SOCKET temclose = Readsockets.fd_array[i];	//记录这个socket
						FD_CLR(Readsockets.fd_array[i], &Allsockets);//从集合中删掉
						FD_CLR(Readsockets.fd_array[i], &Readsockets);//从select中删掉
						closesocket(temclose);
					}
					//正常情况
					else
					{
						//打印信息
						printf("%d  %s\n", nrec, sad);
					}
				}
			}
		}
		else if (nREs == SOCKET_ERROR)	//select函数运行出错
		{
			//得到错误码
			int a = WSAGetLastError();
			//相应处理
		}
	}

	//清除socket、关闭网路库
	for (u_int i = 0; i < Allsockets.fd_count; i++)
	{
		closesocket(Allsockets.fd_array[i]);
	}
	WSACleanup();

	system("pause>0");
	return 0;
}

	//3.1 select 介绍
	/*
	* select模型特点：
	* 1.解决基本C/S模型中，傻等阻塞的问题
	*		傻等阻塞：accept 和 recv 执行时，如果没有等到链接和没有收到信息，将会一直执行这个函数，等到天荒地老
	*		执行阻塞：如send、recv、accept在执行过程中有复制粘贴，这个过程都是阻塞的（可能因为循环啥的）
	*				select解决的问题是傻等阻塞，而不是执行阻塞，也就是不是解决这几个函数执行的阻塞问题
	*		select是解决傻等阻塞的，不解决执行阻塞
	* 
	* 2.实现多个客户端链接，与多个用户分别通信
	* 
	* 3.用于服务器，客户端用不上这个（因为只有一个socket）
	*		客户端recv等时候，那不也是不能send吗？
	*		只要单独创建一根线程，recv放到线程里，就解决了
	* 
	* 
	* select逻辑：
	*		1.服务器中每个客户端都有一个socket，服务器本身也有一个socket
	*			将所有的socket装进一个数组中（数据结构）
	*		2.通过select函数，遍历数组中的socket，当某个socket有响应
	*			则将这个socket通过参数/返回值反馈出来，我们对这个返回值进行相应处理
	*		3.相应的处理：
	*			如果检测到的是服务器本身的socket，那就证明是 有客户链接要连接服务器，就调用accept
	*			如果检测到的是客户端的socket，那就是已连接的客户端请求通信，就调用send或者recv
	* 
	* select模型代码流程：
	*		1.添加网络头文件，网络库
	*		2.打开网络库
	*		3.校验版本
	*		4.创建SOCKET
	*		5.给socket绑定地址，端口号
	*		6.开始监听
	*		7.select
	* 
	*		//发现前6步，与基本C/S模型的是一样的，只是第7步以后换成了select
	*
	*/
	//3.2 fd_set意义与使用
	/*
	* fd_set:
	*		定义一个装客户端socket的数据结构
	*		fd_set是网络定义好供我们使用的
	*		转定义看下fd_set的声明，发现这个结构体有两个成员，一个记录数组中元素有效个数，另一个定义了select能处理的客户端数量（默认是64个）
	*		我们可以自定义那个宏，可以扩充，如：#define FD_SETSIZE      128
	*		但注意不要改得太大了，因为select是不停遍历去检索，太多了效率就很低下，尽量不要太大，几百个，1024就差不多了，当然大家不怕慢，可以设置更多~
	*		如果客户端数量太大，请使用更高级的网络模型，select适合用户量少的访问
	* 
	*	四个操作fd_set的参数宏：
	*			依次转定义看下逻辑
	*		FD_ZERO：将集合清零。				//转到定义，发现就是让计数的count = 0，这样就节省了资源，不必初始化，等到用了再+1覆盖即可
	*		FD_SET:向集合中添加一个socket		//转到定义，那个代码实现我们是容易看的，可以学习下如何让代码更简洁
	*		FD_CLR：集合中删除制定的socket
	*		FD_ISSET：判断一个socket是否存在于集合中（存在返回非0，不在返回0）
	*/
	//3.3 select函数的逻辑调用
	/*
	* select函数作用：
	*		监视socket的集合，当某个socket发生事件（有客户端链接或者发送信息）
	*		则会将这个socket当作返回值返回给我们
	* 
	* select函数使用：
	*		对应参数可以设置成NULL，即不使用对应参数的功能
	*		参数2是我们解决基本C/S模型，傻等阻塞的核心功能
	* 
	* 参数1：int           nfds,
	*		填0就好了，是为了兼容以前的一个版本Berkeley sockets
	*		现在好像几乎废弃了
	* 
	* 参数2： fd_set        *readfds,
	*		检查是否有可读的socket，就是有客户端的消息了，或者收到链接请求，则会被设置
	*		传进来的时候是有所有的socket集合，但是经过函数，系统会将有事件的socket发送回来给这个参数
	*		这样等到函数调用结束，这个参数就只剩下了有事件发生的socket
	*		这个参数就是select解决傻等阻塞的关键参数
	* 
	* 参数3：fd_set        *writefds,
	*		检查是否有可写的socket，即检查有链接的客户端，能send发送信息的socket
	*		一开始传进来时有所有socket的集合，等经过函数，系统会将能够进行send的socket装到这个函数中
	*		等到函数调用结束，这个参数就只剩下了能够进行send的socket
	*		其实并不实用，因为只要有客户端链接，不用这个参数也是能够直接发送的
	*		所以一般直接在函数外就send了，这个参数用的并不多
	* 
	* 参数4：fd_set        *exceptfds,
	*		检查套接字上的异常错误，比如丢包，信息不完整，等
	*		用法和参数2、3一样，将有异常的socket装到这个参数中，反馈给我们
	*		得到异常套接字上的具体错误码
	*
	* 参数5：const timeval *timeout
	*		一个结构体类型，成员分别是：秒、微秒
	*		作用是设置最大等待时间，即遍历到一个socket，到等多久没反应，才继续检索下一个socket
	*		在等待时间内，在检索的socket有事件，则立马将socket反馈，然后检索下一个socket
	*		可以设置成0，0 就是不等待，没有事件直接跳过，非阻塞状态
	*		填NULL的话，就是不使用这个参数，也就是不使用这个功能，
	*		将会造成阻塞，就是检索一个socket就一直傻等
	* 
	* 返回值：
	*		0：客户端在等待时间内没有反应，处理方式：continue就行了（跳到循环开头继续）
	*		>0:有客户端请求交流了
	*		SOCKET_ERROR:发生了错误
	*		
	*
	*/
	//3.4 select函数处理accept、recv
	/*
	* 循环开始的时候：
	*		fd_set Tempsocket = Allsockets;	//用中间变量来记录集合，否则select函数内部会改变这个集合
	* 
	* 如果是服务器的事件：
	*		需要用accept来接收连接
	*		这里就不用参数2、3，如果想获取对应的客户端信息，用getpeername()和getsockname()函数即可获取
	*		连接后，//将连接的客户端的socket装到集合中：FD_SET(socketclient, &Allsockets);
	*		第一次写成把Tempsocket.fd_array[i]装到集合里了，这是错的，因为新的socket还没在集合中
	* 
	* 如果是客户端的事件：
	*		注意：我们测试时关闭客户端的窗口，属于强制下线、非正常下线，是一个错误，不是正常客户端下线，错误码是10054
	*		发生错误，或者客户端正常下线，要将该socket从集合中删掉，也从select中删掉，并且清除，
	*		要用中间变量，否则清除时会将select中顶替原来位置的其它socket给剔除
	*		
	*/
	//3.5 select处理客户端正常下线
	/*
	* 客户端正常下线：
	*		客户端那头，要调用WSACleanup();才是正常关闭
	*		所以我们在客户端做一点改进：单独输入0的时候就调用WSACleanup();
	*		就能正常关闭了
	* 错误处理：
	*		可能要重启啥的
	*		我们10054就清除对应的socket即可
	* 学习：
	*		如fd_set的实现（F12）我们就可以很好的学习
	*		老师说国际约定（规定）的，都是好东西，值得学习
	*/
	//3.6 select 处理参数3，可写的socket集合
	/*
	* send函数返回值：
	*		只有两种返回值，成功返回发送的字节数
	*		失败返回SOCKET_ERROR，如果客户端正常关闭（客户端调用WSACleanup），也是返回SOCKET_ERROR
	*		网上有些人就理所当然地认为有三种返回值，认为正常关闭返回0，其实是错的
	*		所以我们上网搜资料，要考虑到真实性，要经过自己验证
	* 
	* 参数3：writesocket
	*		参数3的集合中可以删除掉服务器本身的socket
	*		不删也没关系，因为服务器对自己是无法发送（可写）的（测试过，writesocket中没有返回服务器的socket）
	*		
	*/
	//3.7 select 参数4 异常处理讲解
	/*
	* 参数4：fd_set        *exceptfds,
	*		检查套接字上的异常错误，比如丢包，信息不完整，等
	*		这个不好测试，因为需要传输过程中出现丢包情况
	*		
	* 函数：
	*		getsockopt （socket，SOL_SOCKET，  SO_ERROR, buf , buflen）
	*		参数1：要进行检测的socket，
	*		参数2：SOL_SOCKET ，表示参数1是一个socket（大概是这样的功能）
	*		参数3: 得到通信过程中的异常错误的错误码（上MSDN上查看，好像还有一些）
	*		参数4：就是用来接收通信异常错误的错误码的 字符串数组
	*		参数5：参数4的字节大小
	*		返回值：这个函数如果运行错误，返回SOCKET_ERROR
	* 注意返回值并不是通信的错误码，而是这个函数本身是否成功的讯息
	*/
	//3.8 select 模型总结
	/*
	* 代码结构：
	*		select模型的代码，也会有细节的优化，结构稍有变化
	*		网上找的代码，与我们的代码肯定会有不一样的地方
	*		但是核心的逻辑，原理都是一样的
	*		我们写的这个代码是思路最简单的方法，方便理解模型
	* 
	* 结构核心：
	*		就是参数2，解决了accept，recv 傻等阻塞的问题
	*		这就是select对比基本模型最大的作用
	*		网上的一些代码，都不讲参数3，参数4，都填了NULL
	*		参数3可有可无，因为随时都能够sand，不一定要检测出来才能send
	* 
	* 流程总结：
	*		将所有socket装在一个集合中
	*		这个集合用select来遍历检索，检索到有事件响应的，则将有响应的socket单独装到一块，然后挨个来处理
	*		返回值 == 0：没有检索到有响应的socket
	*		返回值 >0 检索到了有响应的socket
	*		返回值 == SOCKET_ERROR select函数执行失败
	*		有响应的根据参数，有：可读，可写，异常
	*		可读的socket：服务器的响应处理accept（客户端链接），客户端的响应处理recv接收客户端信息
	* 
	* select是阻塞的：
	*		select是执行阻塞，就是执行accept、recv、send时，缓冲区数据的复制粘贴过程中，无法进行其它任务的执行
	*		这样的话，当函数调用次数一多，则会放大延迟，变得很慢
	*		参数5为 0 0 不等待，是执行阻塞
	*		参数5为正常数值，是执行阻塞+软阻塞（要等待一定时间才检索下一个socket）
	*		参数5为NULL，是执行阻塞+硬阻塞（死等，要等到有响应为止，才检索下一个socket，与傻等阻塞有点类似，但不一样）
	* 
	*网络模型学习：
	*		我们就是一步一步来，看哪种模型有哪种缺点
	*		然后用什么模型来解决这种缺点，最后全部整合起来就是一个完整的完全端口模型了	
	*/
	//3.9 控制台关闭事件
	/*
	* 学习思想：
	*		老师说不要局限于代码层面，更重要的是编程的思想，比如select解决了傻子阻塞的问题
	*		思想上是怎么解决的，这些思想才是我们工作学习要积累的，也是更好提升自己的方法
	*		比如多线程比较好的解决了执行阻塞的问题，但不是完全解决
	* 
	* 释放所有socket、关闭网路库：
	*		程序结束时要清除socket，但是select模型一直在死循环中
	*		这时候要清理socket的话，就得在循环中的错误处理里break
	*		当执行某个错误，比如我们关闭服务器窗口的时候，会有错误（强制下线）
	*		我们用这个错误来break，然后就能执行清除socket、关闭网路库了
	*		或者自己定义一个符合退出的条件，然后执行break
	*		
	* 或者：
	*		我们用钩子（老师说涉及到灰色地带，不愿意多讲）
	*		就是用操作系统来完成，操作系统监控着我们一切的窗口操作
	*		利用回调函数（我们写好的函数，让系统去调用，就是回调函数）
	*		当我们关闭服务器端口时，就调用我们写好的函数，
	*		这个函数内放上我们写好的释放所有socket，这样就行了
	* 
	* 回调函数：
	*		SetConsoleCtrlHandler(fun, TRUE);
	*		上面代码的意思：操作系统先调用自己的默认函数，然后根据TRUE还是FALSE来决定是否调用我们自己定义的函数fun
	*		TRUE则是操作系统调用完自己的默认函数，就会执行我们自己写的函数fun，FALSE则不执行我们的自写函数
	*		自写的函数，我们就写上释放所有socket
	*	自写函数有固定的格式：BOOL WINAPI fun(DWORD dwCtrlType);
	*	CTRL_CLOSE_EVENT是关闭窗口的操作执行了就会有这个返回值
	* 
	*	BOOL WINAPI fun(DWORD dwCtrlType)
	*	{
	*		switch (dwCtrlType)
	*		{
	*		case CTRL_CLOSE_EVENT:
	*			for (u_int i = 0; i < Allsockets.fd_count; i++)
	*				closesocket(Allsockets.fd_array[i]);
	*		}
	*		return TRUE;
	*	}
	*		
	*	
	*/