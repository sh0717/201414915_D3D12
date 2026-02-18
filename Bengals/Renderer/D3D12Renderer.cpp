#include "pch.h"
#include "D3D12Renderer.h"
#include <cassert>
#include "../../Util/D3DUtil.h"
#include <dxgi.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <functional>
#include <iostream>

#include "Manager/CD3D12ResourceManager.h"
#include "RenderHelper/ConstantBufferPool.h"
#include "RenderHelper/ConstantBufferManager.h"
#include "RenderHelper/GpuDescriptorLinearAllocator.h"
#include "RenderHelper/CpuDescriptorFreeListAllocator.h"

#include "RenderObject/BasicMeshObject.h"
#include "RenderObject/SpriteObject.h"
#include "Manager/TextureManager.h"
#include "../Types/typedef.h"

CD3D12Renderer::CD3D12Renderer(HWND hWindow, bool bEnableDebugLayer, bool bEnableGbv)
{
	if (Initialize(hWindow, bEnableDebugLayer, bEnableGbv) == false)
	{
		DebugLog("D3D12 Renderer init failed!!!");
		__debugbreak();
	}
}

CD3D12Renderer::~CD3D12Renderer()
{
	Cleanup();
}

bool CD3D12Renderer::Initialize(HWND hWindow, bool bEnableDebugLayer, bool bEnableGbv)
{
	bool bResult = false;

	ComPtr<ID3D12Debug>	DebugController = nullptr;
	ComPtr<IDXGIFactory4> Factory = nullptr;

	DWORD createFactoryFlags = 0;

	if (bEnableDebugLayer)
	{
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(DebugController.ReleaseAndGetAddressOf()))))
		{
			assert(DebugController);
			DebugController->EnableDebugLayer();
		}

		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		if (bEnableGbv)
		{
			ID3D12Debug5* pDebugController5 = nullptr;
			if (SUCCEEDED(DebugController->QueryInterface(IID_PPV_ARGS(&pDebugController5))))
			{
				assert(pDebugController5);

				pDebugController5->SetEnableGPUBasedValidation(TRUE);
				pDebugController5->SetEnableAutoName(TRUE);
				pDebugController5->Release();
			}
		}
	}

	if (SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(Factory.ReleaseAndGetAddressOf()))))
	{
		assert(Factory != nullptr && "DXGI Factory is not valid");

		D3D_FEATURE_LEVEL	featureLevels[] =
		{	D3D_FEATURE_LEVEL_12_2,
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};
		UINT featureLevelCount = _countof(featureLevels);

		bool bDeviceCreated = false;
		for (UINT index = 0; index < featureLevelCount; index++)
		{
			if (bDeviceCreated)
			{
				break;
			}

			UINT adapterIndex = 0;
			ComPtr<IDXGIAdapter1> DXGIAdapter = nullptr;
			while (Factory->EnumAdapters1(adapterIndex, DXGIAdapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND)
			{
				if (DXGIAdapter == nullptr)
				{
					continue;;
				}

				DXGI_ADAPTER_DESC1 AdapterDesc{};
				DXGIAdapter->GetDesc1(&AdapterDesc);

				if (SUCCEEDED(D3D12CreateDevice(DXGIAdapter.Get(), featureLevels[index], IID_PPV_ARGS(&m_pD3DDevice))))
				{
					bDeviceCreated = true;
					break;
				}

				adapterIndex++;
			}
		}
	}

	m_windowHandle = hWindow;

	if (m_pD3DDevice == nullptr)
	{
		__debugbreak();
		return false;
	}

	if (DebugController)
	{
		SetDebugLayerInfo(m_pD3DDevice);
	}

	D3D12_COMMAND_QUEUE_DESC CommandQueueDesc = {};
	CommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	CommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if (FAILED(m_pD3DDevice->CreateCommandQueue(&CommandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue))) || m_pCommandQueue == nullptr)
	{
		__debugbreak();
		return false;
	}


	RECT	ClientRect;
	::GetClientRect(hWindow, &ClientRect);
	DWORD	wndWidth = ClientRect.right - ClientRect.left;
	DWORD	wndHeight = ClientRect.bottom - ClientRect.top;
	UINT	backBufferWidth = ClientRect.right - ClientRect.left;
	UINT	backBufferHeight = ClientRect.bottom - ClientRect.top;

	//Make swap chain

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = backBufferWidth;
	SwapChainDesc.Height = backBufferHeight;
	SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//swapChainDesc.BufferDesc.RefreshRate.Numerator = m_uiRefreshRate;
	//swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.Scaling = DXGI_SCALING_NONE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	SwapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	m_swapChainFlags = SwapChainDesc.Flags;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullscreenDesc = {};
	swapChainFullscreenDesc.Windowed = TRUE;

	ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
	if (FAILED(Factory->CreateSwapChainForHwnd(m_pCommandQueue, hWindow, &SwapChainDesc, &swapChainFullscreenDesc, nullptr, pSwapChain1.ReleaseAndGetAddressOf())))
	{
		__debugbreak();
		return false;
	}
	pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));

	assert(m_pSwapChain != nullptr && "CD3D12Renderer::Initialize , Swap chain 3 is not valid ");
	m_currentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();
	//~ Make swap chain

	// set viewport and scissor rect
	m_viewport.Width = static_cast<float>(wndWidth);
	m_viewport.Height = static_cast<float>(wndHeight);
	m_viewport.MaxDepth = 1.f;
	m_viewport.MinDepth = 0.f;

	m_scissorRect.left = 0;
	m_scissorRect.right = wndWidth;
	m_scissorRect.top = 0;
	m_scissorRect.bottom = wndHeight;

	m_viewportWidth = wndWidth;
	m_viewportHeight = wndHeight;
	// ~set viewport and scissor rect

	CreateRTVAndDSVDescriptorHeap();

	//create RTV descriptor
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHandle{ m_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pSwapChain->GetBuffer(n, IID_PPV_ARGS(&m_pRenderTargets[n]));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[n], nullptr, RTVDescriptorHandle);
		RTVDescriptorHandle.Offset(1, m_rtvDescriptorSize);
	}

	CreateDepthStencil(wndWidth, wndHeight);
	CreateCommandAllocatorAndCommandList();
	CreateFence();

	InitializeCamera();

	m_resourceManager = std::make_unique<CD3D12ResourceManager>();
	m_resourceManager->Initialize(m_pD3DDevice);

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		FrameContext& ctx = m_frameContexts[i];

		ctx.DescriptorPool = std::make_unique<CGpuDescriptorLinearAllocator>();
		ctx.DescriptorPool->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME * CBasicMeshObject::MaxDescriptorCountForDraw);

		ctx.ConstantBufferManager = std::make_unique<CConstantBufferManager>();
		ctx.ConstantBufferManager->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME);
	}

	m_descriptorAllocator = std::make_unique<CCpuDescriptorFreeListAllocator>();
	m_descriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPTOR_COUNT);

	m_textureManager = std::make_unique<CTextureManager>();
	m_textureManager->Initialize(this);

	return bResult = true;
}

