//#include <Windows.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")



//自定义消息
#define UM_ASYNCSELECTMSG WM_USER+1

//回调函数声明
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam);

//定义一个数组和计数，方便程序结束释放socket
#define MAX_SOCK_COUNT 1024			//自定义的宏
SOCKET g_Socketall[MAX_SOCK_COUNT];
int g_count = 0;

//定义一个客户区Y轴坐标
int y = 0;

//参1：实例句柄(就是要创建窗口的ID) 、参2：保留的（已弃用，为了向下兼容性才留的，另一个窗口的句柄，就是打开两个窗口，会把参1的内容复制到新窗口）
//参3：向命令行传参数、参4：显示方式（比如打开程序是最小化状态这种）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hpreInstance, LPSTR lpCmdLine, int ShowCmd)
{
	//创建窗口结构体
	WNDCLASSEX WD;						//比较新版本的窗口结构体是带EX的
	WD.cbClsExtra = 0;					//窗口结构体额外的空间、带外数据
	WD.cbSize = sizeof(WNDCLASSEX);		//结构体大小
	WD.cbWndExtra = 0;					//窗口额外的空间、带外数据
	WD.hbrBackground = NULL;			//背景颜色，NULL为默认
	WD.hCursor = NULL;					//光标，NULL为默认
	WD.hIcon = NULL;					//左上角图标
	WD.hIconSm = NULL;					//任务栏的图标
	WD.hInstance = hInstance;			//主函数参数1，句柄
	WD.lpfnWndProc = WinbackProc;		//填回调函数的名字
	WD.lpszClassName = "Yywindow";		//类的名字
	WD.lpszMenuName = NULL;		//菜单栏名字
	WD.style = CS_HREDRAW | CS_VREDRAW;	//窗口风格,垂直刷新与水平刷新（比如窗口大小我们改变，内容也要跟着变，详情以后再说）

	//注册结构体
	RegisterClassEx(&WD);
	//创建窗口
	HWND hwnd = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, TEXT("Yywindow"), _T("Yy窗口"), WS_OVERLAPPEDWINDOW, 200, 200, 400, 300, NULL, NULL, hInstance, WinbackProc);
	if (NULL == hwnd)
	{
		return 0;
	}
	//显示窗口
	ShowWindow(hwnd, ShowCmd);
	//更新窗口
	UpdateWindow(hwnd);


	//打开网络库
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA WSsockMsg;				//接住一些反馈信息
	int nres = WSAStartup(WSVersion, &WSsockMsg);
	if (0 != nres)
	{
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
	if (2 != HIBYTE(WSsockMsg.wVersion) || 2 != LOBYTE(WSsockMsg.wVersion))
	{
		printf("网络库版本不对");
		WSACleanup();
		return 0;
	}
	//创建SOCKET
	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == ServerSocket)
	{
		int a = WSAGetLastError();
		WSACleanup();
		return 0;
	}
	//SOCKET绑定端口号与地址
	struct sockaddr_in st;
	st.sin_family = AF_INET;
	st.sin_port = htons(22258);
	st.sin_addr.S_un.S_un_b.s_b1 = 127;
	st.sin_addr.S_un.S_un_b.s_b2 = 0;
	st.sin_addr.S_un.S_un_b.s_b3 = 0;
	st.sin_addr.S_un.S_un_b.s_b4 = 1;
	if (SOCKET_ERROR == bind(ServerSocket, (const struct sockaddr*)&st, sizeof(st)))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}
	//开启监听
	if (listen(ServerSocket, SOMAXCONN))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}
	//将服务器socket装到数组中，方便程序结束释放
	g_Socketall[g_count] = ServerSocket;
	g_count++;

	//将socket与消息绑定，投递给消息队列
	if (SOCKET_ERROR == WSAAsyncSelect(ServerSocket, hwnd, UM_ASYNCSELECTMSG, FD_ACCEPT | FD_WRITE | FD_READ))
	{
		int a = WSAGetLastError();
		closesocket(ServerSocket);
		WSACleanup();
		return 0;
	}	
	//消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//关闭网络库
	WSACleanup();


	return 0;
}

