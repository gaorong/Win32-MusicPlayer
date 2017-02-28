// 音乐播放器.cpp : 定义应用程序的入口点。
//
#include "stdafx.h"
#include "音乐播放器.h"

typedef struct lrc
{
	char song_lrc[256];
	int time;
	struct lrc *next;
	struct lrc *pre;
}LYRIC, *PLYRIC;


// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名

DWORD BeginTime;
static int nIndex, cFlag;
static LONG lSongLength;
static int lVolum,lProgress;
static TCHAR szSong[MAX_PATH],szNowPlayedSong[MAX_PATH];
static PLYRIC pLyric = NULL;
static TCHAR szPerSong[MAX_PATH];
HFONT hFont1,hFont2;
RECT rect;

typedef struct Time
{
	int minute;
	int second;
}TIME,*PTIME;





// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
VOID CALLBACK TimerProc(HWND, UINT, UINT_PTR, DWORD);
void ReadLyric(PLYRIC head);
int music_position(PLYRIC head, HWND hwnd, long playingtime);
void music_lrc_double(PLYRIC head);
void list_del(PLYRIC head);
void music_lrc_sort(PLYRIC head);


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MY, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY));

	// 主消息循环: 
	while (GetMessage(&msg, NULL, 0, 0))
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
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON4));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON3));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      245, 22, 985, 620, NULL, NULL, hInstance, NULL);
   //985  535
   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   SetWindowText(hWnd, TEXT(""));		//将窗口的文字去掉
   
   return TRUE;
}

void Convert(LONG lTime, HWND hwndStatic)
{//将歌曲返回的时间转换为可显示的时间
	TIME tempTime = {0};
	TCHAR szTimeBuffer[256], szMinute[16], szSecond[16];
	tempTime.minute = lTime/ 1000 / 60 % 60;
	tempTime.second = lTime / 1000 % 60;
	
	if (tempTime.minute < 10)
		wsprintf(szMinute, TEXT("0%d"), tempTime.minute);
	else
		wsprintf(szMinute, TEXT("%d"), tempTime.minute);

	if (tempTime.second < 10)
		wsprintf(szSecond, TEXT("0%d"), tempTime.second);
	else
		wsprintf(szSecond, TEXT("%d"), tempTime.second);

	wsprintf(szTimeBuffer, TEXT("%s:%s /"), szMinute, szSecond);
	
	SetWindowText(hwndStatic, szTimeBuffer);
}


void in_it(HWND hwnd, int iSubItem, int cx, TCHAR *text, int cchTextMax, int len)
{
	LVCOLUMN ColInfo1 = { 0 };			//ListView控件所需要的结构
	ColInfo1.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	ColInfo1.iSubItem = iSubItem;
	ColInfo1.fmt = LVCFMT_CENTER;
	ColInfo1.cx = cx;
	ColInfo1.pszText = text;
	ColInfo1.cchTextMax = cchTextMax;
	SendMessage(hwnd, LVM_INSERTCOLUMN, WPARAM(len), LPARAM(&ColInfo1));
}

void set_data(HWND hwnd, TCHAR *text, int x, int y)
{
	LVITEM item;		//往ListView控件中插数据
	item.mask = LVIF_TEXT;
	item.pszText = text;
	item.iItem = x;
	item.iSubItem = y;
	if (y == 0)
		SendMessage(hwnd, LVM_INSERTITEM, 0, LPARAM(&item));
	else
		SendMessage(hwnd, LVM_SETITEM, 0, LPARAM(&item));
}