void CD3D12Renderer::BeginRender()
{
	FrameContext& ctx = m_frameContexts[m_currentContextIndex];

	if (FAILED(ctx.pCommandAllocator->Reset()))
	{
		__debugbreak();
	}

	if (FAILED(ctx.pCommandList->Reset(ctx.pCommandAllocator, nullptr)))
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHandle(m_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentRenderTargetIndex, m_rtvDescriptorSize);

	CD3DX12_RESOURCE_BARRIER RenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_currentRenderTargetIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	ctx.pCommandList->ResourceBarrier(1, &RenderTargetBarrier);

	const float BackColor[] = { 1.0f, 0.0f, 1.0f, 1.0f };
	D3D12_CPU_DESCRIPTOR_HANDLE DsvDescriptorHandle{ m_pDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

	ctx.pCommandList->ClearRenderTargetView(RTVDescriptorHandle, BackColor, 0, nullptr);
	ctx.pCommandList->ClearDepthStencilView(DsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	ctx.pCommandList->RSSetViewports(1, &m_viewport);
	ctx.pCommandList->RSSetScissorRects(1, &m_scissorRect);
	ctx.pCommandList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, &DsvDescriptorHandle);
	/*==========================================================================================*/
}
void CD3D12Renderer::EndRender()
{
	FrameContext& ctx = m_frameContexts[m_currentContextIndex];

	/*==========================================================================================*/
	CD3DX12_RESOURCE_BARRIER RenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_currentRenderTargetIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	ctx.pCommandList->ResourceBarrier(1, &RenderTargetBarrier);
	ctx.pCommandList->Close();

	ID3D12CommandList* pCommandLists[] = { ctx.pCommandList };
    m_pCommandQueue->ExecuteCommandLists(_countof(pCommandLists), pCommandLists);
}

void CD3D12Renderer::Present()
{
	//UINT m_SyncInterval = 1;	// VSync On
	UINT syncInterval = 0;	// VSync Off
	UINT presentFlags = 0;

	if (!syncInterval)
	{
		presentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	// Fence를 Present 앞에 호출: 렌더링 커맨드 완료 시점만 정확히 추적
	SetFence();

	HRESULT hr = m_pSwapChain->Present(syncInterval, presentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}

    m_currentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// 다음 컨텍스트의 이전 GPU 작업 완료 대기
	DWORD nextContextIndex = (m_currentContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	FrameContext& nextCtx = m_frameContexts[nextContextIndex];
	WaitForFenceValue(nextCtx.LastFenceValue);

	// 다음 컨텍스트의 풀 리셋
	nextCtx.ConstantBufferManager->Reset();
	nextCtx.DescriptorPool->Reset();

	// 컨텍스트 전환
	m_currentContextIndex = nextContextIndex;
}

bool CD3D12Renderer::UpdateWindowSize(UINT backBufferWidth, UINT backBufferHeight)
{
	bool bResult = false;

	if (backBufferWidth <= 0 || backBufferHeight <= 0 )
	{
		return bResult;
	}

	if (m_viewportWidth == backBufferWidth && m_viewportHeight == backBufferHeight)
	{
		return bResult;
	}

	// 모든 컨텍스트의 GPU 작업 완료 대기
	SetFence();
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_frameContexts[i].LastFenceValue);
	}

	DXGI_SWAP_CHAIN_DESC1	desc;
	HRESULT	hr = m_pSwapChain->GetDesc1(&desc);
	if (FAILED(hr))
		__debugbreak();

	for (UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++)
	{
		m_pRenderTargets[n]->Release();
		m_pRenderTargets[n] = nullptr;
	}

	if (m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = nullptr;
	}

	if (FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, backBufferWidth, backBufferHeight, DXGI_FORMAT_R8G8B8A8_UNORM, m_swapChainFlags)))
	{
		__debugbreak();
	}
	m_currentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Create frame resources.
	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVHandle(m_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create a RTV for each frame.
	for (UINT Index = 0; Index < SWAP_CHAIN_FRAME_COUNT; ++Index)
	{
		m_pSwapChain->GetBuffer(Index, IID_PPV_ARGS(&m_pRenderTargets[Index]));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[Index], nullptr, RTVHandle);
		RTVHandle.Offset(1, m_rtvDescriptorSize);
	}
	CreateDepthStencil(backBufferWidth, backBufferHeight);

	m_viewportWidth = backBufferWidth;
	m_viewportHeight = backBufferHeight;
	m_viewport.Width = static_cast<FLOAT>(m_viewportWidth);
	m_viewport.Height = static_cast<FLOAT>(m_viewportHeight);

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_viewportWidth;
	m_scissorRect.bottom = m_viewportHeight;

	InitializeCamera();

	return bResult = true;;
}

