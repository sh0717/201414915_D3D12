
#include "pch.h"
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include "Resource.h"
#include "Renderer/D3D12Renderer.h"
#include "Types/typedef.h"
#include "../Util/D3DUtil.h"

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
void* g_pMeshObject0 = nullptr;
void* g_pMeshObject1 = nullptr;
void* g_pSpriteObject0 = nullptr;
void* g_pSpriteObjCommon = nullptr;

XMMATRIX g_worldMatrix0 = {};
XMMATRIX g_worldMatrix1 = {};
float g_rotation0 = 0.f;
float g_rotation1 = 0.f;

ULONGLONG g_previousFrameCheckTick = 0;
ULONGLONG g_previousUpdateTick = 0;
DWORD	g_frameCount = 0;


//dynamic texture
void* g_pDynamicTexHandle = nullptr;
BYTE* g_pImage = nullptr;
UINT g_ImageWidth = 0;
UINT g_ImageHeight = 0;
//~dynamic texture

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

	g_pMeshObject0 = CreateBoxMeshObject(g_renderer.get());
	g_pMeshObject1 = CreateQuadMeshObject(g_renderer.get());
	if (!g_pMeshObject0 || !g_pMeshObject1)
	{
		__debugbreak();
	}


	g_ImageWidth = 512;
	g_ImageHeight = 256;
	g_pImage = (BYTE*)malloc(g_ImageWidth * g_ImageHeight * 4);
	DWORD* pDest = (DWORD*)g_pImage;
	for (DWORD y = 0; y < g_ImageHeight; y++)
	{
		for (DWORD x = 0; x < g_ImageWidth; x++)
		{
			pDest[x + g_ImageWidth * y] = 0xff0000ff;
		}
	}
	g_pDynamicTexHandle = g_renderer->CreateDynamicTexture(g_ImageWidth, g_ImageHeight);

	g_pSpriteObject0 = g_renderer->CreateSpriteObject(
		L"../Resources/Image/sprite_1024x1024.dds",
		512, 512, 512, 512);
	g_pSpriteObjCommon = g_renderer->CreateSpriteObject();


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

	if (g_pMeshObject0)
	{
		g_renderer->DeleteBasicMeshObject(g_pMeshObject0);
		g_pMeshObject0 = nullptr;
	}
	if (g_pMeshObject1)
	{
		g_renderer->DeleteBasicMeshObject(g_pMeshObject1);
		g_pMeshObject1 = nullptr;
	}

	if (g_pSpriteObject0)
	{
		g_renderer->DeleteSpriteObject(g_pSpriteObject0);
		g_pSpriteObject0 = nullptr;
	}

	if (g_pImage)
	{
		free(g_pImage);
		g_pImage = nullptr;
	}

	if (g_pSpriteObjCommon)
	{
		g_renderer->DeleteSpriteObject(g_pSpriteObjCommon);
		g_pSpriteObjCommon = nullptr;
	}

	if (g_pDynamicTexHandle)
	{
		g_renderer->DeleteTexture(g_pDynamicTexHandle);
		g_pDynamicTexHandle = nullptr;
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
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = 256;
	rect.bottom = 256;
	// rendering objects
	g_renderer->RenderMeshObject(g_pMeshObject0, g_worldMatrix0);
	g_renderer->RenderMeshObject(g_pMeshObject1, g_worldMatrix1);

	g_renderer->RenderSprite(g_pSpriteObject0, 100, 80, 100, 100, 0.f);

	//render dynamic texture
	g_renderer->RenderSpriteWithTex(g_pSpriteObjCommon, 0, 300, 300, 200, nullptr, 0.f, g_pDynamicTexHandle);

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

	//scale
	XMMATRIX matScale0 = XMMatrixScaling(0.5, 0.5, 0.5);

	// rotation 
	XMMATRIX matRot0 = XMMatrixRotationX(g_rotation0);

	// translation
	XMMATRIX matTrans0 = XMMatrixTranslation(-0.55f, 0.f, 0.25f);

	// rot0 x trans0
	g_worldMatrix0 = XMMatrixMultiply(matRot0, matTrans0);
	g_worldMatrix0 = XMMatrixMultiply(matScale0, g_worldMatrix0);

	//
	// world matrix 1
	//
	g_worldMatrix1 = XMMatrixIdentity();

	// world matrix 1
	//scale
	XMMATRIX matScale1 = XMMatrixScaling(1.0, 1.0, 1.0);

	// rotation 
	XMMATRIX matRot1 = XMMatrixRotationY(g_rotation1);

	// translation
	XMMATRIX matTrans1 = XMMatrixTranslation(0.15f, 0.0f, 0.25f);

	// rot1 x trans1
	g_worldMatrix1 = XMMatrixMultiply(matRot1, matTrans1);
	g_worldMatrix1 = XMMatrixMultiply(matScale1, g_worldMatrix1);

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

	// Update Texture
	static DWORD g_dwCount = 0;
	static DWORD g_dwTileColorR = 0;
	static DWORD g_dwTileColorG = 0;
	static DWORD g_dwTileColorB = 0;

	const DWORD TILE_WIDTH = 16;
	const DWORD TILE_HEIGHT = 16;

	DWORD TILE_WIDTH_COUNT = g_ImageWidth / TILE_WIDTH;
	DWORD TILE_HEIGHT_COUNT = g_ImageHeight / TILE_HEIGHT;

	if (g_dwCount >= TILE_WIDTH_COUNT * TILE_HEIGHT_COUNT)
	{
		g_dwCount = 0;
	}
	DWORD TileY = g_dwCount / TILE_WIDTH_COUNT;
	DWORD TileX = g_dwCount % TILE_WIDTH_COUNT;

	DWORD StartX = TileX * TILE_WIDTH;
	DWORD StartY = TileY * TILE_HEIGHT;


	//DWORD r = rand() % 256;
	//DWORD g = rand() % 256;
	//DWORD b = rand() % 256;

	DWORD r = g_dwTileColorR;
	DWORD g = g_dwTileColorG;
	DWORD b = g_dwTileColorB;

	DWORD* pDest = (DWORD*)g_pImage;
	for (DWORD y = 0; y < 16; y++)
	{
		for (DWORD x = 0; x < 16; x++)
		{
			if (StartX + x >= g_ImageWidth)
				__debugbreak();

			if (StartY + y >= g_ImageHeight)
				__debugbreak();

			pDest[(StartX + x) + (StartY + y) * g_ImageWidth] = 0xff000000 | (b << 16) | (g << 8) | r;
		}
	}
	g_dwCount++;
	g_dwTileColorR += 8;
	if (g_dwTileColorR > 255)
	{
		g_dwTileColorR = 0;
		g_dwTileColorG += 8;
	}
	if (g_dwTileColorG > 255)
	{
		g_dwTileColorG = 0;
		g_dwTileColorB += 8;
	}
	if (g_dwTileColorB > 255)
	{
		g_dwTileColorB = 0;
	}
	g_renderer->UpdateTextureWithImage(g_pDynamicTexHandle, g_pImage, g_ImageWidth, g_ImageHeight);
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
