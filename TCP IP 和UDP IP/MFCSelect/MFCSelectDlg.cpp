
// MFCSelectDlg.cpp: 实现文件
//


#include "pch.h"
#include "framework.h"
#include "MFCSelect.h"
#include "MFCSelectDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MY_MSG_ID WM_USER+1 


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCSelectDlg 对话框



CMFCSelectDlg::CMFCSelectDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCSELECT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


//*消息映射表，非常重要，每个文件可能存在多个消息映射表，通过参数1（类名）来区分
BEGIN_MESSAGE_MAP(CMFCSelectDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(MY_MSG_ID, &CMFCSelectDlg::OnMymsg)
END_MESSAGE_MAP()

//*自定义的处理函数，就相当于回调函数
LRESULT CMFCSelectDlg::OnMymsg(WPARAM wParam, LPARAM lParam)
{
	//获取socket
	SOCKET sock = (SOCKET)wParam;

	//lparam的高位存放错误码，为0则没有错误
	if (0 == HIWORD(lParam))
	{
		//获取这个消息所在窗口的客户区句柄（ID），在下面需要释放，用完就要释放
		HDC hdc = ::GetDC(m_hWnd);
		//消息类型分类处理
		if (FD_WRITE == LOWORD(lParam))
		{
			//辅助输出的函数TRACE，MFC中替代TextOut
			TRACE("%s\n", "FD_WRITE");
			//TextOut(hdc, 0, n, "FD_WRITE", strlen("FD_WRITE"));
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
				TRACE("%d\n", a);
				//TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
				n += 20;
			}
			TRACE("%s\n", strbuf);
			//TextOut(hdc, 0, n, strbuf, strlen(strbuf));
			n += 20;
			//sendto发送信息
			int snum = sendto(sock, "Server 收到\n", strlen("Server 收到\n"), 0, (const struct sockaddr*)&saClient, sizeof(saClient));
			if (SOCKET_ERROR == snum)
			{
				int a = WSAGetLastError();
				char str[20] = { 0 };
				TRACE("%d\n", a);
				//TextOut(hdc, 0, n, _itoa(a, str, 10), strlen(strbuf));
				n += 20;
			}
		}
		//释放HDC（客户区句柄）,方便下次进入回调函数使用，因为每次只执行一个
		::ReleaseDC(m_hWnd, hdc);
	}

	return 0;
}

//析构函数
CMFCSelectDlg::~CMFCSelectDlg()
{
	closesocket(SerVerSocket);
	WSACleanup();
}

// CMFCSelectDlg 消息处理程序

BOOL CMFCSelectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	//将处理函数中的n初始化
	n = 0;

	//网络通信代码
	//打开网络库
	WORD WSVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int nrse = WSAStartup(WSVersion, &wsaData);
	if (0 != nrse)
	{
		int a = WSAGetLastError();
		printf("WSAStratup error: %d\n", a);
		CloseHandle(m_hWnd);
		system("pause>0");
	}
	//校验版本
	if (2 != HIBYTE(wsaData.wVersion) || 2 != LOBYTE(wsaData.wVersion))
	{
		int a = WSAGetLastError();
		printf("网络库版本不正确 error: %d\n", a);
		CloseHandle(m_hWnd);
		WSACleanup();
		system("pause>0");
	}
	//创建SOCKET （UDP的）（声明放到全局，方便点×退出函数调用）
	SerVerSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == SerVerSocket)
	{
		int a = WSAGetLastError();
		printf("创建SOCKET error: %d\n", a);
		CloseHandle(m_hWnd);
		WSACleanup();
		system("pause>0");
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
		CloseHandle(m_hWnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
	}

	//绑定消息与socket并且投递给操作系统
	int wsaasync = WSAAsyncSelect(SerVerSocket, m_hWnd, MY_MSG_ID, FD_READ | FD_WRITE);
	if (SOCKET_ERROR == wsaasync)
	{
		int a = WSAGetLastError();
		printf("绑定消息函数 error: %d\n", a);
		CloseHandle(m_hWnd);
		closesocket(SerVerSocket);
		WSACleanup();
		system("pause>0");
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCSelectDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCSelectDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCSelectDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void study()
{
	// 5.5 MFC异步选择 笔记
	{
		/*
		**消息映射表，非常重要，每个文件可能存在多个消息映射表，通过参数1（类名）来区分，如
		*	BEGIN_MESSAGE_MAP(CMFCSelectDlg, CDialogEx)
		* 
		* 消息映射表的使用，如：
		* ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
		* 参数1是消息ID，参数2是对应ID的处理函数
		* ID产生消息，然后交给处理函数来处理
		* 我们可以自定义消息ID，自定义处理函数，消息ID与一般的win32的消息ID一样WM_USER+1才是我们能使用的范围
		* #define MY_MSG_ID WM_USER+1 
		* 
		* 注意这是C++格式的代码，要在对应头文件中加上函数声明
		* 且释放网络库的函数放到析构函数中，（不能忘记释放）
		* 
		* 通信网络库代码放到初始化函数中，OnInitDialog函数
		* 通信网络库基本代码也能放到构造函数中，但要做一定修改，否则我的测试中，出现了强制客户端断线的情况
		* m_hWnd 是MFC已经定义好的，我们直接使用就好
		* 
		* 消息处理函数自己创建然后放代码进去
		* 自定义消息用：ON_MESSAGE
		* LRESULT OnMymsg(WPARAM wParam, LPARAM lParam); 记得在头文件类中添加自定义函数
		* 作用与回调函数一样的，LRESULT是声明此函数为处理函数的固定写法吧
		* 
		* n要在类中声明，且在初始化函数中将它初始化
		* 
		* ::GetDC(m_hWnd);
		* ::ReleaseDC(m_hWnd, hdc);
		* 加上:: 是为了让MFC能识别使用
		* 
		* TextOut在MFC中不好使用，因为字符集的问题，常量字符串，如"dawd"能加L改变，但是字符数组就不容易改变了，此时要使用一个函数来改变
		*		但是就会麻烦很多，所以我们改用TRACE
		*		辅助输出的函数TRACE，MFC中替代TextOut
		* 
		* 有一个错误警告，说是要加一个宏，网上搜到的方式为，在项目右击》属性》C/C++》预处理器》预处理器定义中添加那个宏，而不是在文件最上面添加
		* 
		* 最终链接服务器，服务器的消息在右下角的输出框中显示，而不是MFC的窗口框显示
		*/
	}
}