void* CD3D12Renderer::CreateBasicMeshObject()
{
	CBasicMeshObject* pMeshObject = new CBasicMeshObject{this};
	return pMeshObject;
}

bool CD3D12Renderer::BeginCreateMesh(void* pMeshObjectHandle, const void* pVertexList, UINT vertexCount, UINT vertexSize, UINT triGroupCount)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjectHandle;
	return pMeshObj->BeginCreateMesh(pVertexList, vertexCount, vertexSize, triGroupCount);
}

bool CD3D12Renderer::InsertTriGroup(void* pMeshObjectHandle, const WORD* pIndexList, UINT triCount, const WCHAR* wchTexFileName)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjectHandle;
	return pMeshObj->InsertIndexedTriList(pIndexList, triCount, wchTexFileName);
}

void CD3D12Renderer::EndCreateMesh(void* pMeshObjectHandle)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjectHandle;
	pMeshObj->EndCreateMesh();
}

void CD3D12Renderer::RenderMeshObject(void* pMeshObjectHandle, const XMMATRIX& worldMatrix)
{
	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjectHandle;
	pMeshObj->Draw(m_frameContexts[m_currentContextIndex].pCommandList, worldMatrix);
}

void CD3D12Renderer::DeleteBasicMeshObject(void* pMeshObjectHandle)
{
	// wait for all commands
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_frameContexts[i].LastFenceValue);
	}

	CBasicMeshObject* pMeshObject = (CBasicMeshObject*)pMeshObjectHandle;
	delete pMeshObject;
}

