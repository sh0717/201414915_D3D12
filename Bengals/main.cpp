
#include "pch.h"
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include "Resource.h"
#include "Renderer/D3D12Renderer.h"

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
HINSTANCE g_instanceHandle = nullptr;                                // current instance
HWND g_mainWindow = nullptr;
WCHAR g_title[MAX_LOADSTRING];                  // The title bar text
WCHAR g_windowClass[MAX_LOADSTRING];            // the main window class name

std::unique_ptr<CD3D12Renderer> g_renderer = nullptr;
void* g_pMeshObject = nullptr;
void* g_pTextureHandle0 = nullptr;
void* g_pTextureHandle1 = nullptr;

float g_offsetX = 0.0f;
float g_offsetY = 0.0f;
float g_speed = 0.015f;

XMMATRIX g_worldMatrix0 = {};
XMMATRIX g_worldMatrix1 = {};
float g_rotation0 = 0.f;
float g_rotation1 = 0.f;

ULONGLONG g_previousFrameCheckTick = 0;
ULONGLONG g_previousUpdateTick = 0;
DWORD	g_frameCount = 0;

static void RunGame();
static void Update();

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

	g_renderer  = std::make_unique<CD3D12Renderer>(g_mainWindow, true, true);
	if (!g_renderer)
	{
		__debugbreak();
	}
	
	g_pMeshObject = g_renderer->CreateBasicMeshObject();
	g_pTextureHandle0 = g_renderer->CreateTiledTexture(16, 16, 192, 128, 255);
	g_pTextureHandle1 = g_renderer->CreateTiledTexture(32, 32, 128, 255, 192);
	// Main message loop:
	//while (GetMessage(&msg, nullptr, 0, 0))
	//{
	//	if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
	//	{
	//		TranslateMessage(&msg);
	//		DispatchMessage(&msg);
	//	}
	//}
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
			RunGame();
		}
	}


	if (g_pMeshObject)
	{
		g_renderer->DeleteBasicMeshObject(g_pMeshObject);
		g_pMeshObject = nullptr;
	}

	if (g_pTextureHandle0)
	{
		g_renderer->DeleteTexture(g_pTextureHandle0);
		g_pTextureHandle0 = nullptr;
	}
	if (g_pTextureHandle1)
	{
		g_renderer->DeleteTexture(g_pTextureHandle1);
		g_pTextureHandle1 = nullptr;
	}


	g_renderer.reset();

#ifdef _DEBUG
	_ASSERT(_CrtCheckMemory());
#endif
	return (int)msg.wParam;
}

void RunGame()
{
	g_frameCount++;

	g_renderer->BeginRender();
	LONGLONG CurTick = GetTickCount64();
	// game business logic

	if (CurTick - g_previousUpdateTick > 16)
	{
		// Update Scene with 60FPS
		Update();
		g_previousUpdateTick = CurTick;
	}

	// rendering objects
	//g_renderer->RenderMeshObject(g_pMeshObject, g_offsetX, g_offsetY);
	g_renderer->RenderMeshObject(g_pMeshObject, g_worldMatrix0, g_pTextureHandle0);
	g_renderer->RenderMeshObject(g_pMeshObject, g_worldMatrix1, g_pTextureHandle1);
	g_renderer->EndRender();
	g_renderer->Present();

	if (CurTick - g_previousFrameCheckTick > 1000)
	{
		g_previousFrameCheckTick = CurTick;

		WCHAR wchTxt[64];
		swprintf_s(wchTxt, L"FPS:%u", g_frameCount);
		SetWindowText(g_mainWindow, wchTxt);

		g_frameCount = 0;
	}
}

void Update()
{

	//
	// world matrix 0
	//
	g_worldMatrix0 = XMMatrixIdentity();

	// rotation 
	XMMATRIX matRot0 = XMMatrixRotationX(g_rotation0);

	// translation
	XMMATRIX matTrans0 = XMMatrixTranslation(-0.15f, 0.0f, 0.25f);

	// rot0 x trans0
	g_worldMatrix0 = XMMatrixMultiply(matRot0, matTrans0);

	//
	// world matrix 1
	//
	g_worldMatrix1 = XMMatrixIdentity();

	// world matrix 1
	// rotation 
	XMMATRIX matRot1 = XMMatrixRotationY(g_rotation1);

	// translation
	XMMATRIX matTrans1 = XMMatrixTranslation(0.15f, 0.0f, 0.25f);

	// rot1 x trans1
	g_worldMatrix1 = XMMatrixMultiply(matRot1, matTrans1);

	BOOL	bChangeTex = FALSE;
	g_rotation0 += 0.05f;
	if (g_rotation0 > 2.0f * 3.1415f)
	{
		g_rotation0 = 0.0f;
		bChangeTex = TRUE;
	}

	g_rotation1 += 0.1f;
	if (g_rotation1 > 2.0f * 3.1415f)
	{
		g_rotation1 = 0.0f;
	}
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
		case WM_SIZE :
			{
				if (g_renderer)
				{
					RECT	rect;
					GetClientRect(hWnd, &rect);
					DWORD	dwWndWidth = rect.right - rect.left;
					DWORD	dwWndHeight = rect.bottom - rect.top;
					g_renderer->UpdateWindowSize(dwWndWidth, dwWndHeight);
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