int PopupShortcutMenu(HWND hWnd,HMENU hMenu)
{
	POINT point;
	GetCursorPos(&point);//获取鼠标的位置
	//弹出菜单
	TrackPopupMenu(GetSubMenu(hMenu, 0), TPM_CENTERALIGN, point.x, point.y, 0, (HWND)hWnd, NULL);
	return 1;
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	
	PAINTSTRUCT ps;
	HDC hdc = GetDC(hWnd);
	int cxChar, cyChar,nLen,nMaxIndex,i;
	HWND hwndButton,hwndCombobox;
	OPENFILENAME ofn;
	TCHAR szBuffer[MAX_PATH], szPlayedMusic[MAX_PATH],szReturnTime[100], * p;
	static TCHAR szFileName[40 * MAX_PATH];
	static TCHAR szFilter[] = TEXT("Audio(*.mp3;*.wma;*.wav)\0*.mp3;*.wma;*.wav\0All(*.*)\0*.*");	
	static int nPreVolum;
	static BOOL flagChange = FALSE;
	POINT point;
	RECT PointerRect;
	HBITMAP hBitmap;
	HWND hImage;
	char szSongChar[MAX_PATH];
	LVCOLUMN ColInfo1 = { 0 };
	static HWND hWndListView, hwndProgressCtr,hwndTrackBar;
	LPNMHDR lpnmh;						//双击信息所需要的结构体
	WIN32_FIND_DATA FindFileData = {0};		//查找文件所需要的结构体
	static HMENU hPopupMenu;
	

	switch (message)
	{
	case WM_CREATE:
		hPopupMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
		CheckMenuItem(hPopupMenu, IDB_SHOWLIST, MF_CHECKED);

		rect.left = 25;
		rect.top = 25;
		rect.right = 500;
		rect.bottom = 360;				//初始化rect是为绘制歌词的白色背景
		cxChar = LOWORD(GetDialogBaseUnits());
		cyChar = HIWORD(GetDialogBaseUnits());		//得到文字的长宽
		SetClassLong(hWnd, GCL_HBRBACKGROUND, (LONG)CreateSolidBrush(RGB(236, 233, 216)));		//将背景设置为灰色
		hFont1 = CreateFont(22, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, TEXT("楷体_GB2312"));
		
		//创建个按钮
		CreateWindow(TEXT("button"), TEXT("播放"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			20 * cxChar + 15, 335 + 130, 20 * cxChar, 2 * cyChar, hWnd, (HMENU)IDB_PLAY, hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("上一曲"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			10, 335 + 130, 20 * cxChar, 2 * cyChar, hWnd, (HMENU)IDB_PRE, hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("下一曲"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			40 * cxChar + 20, 335 + 130, 20 * cxChar, 2 * cyChar, hWnd, (HMENU)IDB_NEXT, hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("添加歌曲到列表"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
			340,530-20, 20 * cxChar, 2 * cyChar, hWnd, (HMENU)IDB_ADD, hInst, NULL);

	
		CreateWindow(TEXT("static"), TEXT(""), WS_CHILD | WS_VISIBLE |SS_LEFT,
			400 + 6 * cxChar, 300 + 130, 5 * cxChar, cyChar, hWnd, (HMENU)IDB_STATICLENGTH, hInst, NULL);
		CreateWindow(TEXT("static"), TEXT(""), WS_CHILD | WS_VISIBLE | SS_LEFT,
			400, 300 + 130, 6 * cxChar, cyChar, hWnd, (HMENU)IDB_STATICNOWTIME, hInst, NULL);

		//hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_BITMAP1));
		//hImage = GetDlgItem(hWnd, IDB_STATICBITMAP);			//IDC_QQ为一个静态控件的ID
		//SendMessage(hImage, STM_SETIMAGE, IMAGE_BITMAP, (long)hBitmap);

	
		Convert(0, GetDlgItem(hWnd, IDB_STATICNOWTIME));		//将时间的两个静态控件置零
		Convert(0, GetDlgItem(hWnd, IDB_STATICLENGTH));
		InitCommonControls();
		//创建Combobox控件
		hwndCombobox = CreateWindow(WC_COMBOBOX, TEXT("列表循环"), WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST| WS_TABSTOP,
			180, 510, 15 * cxChar, 10 * cyChar, hWnd, (HMENU)IDB_COMBOBOX, hInst, NULL);

		ComboBox_AddString(hwndCombobox,TEXT("列表循环"));
		ComboBox_AddString(hwndCombobox, TEXT("单曲循环"));
		ComboBox_AddString(hwndCombobox, TEXT("随机循环"));
		SendMessage(hwndCombobox, CB_SETCURSEL, 0, 0);		//选择列表循环
			
		//创建ProgressCtrl   控件
		hwndProgressCtr = CreateWindow(TEXT("msctls_progress32"), TEXT("进度条"), WS_CHILD | WS_VISIBLE | PBS_SMOOTH | WS_BORDER,
			20, 300 + 130, 370, cyChar, hWnd, (HMENU)IDB_PROGRESSCTRL, hInst, 0);
		
		//创建TrackBar   控件
		//声音控制
		hwndTrackBar = CreateWindow(TRACKBAR_CLASS, TEXT(""), WS_CHILD | WS_VISIBLE | TBS_TOP ,
			35, 500, 129, 30, hWnd, (HMENU)IDB_TRBVOICE, hInst, 0);
		
		SendDlgItemMessage(hWnd, IDB_TRBVOICE, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, 1000));
		SendDlgItemMessage(hWnd, IDB_TRBVOICE, TBM_SETPOS, TRUE, 500);     //初始化声音进度条最右
		lVolum = 500;
		
		//进度控制
		hwndTrackBar = CreateWindow(TRACKBAR_CLASS, TEXT("Trackbar Control"), WS_CHILD | WS_VISIBLE | TBS_BOTH,
			0, 277 + 130, 395, 1.5*cyChar, hWnd, (HMENU)IDB_TRBPROGRESS, hInst, 0);
		

		//创建ListView控件
		hWndListView = CreateWindowEx(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE, WC_LISTVIEW, TEXT(""),
			WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_ALIGNTOP,
			65 * cxChar, 25, 400, 510, hWnd, (HMENU)IDB_LISTVIEW, hInst, NULL);
		
	
		//设置子控件的字体，属性，颜色
		SendMessage(hWndListView, WM_SETFONT, (WPARAM)hFont1, 0);
		SendMessage(hWndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_GRIDLINES);
		//对于列表框的信息处理
		in_it(hWndListView, 0, 0, TEXT("路径"), MAX_PATH, 0);		//先往进添加的在后面
		in_it(hWndListView, 0, 390, TEXT("歌曲列表"),   MAX_PATH, 0);

		SkinH_Attach();			//第三方皮肤的SDK—-skinh

		pLyric = (PLYRIC)malloc(sizeof(LYRIC));			//创建歌词的头节点
		ZeroMemory(pLyric, sizeof(LYRIC));

		hFont1 = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, TEXT("楷体_GB2312"));	
		hFont2 = CreateFont(40, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, TEXT("Monotype Corsiva"));
		
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDB_PLAY:
			if (ListView_GetItemCount(hWndListView) == 0)		//如果歌词列表为空，则弹出对话框，提示添加歌曲
			{
				if (MessageBox(hWnd, TEXT("你的歌单尚未添加音乐\n\r\n\r是否添加？"), TEXT("提示"), MB_OKCANCEL) == IDCANCEL)
					break;
				else
				{
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDB_ADD, 0);
					ListView_SetItemState(hWndListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);
					ListView_SetItemState(hWndListView, 0, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
			//获得歌曲
			nIndex = ListView_GetNextItem(hWndListView, -1, LVNI_FOCUSED);
			ListView_GetItemText(hWndListView, nIndex, 1, szNowPlayedSong, MAX_PATH);
			GetShortPathName(szNowPlayedSong, szSong, sizeof(szSong));

			if (lstrcmp(szSong, szPerSong) != 0)
			{//若新歌曲和旧歌曲不一样，则对新歌曲进行处理
				for (i = 0; i < lstrlen(szSong) - 3; i++)
					szBuffer[i] = szSongChar[i];		
				lstrcat(szBuffer, TEXT("lrc\0"));		//获得当前歌曲的lrc歌词文件

				if (FindFirstFile(szBuffer, &FindFileData) == NULL)
				{
					list_del(pLyric);			//查看歌词文件是否存在
					pLyric->next = NULL;		//为后文的显示做个标志
				}
				else
				{
					list_del(pLyric);		//删除上一个歌词链表
					ZeroMemory(pLyric, sizeof(LYRIC));
					ReadLyric(pLyric);		//读取新歌词链表
					music_lrc_sort(pLyric);		//排序
					music_lrc_double(pLyric);		//使其成为一个双链表
				}
				lstrcpy(szPerSong, szSong);
				//读出新的歌曲信息
				wsprintf(szBuffer, TEXT("status %s length"), szSong);
				mciSendString(szBuffer, szReturnTime, sizeof(szReturnTime), NULL);		//显示总长
				//SetDlgItemText(hWnd, IDB_STATICLENGTH, szReturnTime);
				
				//设置进度条进度
				lSongLength = _tcstol(szReturnTime, NULL, 10);
				Convert(lSongLength, GetDlgItem(hWnd, IDB_STATICLENGTH));
				SendMessage(hwndProgressCtr, PBM_SETRANGE, 0, MAKELPARAM(0, (lSongLength / 500)-1));
				SendMessage(hwndProgressCtr, PBM_SETPOS, (WPARAM)0, 0);
				//设置滑动条进度
				SendDlgItemMessage(hWnd, IDB_TRBPROGRESS, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, (lSongLength / 500) - 1));
				SendDlgItemMessage(hWnd, IDB_TRBPROGRESS, TBM_SETPOS, TRUE, 0);     //初始化声音进度条最右

				//在窗体上显示歌曲名称
				ListView_GetItemText(hWndListView, nIndex, 0, szBuffer, MAX_PATH);
				SetBkColor(hdc, RGB(236, 233, 216));
				SelectObject(hdc,hFont1);
				
				PointerRect.top = 375;
				PointerRect.left = 20;
				PointerRect.right = 500;
				PointerRect.bottom = 405;
				DrawText(hdc, TEXT("                                                 "), 50, &PointerRect, NULL);
				DrawText(hdc, szBuffer, lstrlen(szBuffer), &PointerRect, NULL);
			}
			GetDlgItemText(hWnd, IDB_PLAY, szBuffer, 1024);
			if (lstrcmp(szBuffer, TEXT("播放")))//通过按钮文字的控制进行判断
			{
				SetDlgItemText(hWnd,IDB_PLAY, TEXT("播放"));
				wsprintf(szBuffer, TEXT("%s %s"), TEXT("pause"), szSong);
				mciSendString(szBuffer, NULL, 0, NULL);
				//关闭定时器
				KillTimer(hWnd, ID_TIMER);
			}
			else
			{//开始播放
				SetDlgItemText(hWnd, IDB_PLAY, TEXT("暂停"));

				wsprintf(szBuffer, TEXT("play %s"), szSong);
				mciSendString(szBuffer, NULL, 0, NULL);
				//处理当前歌曲的音量
				lVolum = SendDlgItemMessage(hWnd, IDB_TRBVOICE, TBM_GETPOS, 0, 0);
				wsprintf(szBuffer, TEXT("setaudio %s volume to %d"), szSong, lVolum);
				mciSendString(szBuffer, NULL, 0, NULL);	//这个函数只有在播放歌曲的时候可以调节音量而且下一首自动变为1000
				
				SetTimer(hWnd, ID_TIMER, 500, TimerProc);		//设置定时器，定时调用回掉函数
			}
			break;
		case IDB_PRE:	//上一首&下一首
		case IDB_NEXT:
			if (ListView_GetItemCount(hWndListView) == 0)
			{
				if (MessageBox(hWnd, TEXT("你的歌单尚未添加音乐\n\r\n\r是否添加？"), TEXT("提示"), MB_OKCANCEL) == IDCANCEL)
					break;
				else
				{
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDB_ADD, 0);
					ListView_SetItemState(hWndListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);
					ListView_SetItemState(hWndListView, 0, LVIS_SELECTED, LVIS_SELECTED);
				}
			}

			nMaxIndex = ListView_GetItemCount(hWndListView);
			nIndex = ListView_GetNextItem(hWndListView, -1, LVNI_FOCUSED);
			if (wmId == IDB_PRE)
			{	
				nIndex = (--nIndex+ nMaxIndex) % nMaxIndex;
			}
			else
			{	
				nIndex = (++nIndex ) % nMaxIndex;
			}
			ListView_SetItemState(hWndListView, nIndex, LVIS_FOCUSED,  LVIS_FOCUSED);
			ListView_SetItemState(hWndListView, nIndex, LVIS_SELECTED, LVIS_SELECTED);
	
			wsprintf(szBuffer, TEXT("%s %s"), TEXT("close"), szSong);
			mciSendString(szBuffer, NULL, 0, NULL);
			hdc = GetDC(hWnd);
			FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));
			ReleaseDC(hWnd,hdc);

			SetDlgItemText(hWnd, IDB_PLAY, TEXT("播放"));
			SendMessage(hWnd, WM_COMMAND, IDB_PLAY, 0);
			break;
	
		case IDB_SHOWLIST:			//歌词列表的显示与隐藏
			CheckMenuItem(hPopupMenu, IDB_SHOWLIST, 
				(GetMenuState(hPopupMenu, IDB_SHOWLIST, MF_CHECKED) == MF_CHECKED) ? MF_UNCHECKED:MF_CHECKED);
			(GetMenuState(hPopupMenu, IDB_SHOWLIST, MF_CHECKED) == MF_CHECKED) ? 
				MoveWindow(hWnd, 245, 22, 985, 620, TRUE) : MoveWindow(hWnd, 245, 22, 525, 620, TRUE);
			break;
	
		case IDB_COMBOBOX:
			if (wmEvent == CBN_SELCHANGE)
			{
				hwndCombobox = (HWND)lParam;			//lParam参数表示窗体的句柄
				cFlag = SendMessage(hwndCombobox, CB_GETCURSEL, 0, 0);				//cFlag 为全局变量，一首歌曲播放完毕时，检查该值，自动确定下一首应该播放的歌曲
				//0. 列表 1. 单曲 2. 随机
			}
			else
				return DefWindowProc(hWnd, message, wParam, lParam);		//注意将其他消息交给windows去处理
			break;
		case IDB_ADD:		//添加歌曲
		case IDM_FILE_OPEN:
			
			ZeroMemory(&ofn, sizeof(OPENFILENAME));
			ofn.lStructSize = sizeof(OPENFILENAME);
			ofn.hwndOwner = hWnd;
			ofn.hInstance = NULL;
			ofn.lpstrFilter = szFilter;
			ofn.lpstrCustomFilter = 0;
			ofn.nMaxCustFilter = 0;
			ofn.nFilterIndex = 0;
			ofn.lpstrFile = szFileName;
			ofn.nMaxFile = 40*MAX_PATH;
			ofn.lpstrFileTitle = szBuffer;
			ofn.nMaxFileTitle = 40*MAX_PATH;
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrTitle = TEXT("打开");
			ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;
			ofn.nFileOffset = 0;
			ofn.nFileExtension = 0;
			ofn.lpstrDefExt = TEXT(".mp3");
			ofn.lCustData = 0L;
			ofn.lpfnHook = NULL;
			ofn.lpTemplateName = NULL;

			if (GetOpenFileName(&ofn))
			{
				ZeroMemory(szBuffer, sizeof(szBuffer));
				lstrcpyn(szBuffer, szFileName, ofn.nFileOffset);
				szBuffer[ofn.nFileOffset] = '\0';		//若是选择多个文件进行文件名的分解
				nLen = lstrlen(szBuffer);
				if (szBuffer[nLen - 1] != '\\')
					lstrcat(szBuffer, TEXT("\\"));
				p = szFileName + ofn.nFileOffset;
				while (*p)
				{
					ZeroMemory(szPlayedMusic, sizeof(szPlayedMusic));
					lstrcat(szPlayedMusic, szBuffer);
					lstrcat(szPlayedMusic, p);
					set_data(hWndListView, p, 0, 0);
					set_data(hWndListView, szPlayedMusic, 0, 1);
					
					p += lstrlen(p) + 1;
				}
			}
			if (ListView_GetSelectionMark(hWndListView)== -1)
			{
				ListView_SetItemState(hWndListView, 0, LVIS_FOCUSED, LVIS_FOCUSED);
				ListView_SetItemState(hWndListView, 0, LVIS_SELECTED, LVIS_SELECTED);
			}
					
			break;
		
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		//ShowScrollBar(hWndListView, SB_HORZ, FALSE);
		
		SetFocus(hWndListView);
		break;
		case WM_HSCROLL:	//滑动条的消息
		//鼠标处理音量	
			
			if ((LOWORD(wParam) == SB_THUMBPOSITION || LOWORD(wParam) == SB_PAGEDOWN || LOWORD(wParam) == SB_PAGEUP))
			{//只有滚动和上下页的时候才处理消息——在滚动条上的点击为上下页，所以只有这种消息才处理
				if (GetAsyncKeyState(VK_LBUTTON) & 0X8000)
				{
					GetCursorPos(&point);
					ScreenToClient((HWND)lParam, &point);
					ZeroMemory(&PointerRect, sizeof(PointerRect));
					GetClientRect((HWND)lParam, &PointerRect);
								
					if (point.x < PointerRect.right && point.x > PointerRect.left && point.y > PointerRect.top - 10 && point.y < PointerRect.bottom + 10)
					{
						//如果在声音滑动条的控件范围内
						if ((GetDlgItem(hWnd, IDB_TRBVOICE) == (HWND)lParam))
						{
							SendDlgItemMessage(hWnd, IDB_TRBVOICE, TBM_SETPOS, TRUE, (LPARAM)(1050 * (point.x - 15) / 100));
							
						}
						//如果在进度滑动条的控件范围内
						if ((GetDlgItem(hWnd, IDB_TRBPROGRESS) == (HWND)lParam))
						{
							flagChange = TRUE;
							SendDlgItemMessage(hWnd, IDB_TRBPROGRESS, TBM_SETPOS, TRUE, (LPARAM)((lSongLength / 500) * (point.x - 10) / 370.0));
							//lProgress = lSongLength * (point.x - 10) / 370.0;
							

						}		
					}
				}
				if (flagChange)
				{
					lProgress = 500 * SendDlgItemMessage(hWnd, IDB_TRBPROGRESS, TBM_GETPOS, 0, 0);
					wsprintf(szBuffer, TEXT("play %s from %ld"), szSong, lProgress);
					mciSendString(szBuffer, NULL, 0, NULL);
					wsprintf(szBuffer, TEXT("play %s from %ld"), szSong, lProgress);
					mciSendString(szBuffer, NULL, 0, NULL);
				}
				GetDlgItemText(hWnd, IDB_PLAY, szBuffer, 1024);
				if (lstrcmp(szBuffer, TEXT("播放")))
				{
					SetDlgItemText(hWnd, IDB_PLAY, TEXT("播放"));
					SendMessage(hWnd, WM_COMMAND, IDB_PLAY, 0);
				}
			}	
			break;
	case WM_RBUTTONUP:
		PopupShortcutMenu(hWnd, hPopupMenu);
		break;
	case WM_NOTIFY:
		lpnmh = (LPNMHDR)lParam;
		if (NM_DBLCLK == lpnmh->code)
		{
			wsprintf(szBuffer, TEXT("%s %s"), TEXT("close"), szSong);
			mciSendString(szBuffer, NULL, 0, NULL);
			hdc = GetDC(hWnd);
			FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));
			ReleaseDC(hWnd, hdc);
			hwndButton = GetDlgItem(hWnd, IDB_PLAY);
			SetWindowText(hwndButton, TEXT("播放"));
			SendMessage(hWnd,WM_COMMAND,IDB_PLAY,0);
		}

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...

		FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));
		PointerRect.top = 375;
		PointerRect.left = 20;
		PointerRect.right = 500;
		PointerRect.bottom = 405;
		ZeroMemory(szBuffer, sizeof(szBuffer));

		GetFileTitle(szNowPlayedSong, szBuffer, sizeof(szBuffer));
		if (szBuffer != NULL)
		{
			SetBkColor(hdc, RGB(236, 233, 216));
			SelectObject(hdc, hFont1);
			DrawText(hdc, TEXT("                                                 "), 50, &PointerRect, NULL);
			DrawText(hdc, szBuffer, lstrlen(szBuffer), &PointerRect, NULL);
		}
		EndPaint(hWnd, &ps);
		break;
	
		
	case WM_DESTROY:
		free(pLyric);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
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

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	TCHAR szBuffer[MAX_PATH], szReturnTimePos[256];
	HWND hWndListView;
	int nMaxIndex,nPreIndex,i,j = 0;
	LONG lTimePos = 0;
	static LONG lPreTimePos;
	PLYRIC temp = pLyric->next;		//相对较费CPU
	HDC hdc = GetDC(hwnd);
	TCHAR szTchar[500],szShortTchar[250];
	int x = 30;
	int y = 200;
	RECT rect;
	rect.left = 25;
	rect.top = 25;
	rect.right = 500;
	rect.bottom = 360;
	SIZE size;
	
	wsprintf(szBuffer, TEXT("status %s position"), szSong);
	mciSendString(szBuffer, szReturnTimePos, sizeof(szReturnTimePos), NULL);
	//时间的显示应该是时分秒的形式
	lTimePos = _tcstol(szReturnTimePos, NULL, 10);

	Convert(lTimePos, GetDlgItem(hwnd, IDB_STATICNOWTIME));

	SendDlgItemMessage(hwnd, IDB_PROGRESSCTRL, PBM_SETPOS, lTimePos / 500, 0);
	SendDlgItemMessage(hwnd, IDB_TRBPROGRESS, TBM_SETPOS, TRUE, (LPARAM)(lTimePos / 500));

	if (lTimePos >= lSongLength - 2000)
	{//当播放完毕时
		wsprintf(szBuffer, TEXT("%s %s"), TEXT("close"), szSong);
		mciSendString(szBuffer, NULL, 0, NULL);
		SetDlgItemText(hwnd, IDB_PLAY, TEXT("播放"));
		nPreIndex = nIndex;
		if (cFlag == 1);
		else
		{
			hWndListView = GetDlgItem(hwnd, IDB_LISTVIEW);
			nMaxIndex = ListView_GetItemCount(hWndListView);

			if (cFlag == 0)
			{
				nIndex = (++nIndex) % nMaxIndex;
			}
			else
			{
				while (nPreIndex == nIndex)
					nIndex = rand() % nMaxIndex;
			}
			ListView_SetItemState(hWndListView, nIndex, LVIS_FOCUSED, LVIS_FOCUSED);
			ListView_SetItemState(hWndListView, nIndex, LVIS_SELECTED, LVIS_SELECTED);

		}
		FillRect(hdc, &rect, GetStockBrush(WHITE_BRUSH));
		SendMessage(hwnd, WM_COMMAND, IDB_PLAY, 0);

		return;
	}

	if (temp != NULL)		//存在歌词时
	{
		SelectObject(hdc, hFont1);
		while (lTimePos >= (temp->time))       //和现在正在播放的时间点匹配
		{
			temp = temp->next;
		}

		for (i = 0; i < 4; i++)			//定义到前三行，便于后面的七行显示
		{
			if (temp->pre != NULL)
			{
				temp = temp->pre;
				y -= 30;
				j++;
			}
		}
		for (i = 0; i < j+3; i++)		//显示七行文字
		{

			SetTextColor(hdc, RGB(0, 255, 0));
			if (temp != NULL || temp->next != NULL)		//当歌词显示完毕，不再执行循环
			{
				if (i == 3)
					SetTextColor(hdc, RGB(225, 0, 0));
				TextOut(hdc, 30, y, TEXT("                                              "), 46);
				MultiByteToWideChar(CP_ACP, 0, temp->song_lrc, -1, szTchar, 200);

				GetTextExtentPoint32(hdc, szTchar, lstrlen(szTchar), &size);
				if (30 + size.cx >= 500)
				{					
					lstrcpyn(szShortTchar, szTchar, 23);
					lstrcat(szShortTchar, TEXT("..."));
					TextOut(hdc, 30, y, szShortTchar, lstrlen(szShortTchar));
				}
				else
				{
					TextOut(hdc, 30, y, szTchar, lstrlen(szTchar));
				}
			}
	
			y += 30;
			temp = temp->next;
		}
	}
	else
	{
		SelectObject(hdc, hFont1);
		SetTextColor(hdc, RGB(255, 52, 158));
		TextOut(hdc, 250, 250, TEXT("(暂无歌词๑ -﹏- ๑)"), lstrlen(TEXT("(暂无歌词率(๑ -﹏- ๑))")));
		SelectObject(hdc, hFont2);
		SetTextColor(hdc, RGB(161, 52, 158));
		TextOut(hdc, 60, 120, TEXT("Grower Music Player,"), 20);
		TextOut(hdc, 120, 170, TEXT("playing you daily life!"), 23);
	}
}

