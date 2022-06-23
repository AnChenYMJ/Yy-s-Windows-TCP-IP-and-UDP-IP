// WindowsProject1.cpp : 定义应用程序的入口点。
//
#define _CRT_SECURE_NO_WARNINGS     //_itoa    的警告提示要加

#include "framework.h"
#include "WindowsProject1.h"

#define MAX_LOADSTRING 100

//非空异步选择复制过来的头文件
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#define MY_MSG_ID WM_USER+1

//定义在全局方便调用，主函数中不能再定义，只能调用
SOCKET SerVerSocket;
HWND hWnd;          

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance); //自定义函数，定义了了窗口类成员、注册窗口，就是空项目的第1、2步

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))    //自定义函数，与空项目的步骤吻合，只是封装成此函数了
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    //网络通信代码
    //打开网络库
    WORD WSVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    int nrse = WSAStartup(WSVersion, &wsaData);
    if (0 != nrse)
    {
        int a = WSAGetLastError();
        printf("WSAStratup error: %d\n", a);
        CloseHandle(hWnd);
        system("pause>0");
        return 0;
    }
    //校验版本
    if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
    {
        int a = WSAGetLastError();
        printf("网络库版本不正确 error: %d\n", a);
        CloseHandle(hWnd);
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
        CloseHandle(hWnd);
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
        CloseHandle(hWnd);
        closesocket(SerVerSocket);
        WSACleanup();
        system("pause>0");
        return 0;
    }

    //绑定消息与socket并且投递给操作系统
    int wsaasync = WSAAsyncSelect(SerVerSocket, hWnd, MY_MSG_ID, FD_READ | FD_WRITE);
    if (SOCKET_ERROR == wsaasync)
    {
        int a = WSAGetLastError();
        printf("绑定消息函数 error: %d\n", a);
        CloseHandle(hWnd);
        closesocket(SerVerSocket);
        WSACleanup();
        system("pause>0");
        return 0;
    }


    MSG msg;
    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex); //注册
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
int n = 0;  //给TextOut使用
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) //回调函数
{
    switch (message)
    {
    case MY_MSG_ID:
    {
        //获取socket
        SOCKET sock = (SOCKET)wParam;

        //lparam的高位存放错误码，为0则没有错误
        if (0 == HIWORD(lParam))
        {
            //获取这个消息所在窗口的客户区句柄（ID），在下面需要释放，用完就要释放
            HDC hdc = GetDC(hWnd);
            //消息类型分类处理
            if (FD_WRITE == LOWORD(lParam))
            {
                TextOut(hdc, 0, n, "FD_WRITE", strlen("FD_WRITE"));
                n += 20;
            }
            if (FD_READ == LOWORD(lParam))
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
            ReleaseDC(hWnd, hdc);
        }
    }
    break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:         //菜单栏帮助的下拉项（关于按钮）
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:          //菜单栏文件的下拉项（退出按钮）
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);    
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);    //默认流程，什么都不处理，但必须有，这样放的位置更合理
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}



void study()
{
    //5.4 飞控项目异步选择
    {
       /*
       * 我们的UDP代码放到 主消息循环前     代码从 打开网络库到投递 为止
       * 分类处理直接复制到这里的回调函数中
       * 要修改成多字符集，项目右击属性--高级--修改字符集--多字节   才能使用TextOut 然后 一个参数n 要在函数外定义初始化成0
       * _itoa_s  (a, str, 20, 10)  参数中的20 是s 版本多的参数，是字符的长度，且返回值不一样了，所以不能直接在TextOut函数中作为参数
       * 
       * 有时#define _CRT_SECURE_NO_WARNINGS 不管用，就加到预编译头文件中，就是预编译头文件中（C++的项目结构）
       * 
       * 问题与解决方案：在弹出的窗口中，客户端传输的内容，无法将 空格 打印 一遇到空格就变成了换行
       *    原因是：我们客户端输入使用了scanf 而scanf自动将空格理解为换行，就分成多个包发送了
       *    解决方案，将scanf用gets来替换即可
       */
    }
}
