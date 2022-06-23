#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

//自定义消息编号
#define MY_MSG_ID WM_USER+1
//服务器socket
SOCKET SerVerSocket;
//回调函数
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam);
//窗口每次打印的间隔
int n;

//参1：实例句柄(就是要创建窗口的ID) 、参2：保留的（已弃用，为了向下兼容性才留的，另一个窗口的句柄，就是打开两个窗口，会把参1的内容复制到新窗口）
//参3：向命令行传参数、参4：显示方式（比如打开程序是最小化状态这种）
//转定义WINAPI 发现与CALLBACK等一样都是 __stdcall的宏
int WINAPI WinMain(_In_ HINSTANCE hinstance, _In_opt_ HINSTANCE hprehinstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
	//1.创建窗口结构体 WNDCLASSEX
	WNDCLASSEX woYy;					//窗口结构体
	woYy.cbClsExtra = 0;				//结构体拓展大小（可拓展）结构体额外的空间
	woYy.cbSize = sizeof(WNDCLASSEX);	//结构体大小
	woYy.cbWndExtra = 0;				//窗口拓展大小，窗口额外的空间
	woYy.hbrBackground = NULL;			//背景颜色，NULL为默认
	woYy.hCursor = NULL;				//光标样式
	woYy.hIcon = NULL;					//左上角图标
	woYy.hIconSm = NULL;				//任务栏图标
	woYy.hInstance = hinstance;			//主函数参数1，主程序的句柄
	woYy.lpfnWndProc = WinbackProc;		//回调函数
	woYy.lpszClassName = "Yyasyncselect";//这个类（结构体）的名字
	woYy.lpszMenuName = NULL;			//菜单栏名字
	woYy.style = CS_HREDRAW | CS_VREDRAW;//窗口风格：水平刷新与垂直刷新（当窗口有水平变化时，要重绘窗口，垂直也一样）
	//2.注册窗口结构体 RegisterClassEx
	RegisterClassEx(&woYy);				//注册了系统才能知道它
	//3.创建窗口		CreateWindowEx
	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, "Yyasyncselect", "Yy窗口", WS_OVERLAPPEDWINDOW, 200, 200, 700, 500, NULL, NULL, hinstance, NULL);
	//4.显示窗口		ShowWindow
	ShowWindow(hwnd, nShowCmd);
	//5.更新窗口
	UpdateWindow(hwnd);

	//网络通信代码
	//打开网络库
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nrse = WSAStartup(WSVersion, &wsaData);
	if (0 != nrse)
	{
		int a = WSAGetLastError();
		printf("WSAStratup error: %d\n", a);
		CloseHandle(hwnd);
		system("pause>0");
		return 0;
	}
	//校验版本
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("网络库版本不正确 error: %d\n", a);
		CloseHandle(hwnd);
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
		CloseHandle(hwnd);
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
		CloseHandle(hwnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//绑定消息与socket并且投递给操作系统
	int wsaasync = WSAAsyncSelect(SerVerSocket, hwnd, MY_MSG_ID, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaasync)
	{
		int a = WSAGetLastError();
		printf("绑定消息函数 error: %d\n", a);
		CloseHandle(hwnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
		return 0;
	}

	//6.消息循环		循环中不断从消息队列取消息，用GetMessage来取消息，关闭窗口返回0
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))//参1：消息结构体、参2：需要接收的消息的对应窗口（填NULL，则全部窗口都能接收到）参3、4：接收的范围（填0，则是所有范围）
	{
		TranslateMessage(&msg);//取到消息后对消息进行翻译
		DispatchMessage(&msg); //分类分发，回调函数就在此函数中被调用
	}
		

	//释放与清除
	//CloseHandle(hwnd);
	//ReleaseDC(hwnd);
	closesocket(SerVerSocket);
	WSACleanup();
	
	return 0;
}

//6.回调函数		参数4（末尾参数）与创建窗口函数的末尾参数是连通的，如果那边有写回调函数，则会将数据传送到这里
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
{	//回调函数在DispatchMessage中被调用执行
	switch (msgID)
	{
	case MY_MSG_ID:
	{
		//获取socket
		SOCKET sock = (SOCKET)wparam;
	
		//lparam的高位存放错误码，为0则没有错误
		if (0 == HIWORD(lparam))
		{
			//获取这个消息所在窗口的客户区句柄（ID），在下面需要释放，用完就要释放
			HDC hdc = GetDC(hwnd);
			//消息类型分类处理
			if (FD_WRITE == LOWORD(lparam))
			{
				TextOut(hdc, 0, n, "FD_WRITE", strlen("FD_WRITE"));
				n += 20;
			}
			if (FD_READ == LOWORD(lparam))
			{	//lparam的低位存放具体消息类型
				
				struct sockaddr saClient;
				int nlen = sizeof(saClient);
				char strbuf[548] = { 0 };
				int num = recvfrom(sock, strbuf, 548, 0, &saClient, &nlen);
				if (SOCKET_ERROR == num)
				{
					int a = WSAGetLastError();
					//将错误码a,用itoa转换成字符串，然后存在str中，再打印到窗口上，参数3的10 表示10进制的转换
					char str[20] = { 0 };
					TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
					n += 20;
				}
				TextOut(hdc, 0, n, strbuf, strlen(strbuf));
				n += 20;
				//sendto发送信息
				int snum = sendto(sock, "Server 收到\n", strlen("Server 收到\n"), 0, (const struct sockaddr*)&saClient, sizeof(saClient));
				if (SOCKET_ERROR == snum)
				{
					int a = WSAGetLastError();
					char str[20] = { 0 };
					TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
					n += 20;
				}
			}
			//释放HDC（客户区句柄）,方便下次进入回调函数使用，因为每次只执行一个
			ReleaseDC(hwnd, hdc);
		}	
	}
		break;
	case WM_DESTROY:			//这个类似于点×操作，之前TCP没加
		PostQuitMessage(0);		//这个函数就是我们点×后就关闭程序的作用,就是退出消息循环
		break;
	}
	//这一步需要加上，否则窗口无法显示
	return DefWindowProc(hwnd, msgID, wparam, lparam);
}