int get_time(char *buf)

{ //获取歌词时间标签，返回得到的时间
	static int pretime ;
	if (lstrcmp(szSong, szPerSong) != 0)
	{
		pretime = 0;
	}

	int time = 0;
	
	if ((buf[0]) == '[' && buf[1] == '0')	//如果是正常的时间，取出时间
	{
		time = ((((buf[1] - '0') * 10 + (buf[2] - '0')) * 60) + ((buf[4] - '0') * 10 +
			(buf[5] - '0'))) * 1000 + (buf[7] - '0') * 10 + buf[8] - '0';
		pretime = time;
		return time;
	}
	else
		return pretime + 1;		//如果不是正常的时间则加一用以排序的时候排在后边

}
char *get_lrc(char * buff)
{
	int i = 0;
	while ((buff[i] == '[') || (buff[i] == ']') || (buff[i] == ':') || (buff[i] == '.') || (buff[i] >= '0' && buff[i] <= '9'))
		i++;

	return buff + i;
}

void ReadLyric(PLYRIC head)
{
	PLYRIC node,temp;
	char szFileName[MAX_PATH] = {0};
	int i, count, nextposition = 0,findend = 0;           //找标题结束标记//count表示时间个数
	FILE *fp;
	char szBuffer[500];
	char szSongChar[MAX_PATH];
	errno_t err;
	temp = head;


	WideCharToMultiByte(CP_ACP, 0, (PWSTR)szNowPlayedSong, -1, szSongChar,
	MAX_PATH + 2, NULL, NULL);

	for (i = 0; i < strlen(szSongChar) - 3; i++)
		szFileName[i] = szSongChar[i];
	strcat_s(szFileName, "lrc\0");
	
	err = fopen_s(&fp, szFileName, "r");
	if (err != NULL)
	{
		return;
	}
	else
	{
		do{
			i = 0;
			count = 0;
			ZeroMemory(szBuffer, sizeof(szBuffer));
			fgets(szBuffer,500,fp);
			

			while (szBuffer[i] != '\0')
				if (szBuffer[i++] == '[')
					count++;
				nextposition = 0;
				for (i = 0; i < count; i++)
				{
					node = (PLYRIC)malloc(sizeof(LYRIC));
					if (szBuffer[1] == '0')
					{//如果是正常的歌词
						node->time = get_time(szBuffer + nextposition);
						strcpy_s(node->song_lrc, get_lrc(szBuffer));
						temp->next = node;
						head->time++;  //充分利用头节点作为一个给整个链表计数的变量
						temp = temp->next;
						nextposition += 10;
					}
					else if (szBuffer[1] == 't' || szBuffer[1] == 'a') //如果是标题或者作者或者其它
					{					
						node->time = 0;
						strcpy_s(node->song_lrc, szBuffer);
						temp->next = node;
						head->time++;
						temp = temp->next;			
					}
					else    //其它，前面没有标记的，作为普通歌词
					{
						node->time = get_time(szBuffer + nextposition);
						strcpy_s(node->song_lrc, szBuffer);
						temp->next = node;
						head->time++;
						temp = temp->next;
					}
					node->next = NULL;
				}
		} while (!feof(fp));


		

		fclose(fp);
	}
}

void music_lrc_sort(PLYRIC head)
{//歌词链表按时间排序
	int i = 0;
	PLYRIC al = head;
	PLYRIC pre = NULL;
	PLYRIC sw = NULL;
	PLYRIC temp = NULL;
	PLYRIC tempnext = NULL;
	
	for (i = 0; i < head->time - 1; i++)
	{
		al = head;
		while (al->next->next != NULL)
		{
			pre = al;
			temp = al->next;
			tempnext = al->next->next;
			if (temp->time > tempnext->time)
			{
				sw = tempnext;
				temp->next = sw->next;
				sw->next = pre->next;
				pre->next = sw;
			}
			al = al->next;
		}
	}
	return;
}

void music_lrc_double(PLYRIC head)		//将双链表链接起来
{
	PLYRIC temp = head;
	PLYRIC tempnext = head->next;

	while (temp->next != NULL)
	{
		tempnext->pre = temp;
		temp = temp->next;
		tempnext = tempnext->next;
	}
	//head->next->pre = NULL;
}

void list_del(PLYRIC head)       //删除链表
{
	if (head->next == NULL)
		return;
	PLYRIC del = NULL;
	while (NULL != head->next)
	{
		del = head->next;
		head->next = del->next;
		free(del);
	}
	return;
}