void* CD3D12Renderer::CreateSpriteObject()
{
	CSpriteObject* pSpriteObject = new CSpriteObject(this);
	return pSpriteObject;
}

void* CD3D12Renderer::CreateSpriteObject(const WCHAR* wchTexFileName, int posX, int posY, int width, int height)
{
	RECT rect = {};
	RECT* pRect = nullptr;

	if (width > 0 && height > 0)
	{
		rect.left = posX;
		rect.top = posY;
		rect.right = posX + width;
		rect.bottom = posY + height;
		pRect = &rect;
	}

	CSpriteObject* pSpriteObject = new CSpriteObject(this, wchTexFileName, pRect);
	return pSpriteObject;
}

void CD3D12Renderer::DeleteSpriteObject(void* pSpriteObjectHandle)
{
	// wait for all commands
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_frameContexts[i].LastFenceValue);
	}

	CSpriteObject* pSpriteObject = (CSpriteObject*)pSpriteObjectHandle;
	delete pSpriteObject;
}

void CD3D12Renderer::RenderSpriteWithTex(void* pSpriteObjectHandle, int posX, int posY, int width, int height, const RECT* pRect, float z, void* pTexHandle)
{
	CSpriteObject* pSpriteObject = (CSpriteObject*)pSpriteObjectHandle;
	TextureHandle* pTextureHandle = (TextureHandle*)pTexHandle;
	if (!pSpriteObject || !pTextureHandle)
	{
		return;
	}

	if (pTextureHandle->pUploadBuffer)
	{
		if (pTextureHandle->bUpdated)
		{
			UpdateTexture(m_pD3DDevice, m_frameContexts[m_currentContextIndex].pCommandList, pTextureHandle->TextureResource, pTextureHandle->pUploadBuffer);
		}
	}

	XMFLOAT2 pos = { static_cast<float>(posX), static_cast<float>(posY) };
	XMFLOAT2 pixelSize = { static_cast<float>(width), static_cast<float>(height) };
	pSpriteObject->DrawWithTex(m_frameContexts[m_currentContextIndex].pCommandList, pos, pixelSize, pRect, z, pTextureHandle);
}

void CD3D12Renderer::RenderSprite(void* pSpriteObjectHandle, int posX, int posY, int width, int height, float z)
{
	CSpriteObject* pSpriteObject = (CSpriteObject*)pSpriteObjectHandle;
	if (!pSpriteObject)
	{
		return;
	}

	XMFLOAT2 pos = { static_cast<float>(posX), static_cast<float>(posY) };
	XMFLOAT2 pixelSize = { static_cast<float>(width), static_cast<float>(height) };
	pSpriteObject->Draw(m_frameContexts[m_currentContextIndex].pCommandList, pos, pixelSize, z);
}

