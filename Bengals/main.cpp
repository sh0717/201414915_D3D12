
#include "pch.h"
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include "Resource.h"
#include "Game.h"

// required .lib files
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }

//////////////////////////////////////////////////////////////////////////////////////////////////////
// D3D12 Agility SDK Runtime

//extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }

//#if defined(_M_ARM64EC)
//	extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\arm64\\"; }
//#elif defined(_M_ARM64)
//	extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\arm64\\"; }
//#elif defined(_M_AMD64)
//	extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\x64\\"; }
//#elif defined(_M_IX86)
//	extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\x86\\"; }
//#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////

static constexpr int MAX_LOADSTRING = 100;

// Global Variables:
HINSTANCE g_instanceHandle = nullptr;
HWND g_mainWindow = nullptr;
WCHAR g_title[MAX_LOADSTRING];
WCHAR g_windowClass[MAX_LOADSTRING];

std::unique_ptr<CGame> g_game;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPWSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, g_title, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_MY01CREATEDEVICE, g_windowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	g_mainWindow = InitInstance (hInstance, nCmdShow);
	if (!g_mainWindow)
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY01CREATEDEVICE));

	MSG msg;

	g_game = std::make_unique<CGame>();
	if (!g_game->Initialize(g_mainWindow, true, true))
	{
		__debugbreak();
	}

	// Main message loop:
	while (true)
	{
		BOOL bHasMsg = PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
		if (bHasMsg)
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else /*if no message*/
		{
			g_game->Run();
		}
	}

	g_game.reset();

#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory());
#endif
	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY01CREATEDEVICE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MY01CREATEDEVICE);
	wcex.lpszClassName = g_windowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_instanceHandle = hInstance; // Store instance handle in our global variable

	HWND hWnd = CreateWindowW(g_windowClass, g_title, WS_OVERLAPPEDWINDOW,
							  CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return nullptr;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
			{
				int wmId = LOWORD(wParam);
				// Parse the menu selections:
				switch (wmId)
				{
					case IDM_ABOUT:
						DialogBox(g_instanceHandle, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
						break;
					case IDM_EXIT:
						DestroyWindow(hWnd);
						break;
					default:
						return DefWindowProc(hWnd, message, wParam, lParam);
				}
			}
			break;
		case WM_SIZE:
			{
				if (g_game)
				{
					RECT	rect;
					GetClientRect(hWnd, &rect);
					UINT wndWidth = rect.right - rect.left;
					UINT wndHeight = rect.bottom - rect.top;
					g_game->UpdateWindowSize(wndWidth, wndHeight);
				}
			}
			break;
		case WM_KEYDOWN:
			{
				if (g_game)
				{
					g_game->OnKeyDown((UINT)wParam, (UINT)((lParam >> 16) & 0xFF));
				}
			}
			break;
		case WM_KEYUP:
			{
				if (g_game)
				{
					g_game->OnKeyUp((UINT)wParam, (UINT)((lParam >> 16) & 0xFF));
				}
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);
				// TODO: Add any drawing code that uses hdc here...
				EndPaint(hWnd, &ps);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
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