//学习笔记
void studybook14()
{
	//5.1 自建窗口主函数
	{
		/*
			Win32异步选择模型自建窗口流程：
				1.创建窗口结构体 WNDCLASSEX
				2.注册窗口结构体 RegisterClassEx
				3.创建窗口		CreateWindowEx
				4.显示窗口		ShowWindow
				5.消息循环		GetMessage、TranslateMessage、DispatchMessage
				6.回调函数

			主函数：
				参1：实例句柄(就是要创建窗口的ID) 、参2：保留的（已弃用，为了向下兼容性才留的，另一个窗口的句柄，就是打开两个窗口，会把参1的内容复制到新窗口）
				参3：向命令行传参数、参4：显示方式（比如打开程序是最小化状态这种）
				转定义WINAPI 发现与CALLBACK等一样都是 __stdcall的宏

				WinMain大小写严格，它底层本质还是main，但是调用main的时候传递了一些参数，外表用WinMain来表示

			实例句柄：
				我们桌面能看到的都有句柄（ID）

		
		*/
	}
	//5.2 自建窗口
	{
		/*
			字符集的问题：
				四种方式：
					1.项目-属性-字符集 然后修改
					2.字符串前加 L 
					3.字符串前加 _T  需要头文件 tchar.h
					4.字符串前加 TEXT

			总结：

				回调函数中要加点×退出，否则我们关闭窗口，但是程序依然在运行
				消息循环是从消息队列拿消息，然后分发
				创建窗口的参数1，是风格，可以多个风格，用按位或 | 连起来就可以
				警告	C28251	“WinMain”的批注不一致: 此实例包含 无批注。在主函数参数前加_In_ 第二个不一样，但是还是没解决


				函数的参数太多了，详情看TCP的异步选择的笔记
		*/
	};
	//5.3 异步选择模型分类处理
	{
		/*
			上节课知识点补充：
				1.循环中不断从消息队列取消息，用GetMessage来取消息，关闭窗口返回0
				2.TranslateMessage函数：取到消息后对消息进行翻译
				3.回调函数在DispatchMessage中被调用执行
				4.windows.h 与 winsock2.h 有冲突，删掉windows.h

			本节课知识点：
				1.在我们创建好的窗口结构中添加UDP的通信代码，添加的位置在消息循环前，创建窗口后（产生窗口句柄后）
				2.绑定消息与事件时，系统占用的范围是WM_USER,我们要使用，就WM_USER+1 WM_USER以后就是我们程序员使用的范围了
				3.回调函数的参数WPARAM wparam，是用来接收产生消息的事件的SOCKET
				4.回调函数的参数LPARAM lparam，分为高低位，高位存储错误码，低位存储对应的消息种类，如FD_READ等，
					用HIWORD 与 LOWORD来取，因为LPARAM是int，4字节，之前学的HIBYTE对应short 2字节
				5.经验：变量往往放到 == 的后面，防止出错
				6.printf是控制台打印，TextOut是窗口打印
				7.程序运行后，不断在消息循环中获取消息，当我们关闭窗口时，就会退出循环，所以我们要在主函数结尾释放句柄、socket、关闭网络库
				8.窗口有分客户区（我们打印的矩形区域）、非客户区等，且不同的区，哪怕再小的区都有自己的句柄（ID），ID让系统知道谁是谁，在哪个ID上操作
				9.GetDC获取句柄，DeleaseDC释放句柄，获取了就必须释放，否则有问题
				10.标签中（switch）中不能声明变量，解决方法：1.switch中用{}，在{}中声明变量，2.在switch外（上方）声明变量
				11._itoa(a, str, 10);用这个函数将错误码a int类型 10进制转换成字符串存放到str字符串数组中
				12.TextOut 窗口打印会将字符串的结尾的\0也打印出来，所以末尾参数用sizeof(a-1),或用strlen即可
				13.句柄（ID）分类中，分WRITE与REDA

			绑定事件与socket并且投递出去:
				函数原型：
					int WSAAsyncSelect(
						  SOCKET s,
						  HWND   hWnd,
						  u_int  wMsg,
						  long   lEvent
						);
				功能：
					将socket与消息绑定在一起，并投递给操作系统监视
				参数1：
					socket
				参数2：
					窗口句柄（要绑定到哪个窗口上）
				参数3：
					消息编号，系统占用的范围是WM_USER,我们要使用，就WM_USER+1 WM_USER以后就是我们程序员使用的范围了
					本课自定义一个宏(MY_MSG_ID)来记录我们要用的编号
				参数4：
					消息类型，如FD_READ等，与跟WSASelectEvent一模一样（详情看课件）
				返回值：
					成功：返回0
					失败：返回SOCKET_ERROR
		
		*/
	};
}