void CD3D12Renderer::UpdateTextureWithImage(void* pTexHandle, const BYTE* pSrcBits, UINT SrcWidth, UINT SrcHeight)
{
	m_textureManager->UpdateTextureWithImage((TextureHandle*)pTexHandle, pSrcBits, SrcWidth, SrcHeight);
}

void* CD3D12Renderer::CreateTiledTexture(UINT texWidth, UINT texHeight, BYTE r, BYTE g, BYTE b)
{
	DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	BYTE* pImage = (BYTE*)malloc(texWidth * texHeight * 4);
	memset(pImage, 0, texWidth * texHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;

	for (UINT y = 0; y < texHeight; y++)
	{
		for (UINT x = 0; x < texWidth; x++)
		{

			RGBA* pDest = (RGBA*)(pImage + (x + y * texWidth) * 4);

			if ((bFirstColorIsWhite + x) % 2)
			{
				pDest->R = r;
				pDest->G = g;
				pDest->B = b;
			}
			else
			{
				pDest->R = 0;
				pDest->G = 0;
				pDest->B = 0;
			}
			pDest->A = 255;
		}
		bFirstColorIsWhite++;
		bFirstColorIsWhite %= 2;
	}
	TextureHandle* pTexHandle = m_textureManager->CreateStaticTexture(texWidth, texHeight, textureFormat, pImage);

	free(pImage);
	pImage = nullptr;

	return pTexHandle;
}

void* CD3D12Renderer::CreateDynamicTexture(UINT texWidth, UINT texHeight)
{
	return m_textureManager->CreateDynamicTexture(texWidth, texHeight);
}

void* CD3D12Renderer::CreateTextureFromFile(const WCHAR* filePath)
{
	return m_textureManager->CreateTextureFromFile(filePath);
}

void CD3D12Renderer::DeleteTexture(void* pTextureHandle)
{
	// GPU 작업 완료 대기 (fence는 renderer 소유)
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_frameContexts[i].LastFenceValue);
	}

	m_textureManager->DeleteTexture((TextureHandle*)pTextureHandle);
}

void CD3D12Renderer::GetViewProjMatrix(XMMATRIX* pOutViewMatrix, XMMATRIX* pOutProjMatrix)
{
	*pOutViewMatrix = m_viewMatrix;
	*pOutProjMatrix = m_projectionMatrix;
}


UINT64 CD3D12Renderer::SetFence()
{
	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence, m_fenceValue);
	m_frameContexts[m_currentContextIndex].LastFenceValue = m_fenceValue;
	return m_fenceValue;
}

void CD3D12Renderer::WaitForFenceValue(uint64_t expectedFenceValue) const
{
	if (m_pFence->GetCompletedValue() < expectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(expectedFenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

bool CD3D12Renderer::CreateCommandAllocatorAndCommandList()
{
	assert(m_pD3DDevice != nullptr && "CD3D12Renderer::CreateCommandAllocatorAndCommandList, m_pD3DDevice is not valid");

	bool bResult = false;

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		FrameContext& ctx = m_frameContexts[i];

		if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&ctx.pCommandAllocator))))
		{
			__debugbreak();
		}

		if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, ctx.pCommandAllocator, nullptr, IID_PPV_ARGS(&ctx.pCommandList))))
		{
			__debugbreak();
		}

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ctx.pCommandList->Close();
	}

	bResult = true;

	return bResult;
}

void CD3D12Renderer::CreateFence()
{
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence))))
	{
		__debugbreak();
	}

	m_fenceValue = 0;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