//回调函数
LRESULT CALLBACK WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
{
	//获取这个窗口的HDC（客户区句柄：ID），在下面需要释放句柄
	HDC hdc = GetDC(hwnd);
	//分类处理
	switch (msgID)
	{
	case UM_ASYNCSELECTMSG:	//经过消息循环，将消息投递到这里，然后就通过case来分类处理了
		//MessageBox(NULL, L"有信号啦", TEXT("提示"), MB_YESNO);
		{
			//获取SOCKET
			SOCKET sock = (SOCKET)wparam;
			//获取消息 
			if (0 != HIWORD(lparam))		//错误码
			{
				if (10053 == HIWORD(lparam))
				{
					TextOut(hdc, 0, y, TEXT("FD_CLOSE"), strlen("FD_CLOSE"));
					y += 15;
					//关闭socket上的消息
					if (SOCKET_ERROR == WSAAsyncSelect(sock, hwnd, 0, 0))
					{
						int a = WSAGetLastError();
					}
					//从数组中删掉这个socket
					for (int i = 0; i < g_count; i++)
					{
						if (sock == g_Socketall[i])
						{
							g_Socketall[i] = g_Socketall[g_count];
							g_count--;
						}
					}
					//释放socket
					closesocket(sock);
				}
				break;
			}
				
			//分类处理
			switch (LOWORD(lparam))			//操作码		//收到了操作码，则开始分类处理
			{
			//有客户端连接
			case FD_ACCEPT:					
			{
				//在窗口客户区打印FD_ACCEPT
				TextOut(hdc, 0, y, "FD_ACCEPT", strlen("FD_ACCEPT"));
				y += 15;
				//接受连接并创建客户端socket
				SOCKET ClientSocket = accept(sock, NULL, NULL);
				if (INVALID_SOCKET == ClientSocket)
				{
					int a = WSAGetLastError();
					break;						
				}
				//客户端socket绑定消息，投递给消息队列
				if (SOCKET_ERROR == WSAAsyncSelect(ClientSocket, hwnd, UM_ASYNCSELECTMSG,FD_WRITE | FD_READ | FD_CLOSE))
				{
					int a = WSAGetLastError();
					closesocket(ClientSocket);
					break;
				}
				//将客户端socket装到数组中，方便程序结束释放
				g_Socketall[g_count] = ClientSocket;
				g_count++;
			}	
				break;
			case FD_WRITE:
			{
				TextOut(hdc, 0, y, TEXT("FD_WRITE"), strlen("FD_WRITE"));
				y += 15;
				//代表可以对客户端进行send了
			}
				break;
			case FD_READ:
			{
				TextOut(hdc, 0, y, TEXT("FD_READ"), strlen("FD_READ"));
				y += 15;
				//FD_READ消息表示客户端发送了信息
				char buf[1024] = {0};
				if (SOCKET_ERROR == recv(sock, buf, 1023, 0))
				{
					int a = WSAGetLastError();
					break;
				}
				TextOut(hdc, 80, y - 15, buf, strlen(buf));	
			}
				break;
			case FD_CLOSE:
				TextOut(hdc, 0, y, TEXT("FD_CLOSE"), strlen("FD_CLOSE"));
				y += 15;
				//关闭socket上的消息
				if (SOCKET_ERROR == WSAAsyncSelect(sock, hwnd, 0, 0))
				{
					int a = WSAGetLastError();
				}
				//从数组中删掉这个socket
				for (int i = 0; i < g_count; i++)
				{
					if (sock == g_Socketall[i])
					{
						g_Socketall[i] = g_Socketall[g_count];
						g_count--;
					}
				}
				//释放socket
				closesocket(sock);
				;
			}
		}
		break;
	case WM_CREATE:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	

	ReleaseDC(hwnd, hdc);

	return DefWindowProc(hwnd, msgID, wparam, lparam);
}


void kejian()
{
	{
		//5.1 创建窗口结构体
		/*
		* 内容：
		*		没啥内容，就是创建窗口结构体
		*		我希望暂时我先不用记住这些内容，太琐碎了，
		*		老师说异步选择模型很简单，我就暂时不记住这些类成员是啥了
		*		WD.lpszClassName = L"Yywindow"这一部L是字符集转换，不加L有提示，char * 转换为 wchar_t *。
		*		第一种加L 第二种：右击项目，属性，选择字符集（不推荐，尽量代码层面解决）第三种：TEXT("") 第四种：_T("")
		* 核心：消息队列
		*		操作系统为每一个窗口创建一个消息队列并且维护
		*		所以我们要使用消息队列就要创建一个窗口
		* 
		* 第1步：
		*		将我们的socket与一个消息绑定，投递给操作做系统
		* 第2步：
		*		消息分类处理，当有对应的操作，就会产生消息
		* 老师说很简单：
		*		但该模型只能用于windows
		*		不过我们可以学到这种处理思想
		*
		*/
		//5.2 win32创建窗口
		/*
		*	创建窗口结构体用了EX版本，那注册结构体，创建窗口都要用EX版本，是配套使用的、
		*	实例就是我们的应用程序，卡可看成项目，
		*	实例有实例的句柄（ID）、结构体有结构体的句柄、窗口有窗口的句柄、画刷也有画刷的句柄
		*	通过句柄（ID）才能识别找到他们，HWND就是窗口的句柄类型
		* 
		* 注册结构体: RegisterClassEx
		*		创建好窗口结构体，要在操作系统里注册才能使用，就是让操作系统认可识别它
		* 
		* 创建窗口: CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, TEXT("Yywindow"), _T("Yy窗口"), WS_OVERLAPPEDWINDOW, 200, 200, 400, 300, NULL, NULL, hInstance, NULL)
		*		参1：基本风格、参2：结构体名、参3：窗口名、参4：额外风格（可以用按位或 | 来设置多种风格）
		*		参5、6：起始坐标（窗口左上角的坐标）、参7、8：窗口的宽、高
		*		参9：父窗口、参10：菜单（不用就填NULL）、参11：窗口实例的句柄、参12：回调函数
		* 
		* 显示窗口：ShowWindow(hwnd, ShowCmd);
		*		参1：创建的窗口、参2：显示方式，（主函数的最后一个参数）
		* 
		* 更新窗口：
		*		就是Qt里的绘窗口，参数为我们创建的窗口
		* 
		* 消息循环：
		*		GetMessage(&msg, NULL, 0, 0)：
		*			参1：消息结构体、参2：需要接收的消息的对应窗口（填NULL，则全部窗口都能接收到）
		*			参3、4：接收的范围（填0，则是所有范围）
		*		消息循环起到分类分发的作用，就像Qt的Event
		*		关闭窗口返回0
		* 
		* 回调函数：
		*		WinbackProc(HWND hwnd, UINT msgID, WPARAM wparam, LPARAM lparam)
		*			参1：窗口、参2：消息的ID、参3：得到的消息由哪个socket产生的（通过这个参数传递socket到回调函数中）、参4：是哪一个操作触发的
		*			参4：有分高位与低位，高位HIWORD装错误码，地位LOWORD装操作码如FD_ACCEPT等
		*		固定形式的写法
		*		将我们按键或者操作的信号转换成系统能识别的代码，进行后续的操作
		*		
		*/
		//5.3 WSAAsyncSelect函数的使用
		/*
		* 开始构建初步异步选择模型：
		*		打开网络库到开启监听的操作，只要放到消息循环之前就可以了，且只运行一次
		*		网络头文件（网络库）与windows.h有包含与被包含关系，所以我们用一个就可以了
		* 
		* WSAAsyncSelect：
		*		绑定事件与socket并投递出去
		*		
		*	功能：将socket与消息绑定在一起，并投递给操作系统
		*	
		*	参数1：socket
		*	参数2：窗口句柄，要绑定到哪个窗口上，就写那个窗口的句柄（ID,窗口的编号）
		*	参数3：消息编号，自定义的消息，（本质就是一个数）
		*	参数4：消息的类型（就是我们之前模型的错误码，如FD_WRITE等）
		*	返回值：成功返回0  失败返回SOCKET_ERROR
		*	有一个报错添加一个宏，在网络头文件的前面就好了 #define _WINSOCK_DEPRECATED_NO_WARNINGS
		* 
		* 整体逻辑：
		*		我们把socket与消息绑定在一起，通过上述函数，装到一个消息队列中（我们看不到）
		*		然后消息循环的GetMessage函数，依次从消息队列中往外拿消息
		*		通过循环中的两个函数分发，然后分发到了回调函数中
		*		回调函数中收到了消息，通过case去分类处理
		* 
		* 回调函数：
		*		WM_CREATE消息是初始化，只执行一次（往往只执行一次的都是初始化），
		*		所以我们上面的模型的通用结构（开启网络库~开启监听）可以放到回调函数中
		*		W:windows M ：Message消息
		*		WM_就是系统已经定义好的消息
		*		经过消息循环，将我们的自定义消息投递到回调函数，然后就能通过case来分类处理了
		*		当消息被触发，回调函数中就能捕获到了
		* 
		* 自定义消息：
		*		#define UM_ASYNCSELECTMSG WM_USER+1
		*		WM_USER是系统提供给我们自定义消息的
		*		因为消息就是一个数，那我们自定义的消息的数值不能与系统已经定义好的消息重复
		*		那比WM_USER这个宏的数值小的 都是系统已经定义好了的，我们不能用
		*		但大于WM_USER的数 就是我们能使用了的
		*		UM_代表是我们自定义的
		* 
		* MessageBox(NULL, L"有信号啦", TEXT("提示"), MB_OK);
		*		一个提示窗口：
		*		参1：指定父窗口，也可以不指定
		*		参2：窗口的内容
		*		参3：窗口标题
		*		参4：这个提示窗口的风格，MB_OK是有一个按钮，MB_YESNO 有两个按钮
		*/
		//5.4 消息分类处理1
		/*
		* 回调函数：
		*		参数4有分高位与低位，高位HIWORD装错误码，地位LOWORD装操作码如FD_ACCEPT等
		*		如果高位为0 则说明没有出错，不为0则说明出错了
		*		回调函数一次只处理一个消息
		* 
		*		switch中，不能直接声明变量，所以我们用{}括起来即可
		*	
		* 这节课的内容：
		*		回调函数中，进行分类处理，通过参数3将socket传进回调函数
		*		在FD_ACCEPT的处理中，我们接受连接，将客户端socket绑定消息，投递给消息队列
		*		自定义一个数组和计数，为了记录socket方便程序结束释放
		*		通过参数3获取到传递进回调函数的socket，参数4获取到操作码和错误码
		*	回调函数内错误处理就break即可，不用关闭网络库（小事不用关闭）
		* 
		* 事件分类处理逻辑：
		*		三个要点：客户端socket、产生的错误码、具体的消息种类
		*/
		//5.5 消息分类处理2
		/*
		* 1.客户区打印的问题、释放句柄的问题
		*		TextOut:在客户区打印内容
		*				客户区、非客户区：窗口标题能拖动部分是非客户区，而显示内容的矩形区域叫客户区
		*				HDC就是客户区的句柄，窗口有很多，就像纸有很多，要通过一个ID去区别找到这张纸
		*			参1：客户区HDC 参2、3：打印内容的起始坐标（客户区的原点在左上角0，0）
		*			参4：打印的内容、参5：打印多少个字符（用strlen更合适）sizeof的话会把\0也打印
		*		HDC可以申请后，在循环结束释放掉，方便下次进入回调函数使用，因为每次循环只执行一个
		* 2.FD_WRITE等消息的处理
		*		我们再打印时，打印的坐标位置要发生变动，
		*		可以定义一个全局变量，让其代表Y轴坐标进行变化
		*		FD_READ处理中，我们想用TextOut打印收到的字符串，但是提示：“函数”: 从“char [1024]”到“LPCWSTR”的类型不兼容
		*		我们右击项目-属性-字符集 改为多字节字符集就好了，但是要把之前的L TEXT("") 给去掉
		*		FD_ACCEPT与FD_WRITE消息的生成顺序与之前学的模型一样的，最先是FD_ACCEPT，连接后马上生成FD_WRITE
		* 
		*/
		//5.6 消息分类处理3 FD_CLOSE处理
		/*
		*		之前我们也知道，不论正常下线还是强制下线，FD_CLOSE都有错误码10053（宏为：WSAECONNABORTED）
		*		所以这里也要识别出错误码，然后进行以下3个操作
		*		而关于FD_CLOSE没有错误码的那里我们也加上下面3个操作，因为MSDN上对这个错误码描述不清楚
		*		怕有特殊情况
		* 
		* 1.关闭socket上的消息
		*		关闭消息：因为客户端下线，所以我们需要取消这个socket的消息
		*		让操作系统不用再监管他
		*		关闭的方式，用WSAAsyncSelect绑定投递函数，后两个参数设置成0，就覆盖掉原来的内容，就关闭了
		* 2.记录数组中删除改socket
		*		用循环找出这个socket的位置，然后与末尾元素交换
		*		然后计数-1即可
		* 3.释放这个socket
		*		closesocket();来释放
		*/
		//5.7 异步选择模型问题分析与优化
		/*
		* 课时49 老师有画一个流程图，我们可以对比一下之前的两个模型的流程图来学习理解
		* 
		* 存在阻塞：
		*		执行处理一个消息的时候，是无法执行其它的消息的
		*		所以用户量不要太多
		*	解决方案：
		*		采用多线程，我们手动控制每个窗口最多64个用户
		*		然后每个窗口分配1个线程即可解决
		* 
		* 小BUG：
		*		比如当我们在执行处理1的操作的时候，3客户端连续发送了多条消息
		*		那处理完1，再去处理3的时候，recv读取会将这多条消息一起读出来
		*		因为这多条消息都存在协议缓冲区把里了，事件选择模型也有这个问题
		*	解决方案：
		*		也是采用多线程就能解决
		*		多线程现在在还没有学习，但重要的是这种解决思路
		*/
	}
}