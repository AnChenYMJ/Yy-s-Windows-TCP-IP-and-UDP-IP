#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//创建socket放到全局，方便点×退出函数调用，注意主函数里不要再写声明了，会有二义性
SOCKET SerVerSocket;

BOOL WINAPI HandlerRoutine(DWORD dwType)
{
	switch (dwType)
	{
		case CTRL_CLOSE_EVENT:
			closesocket(SerVerSocket);
			WSACleanup();
			break;
	}
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

	//接收与发送
	while (1)
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

	//释放socket、关闭网络库
	closesocket(SerVerSocket);
	WSACleanup();

	system("pause>0");
	return 0;
}


void studybook11(void)
{
	//2.1 服务器卡开网络库
	{
		/*
			这节课的内容与TCP/IP的打开网络库的内容完全一样
				过多的笔记我就不写了，可以看课件或者TCP的笔记

			我知识的遗漏点：
				1.WORD F12得到它的类型是unsigned short类型，2字节，16位
				高数据位也就是高地址位，存放副版本，低地址位低数据位存放主版本
				MAKEWORD这个宏的功能就是将版本号分开存到高低地址位中，不一定是非要用在打开网络库中

				2.WSADATA wsaData;参数2用来接住打开函数的一些信息，包括当前使用版本号，系统支持的最高版本，
				还有3个已经在2.0以上版本弃用了的信息，保留它是为了兼容低版本，为了兼容性

				3.静态库的写法是固定好的，且ws2_32.lib 的32没有64的写法，不论是32位还是64位的编译环境都写32

				4.打开网络库函数的错误码其实常见的就那几个，进行错误处理，在菜单栏工具有查看错误码是啥的（错误查找）
					看MSDN去找错误码有哪些
		*/
	};
	//2.2 校验版本、socket、bind
	{
		/*
			都与TCP的一样，详情看课件或者TCP的笔记

			查缺补漏：
					地址是找电脑的，端口是找电脑上应用程序的，
					0~1023的端口是给系统使用的，我们不能占用，49152~65535是系统动态端口号（系统要使用时就会占用）所以动态端口号一般不建议使用

					sockaddr是将端口号与地址绑定在一起，所以我们不容易去输入，用sockaddr_in就方便输入多了
					最后将结构体再强转成sockaddr即可，强转时注意大小写，小写要加上struct，大写就像一个宏，不用加struct

					创建socket的参数分别为：地址类型、套接字类型、协议类型（填0则自己判断用哪个协议）
		
		*/
	};
	//2.3 recvfrom接收消息函数详解
	{
		/*
			函数原型：
					int recvfrom(
					  SOCKET   s,
					  char     *buf,
					  int      len,
					  int      flags,
					  sockaddr *from,
					  int      *fromlen
					);

			作用：
				得到当前服务器接收到的消息
			本质：
				复制，将协议缓冲区的数据报复制黏贴进咱们的buf
			阻塞的：
				跟recv一样，不同的是recv是傻等，recvfrom死等（死等与傻等不一样）

			参数1：
				服务器端的socket
				特点：
					TCP是获取指定的，recv函数与客户端是1对1的关系，傻等
					UDP是无差别获取，recvfrom函数与客户端是1对多的关系，没有傻等
				就是说：	recv的参数1是目标的socket，每个recv都只等待接收那个目标socket的消息，别的socket是不管的
						recvfrom是本身的socket，就是等待接收所有发过来的消息，不指定为哪一个socket，无差别，谁发都收
			参数2：
				客户端消息的存储空间，也就是个字符数组
				字符数组的大小多少合适：
					局域网下
						数据链路层以太网协议的MTU（最大网络传输单元）1500字节
							tcp=1500-20(tcp包头)-20(IP包头)=1460    1420
							udp=1500-8(udp包头)-20(IP包头)=1472
					广域网
						各级路由器上的MTU最小值是576字节
							tcp=576-20(tcp包头)-20(IP包头)=536
							udp=576-8(udp包头)-20(IP包头)=548
					如果包大了
						协议底层会剪碎发送，接收方组合拼接
							1、通信效率会下降
							2、对于TCP影响不大，毕竟是可靠的，1460或者536均可
							3、UDP剪碎大概率会导致丢包，所以不要超过这个标准MTU大小，即548
			参数3：
				想要读取得字节个数
				一般是参数2得字节数
				注意
					假设我的一个数据报发来20字节，咱们这里写个10，那么，buf里就装前10个字节的数据而且只读一次
						后10个就丢弃了
					对于tcp/ip的recv函数，放在循环里，读10个之后，会再次读一次，把下10个读出来
			参数4：
				数据的读取方式
					填 0
						从协议缓冲区取一个数据报，然后就删除掉
					MSG_PEEK
						取出数据报，但是协议缓冲内不删除，一直残留，导致影响后面的数据读取
					MSG_OOB
						带外数据，尽量不用了
			参数5：
				用于接收记录对方的IP地址端口号，此处结构体为相应客户端的IP地址和端口号
			参数6：
				参数５结构体的大小
			返回值：
				成功：返回读到的字节数
				失败：返回SOCKET_ERROR
			注意：
				1.tcp中，当客户端下线，这边会收到0，不过UDP没有这个，UDP是面向非链接的，客户端如何与我无关，发了就收，不发我就等。
				2.此函数是阻塞、同步的：在recvfrom函数卡着，等着客户端发来数据
				3.参数2的大小要满足数据链路层以太网协议的MTU（最大网络传输单元），局域网中加上包头不大于1500，广域网不大于576
					IP协议的包头20字节是固定的，TCP的包头能拓展最大60字节，UDP8字节
					UDP包太大，协议底层会剪碎发送，但是UDP的包不能再与别的包结合（不可靠），会导致丢包，降低通信质量
				4.参数3，想要读取的字节数不要写小了，不然没读到的部分就丢了，再也不能读取，因为recvfrom一个socket的send只读一次
					不像TCP的能多次读取，因为TCP的是字节流，能够任意截断再组合
	
		
		*/
	};
	//2.4 sendto发送信息函数
	{
		/*
			函数原型：
					int sendto(
					  SOCKET         s,
					  const char     *buf,
					  int            len,
					  int            flags,
					  const sockaddr *to,
					  int            tolen
					);
			作用：
				向目标发送数据报
			本质：
				sendto函数将我们要发送的数据复制到协议缓冲区，计算机伺机发送出去（伺机：等待合适的时机，比如当前有别的recv或send在执行，则要等一下，很快的）

			参数1：
				自己的socket
			参数2：
				给对方发送的字节串（发送大小要遵守 数据链路层以太网协议的MTU（最大网络传输单元））
				为了方便统一，我们统一使用548，这样就不必计算TCP或者UDP或者局域网、广域网了
			参数3：
				发送的数据的字节个数，最大548
			参数4：
				写0就行了
				其他：
					MSG_OOB：带外数据，不建议使用了
					MSG_DONTROUTE：指定数据不应受路由限制。 Windows套接字服务提供程序可以选择忽略此标志。
			参数5：
				对方（接收方）的端口号与IP地址的结构体
				本课时代码使用了recvfrom的结构体，因为recvfrom的结构体记录了发信息的客户端的地址与端口号，我们拿来用就好，谁发了，我们就回一个信息
			参数6：
				参数5的字节大小
			返回值：
				成功：返回写入的字节数
				失败：返回SOCKET_ERROR

			总结：
				大部分参数与recvfrom的参数是很相似的，可以对比看一下细节
				send的参数5是用const修饰的，因为这个结构体不是用来接收函数传递的信息，而是用它来标记发给谁
				发送的字符串与以后工作要发送的文件其实是一样的，不同的是文件需要用到文件操作函数（比如fwrite）来操作，但本质一样


		*/
	};
	//2.5 客户端与服务器通信
	{
		/*
			客户端流程：
					1.添加网络库、网络头文件
					2.打开网络库
					3.校验版本
					4.创建SOCKET
					5.创建服务器端口号、IP地址的结构体
					6.与服务器收发消息
			服务器流程：
					1.添加网络库、网络头文件
					2.打开网络库
					3.校验版本
					4.创建SOCKET
					5.绑定端口号、IP地址
					6.与客户端收发消息

			总结：
				1.客户端不需要绑定端口号与地址，而是要与服务器连接
					所以服务器要有一个固定的端口号与地址（所以要绑定），否则客户端不知道连哪一个

				2.创建新工程时，有一个选项为：安全开发生命周期警告，
					如果勾上了，那我们使用scanf等函数会有报错警告，要我们处理：比如加上宏，或者使用scanf_s
					scanf_s是微软编译器自定的更安全的函数，并非C++原有的，所以为了移植性，我们还是使用scanf更好

				3.本课的客户端循环里先发送，这样服务器收到就发送一个回馈信息，然后客户端收到后就能继续发送了
					其实没啥，就是怕以后的我不知道为啥这样的顺序

				4.对比TCP发现UDP的C/S模型相当简单，因为包头信息太少，控制的信息不多，不复杂

				5.sendto的参数5要用const修饰，否则有警告，要加上(const struct sockaddr*)

				6.结尾要：释放socket、关闭网络库
		*/
	};
	//2.6 优化：点×退出，总结分析
	{
		/*
			优化：点×退出
				程序结束时要释放socket与关闭网络库，socket是系统内核对象，不手动关闭的话会一直存在导致内核空间泄露，只有重启能解决
				而我们打开客户端或服务器，控制台右上角点×关闭了，则是强制退出，这是不会执行释放socket与关闭网络库的
				所以我们要加上点叉退出的函数，这个函数是系统提供的API，我们设定为点×就自动调用释放、关闭网络库函数
				这个函数很有用，以后工作会常用到，一定要会
				首先：主函数投递一个监视，SetConsoleCtrlHandler(HandlerRoutine, TRUE);
					参数1为处理函数
				然后，全局区创建处理函数，系统监视到击了控制台的关闭按钮（×）则会调用处理函数
					断点调试能看到执行了处理函数
			UDP C/S模型总结分析：
				1.UDP不存在链接，所以不存在是上下线
				2.recvfrom函数是阻塞的，不是傻等，是可以处理多个客户端请求的（谁发来都收）
				3.recvfrom，sendto是同步的：即复制黏贴的过程是阻塞的，当前复制黏贴完了，其他的才能复制黏贴
					所以，如果像我们这种单线程处理多个客户端，就会出现延迟的情况，客户端越多，延迟越大。
					可以采用多线程解决，多线程最好的解决方案就是完成端口，而且可以高效处理并发
				4.UDP对比TCP：
					tcp/ip基本c/s模型中，recv，send也是阻塞的，但是recv是傻等问题，即recv只接收指定客户端的消息
						所以，我们学习了第一个模型，select模型，解决了傻等的问题（select是让一个函数遍历，有信号产生的socket返回到参数的socket数组中）
							但是select本身执行过程是同步的
						然后接下来，eventselect，asyncselect模型对select过程进行异步，即获取客户端通知的过程进行了异步，释放了主线程，释放了cpu（等信号的过程主线程是挂起的）
							就是让获取通知（产生信号）的过程投递给系统，让系统来监视，我们等着分类处理就好了
						最后是重叠IO与完成端口对整个过程进行异步，并有效的处理并发
							重叠IO的问题是，有一个任务就开一条线程去处理，线程数量太多就会导致性能下降，完成端口就解决了这个问题
					UDP/IP适用模型
						所以宏观上，select模型对其，没啥作用，UDP/IP基本模型自身就解决了傻等问题
						接下来eventselect，asyncselect这两个模型，把阻塞状态的select解放出来，从在解决了死等的问题。
						接下来学习并发模型，重叠IO与完成端口处理并发非常重要
		*/
	};
}