void CD3D12Renderer::CreateRTVAndDSVDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC RTVDescriptorHeapDesc = {};
	RTVDescriptorHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;
	RTVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RTVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&RTVDescriptorHeapDesc, IID_PPV_ARGS(&m_pRtvDescriptorHeap))))
	{
		__debugbreak();
	}

	m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC DSVDescriptorHeapDesc = {};
	DSVDescriptorHeapDesc.NumDescriptors = 1;	// Default Depth Buffer
	DSVDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	DSVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&DSVDescriptorHeapDesc, IID_PPV_ARGS(&m_pDsvDescriptorHeap))))
	{
		__debugbreak();
	}

	m_dsvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	return;
}

bool CD3D12Renderer::CreateDepthStencil(UINT width, UINT height)
{
	bool bResult = false;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC DepthDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		width,
		height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);



	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES{ D3D12_HEAP_TYPE_DEFAULT },
		D3D12_HEAP_FLAG_NONE,
		&DepthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_pDepthStencilBuffer)
	)))
	{
		__debugbreak();
	}

	m_pDepthStencilBuffer->SetName(L"CD3D12Renderer::DepthStencilBuffer");

	D3D12_DEPTH_STENCIL_VIEW_DESC DepthStencilViewDesc = {};
	DepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	DepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	DSVDescriptorHandle(m_pDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &DepthStencilViewDesc, DSVDescriptorHandle);

	return bResult;
}

void CD3D12Renderer::InitializeCamera()
{
	XMVECTOR eyePos = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f);
	XMVECTOR eyeDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// view matrix
	m_viewMatrix = XMMatrixLookToLH(eyePos, eyeDir, upDir);

	float fovY = XM_PIDIV4;

	// projection matrix
	float fAspectRatio = (float)m_viewportWidth / (float)m_viewportHeight;
	float fNear = 0.1f;
	float fFar = 1000.0f;
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);
}

void CD3D12Renderer::Cleanup()
{
	// 모든 컨텍스트의 GPU 작업 완료 보장
	SetFence();
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		WaitForFenceValue(m_frameContexts[i].LastFenceValue);
	}

	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		m_frameContexts[i].DescriptorPool = nullptr;
		m_frameContexts[i].ConstantBufferManager = nullptr;
	}

	m_textureManager = nullptr;
	m_resourceManager = nullptr;
	m_descriptorAllocator = nullptr;

	CleanupDescriptorHeap();
	for (DWORD i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		if (m_pRenderTargets[i])
		{
			m_pRenderTargets[i]->Release();
			m_pRenderTargets[i] = nullptr;
		}
	}

	if (m_pDepthStencilBuffer)
	{
		m_pDepthStencilBuffer->Release();
		m_pDepthStencilBuffer = nullptr;
	}

	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}

	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}

	CleanupCommandAllocatorAndCommandList();
	CleanupFence();

	if (m_pD3DDevice)
	{
		ULONG refCount = m_pD3DDevice->Release();
		if (refCount)
		{
			//resource leak
			IDXGIDebug1* pDebug = nullptr;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
			{
				pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
				pDebug->Release();
			}
			__debugbreak();
		}

		m_pD3DDevice = nullptr;
	}
}

void CD3D12Renderer::CleanupFence()
{
	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}

	m_fenceValue = 0;
}

void CD3D12Renderer::CleanupDescriptorHeap()
{
	if (m_pRtvDescriptorHeap)
	{
		m_pRtvDescriptorHeap->Release();
		m_pRtvDescriptorHeap = nullptr;
	}

	if (m_pDsvDescriptorHeap)
	{
		m_pDsvDescriptorHeap->Release();
		m_pDsvDescriptorHeap = nullptr;
	}
}

void CD3D12Renderer::CleanupCommandAllocatorAndCommandList()
{
	for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		FrameContext& ctx = m_frameContexts[i];
		if (ctx.pCommandList)
		{
			ctx.pCommandList->Release();
			ctx.pCommandList = nullptr;
		}
		if (ctx.pCommandAllocator)
		{
			ctx.pCommandAllocator->Release();
			ctx.pCommandAllocator = nullptr;
		}
	}
}
