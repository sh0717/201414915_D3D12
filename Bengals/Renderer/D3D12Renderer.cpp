#include "pch.h"
#include "D3D12Renderer.h"
#include <cassert>
#include "../../Util/D3DUtil.h"
#include <dxgi.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <functional>
#include <iostream>

#include "ResourceManager/CD3D12ResourceManager.h"
#include "RenderHelper/ConstantBufferPool.h"
#include "RenderHelper/ShaderVisibleDescriptorTableAllocator.h"
#include "RenderHelper/DescriptorAllocator.h"

#include "BasicMeshObject/BasicMeshObject.h"
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
		UINT featureLevelNum = _countof(featureLevels);

		bool bDeviceCreated = false;
		for (UINT index = 0; index < featureLevelNum; index++)
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
	UINT	dwBackBufferWidth = ClientRect.right - ClientRect.left;
	UINT	dwBackBufferHeight = ClientRect.bottom - ClientRect.top;

	//Make swap chain 

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
	SwapChainDesc.Width = dwBackBufferWidth;
	SwapChainDesc.Height = dwBackBufferHeight;
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

	m_pResourceManager = new CD3D12ResourceManager{};
	m_pResourceManager->Initialize(m_pD3DDevice);

	m_pDescriptorPool = std::make_unique<CShaderVisibleDescriptorTableAllocator>();
	m_pDescriptorPool->Initialize(m_pD3DDevice, MAX_DRAW_COUNT_PER_FRAME* CBasicMeshObject::DescriptorCountPerDraw);

	m_pConstantBufferPool = std::make_unique<CConstantBufferPool>();
	m_pConstantBufferPool->Initialize(m_pD3DDevice, static_cast<UINT>(AlignConstantBufferSize(sizeof(ConstantBufferDefault))), MAX_DRAW_COUNT_PER_FRAME);

	m_pDescriptorAllocator = new CDescriptorAllocator{};
	m_pDescriptorAllocator->Initialize(m_pD3DDevice, MAX_DESCRIPTOR_COUNT);

	return bResult = true;
}

void CD3D12Renderer::BeginRender()
{
	if (FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
	}

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE RTVDescriptorHandle(m_pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentRenderTargetIndex, m_rtvDescriptorSize);

	CD3DX12_RESOURCE_BARRIER RenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition (m_pRenderTargets[m_currentRenderTargetIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->ResourceBarrier(1, &RenderTargetBarrier);

	const float BackColor[] = { 1.0f, 0.0f, 1.0f, 1.0f };
	D3D12_CPU_DESCRIPTOR_HANDLE DsvDescriptorHandle{ m_pDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()};

	m_pCommandList->ClearRenderTargetView(RTVDescriptorHandle, BackColor, 0, nullptr);
	m_pCommandList->ClearDepthStencilView(DsvDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
	m_pCommandList->RSSetViewports(1, &m_viewport);
	m_pCommandList->RSSetScissorRects(1, &m_scissorRect);
	m_pCommandList->OMSetRenderTargets(1, &RTVDescriptorHandle, FALSE, &DsvDescriptorHandle);
	/*==========================================================================================*/
}
void CD3D12Renderer::EndRender()
{
	/*==========================================================================================*/
	CD3DX12_RESOURCE_BARRIER RenderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_currentRenderTargetIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &RenderTargetBarrier);
	m_pCommandList->Close();
	
	ID3D12CommandList* pCommandLists[] = { m_pCommandList };
    m_pCommandQueue->ExecuteCommandLists(_countof(pCommandLists), pCommandLists);
}

void CD3D12Renderer::Present()
{
	//UINT m_SyncInterval = 1;	// VSync On
	UINT syncInterval = 0;	// VSync Off

	UINT uiSyncInterval = syncInterval;
	UINT uiPresentFlags = 0;

	if (!uiSyncInterval)
	{
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);
	
	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}
	
    m_currentRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	SetFence();
	WaitForFenceValue();

	m_pConstantBufferPool->Reset();
	m_pDescriptorPool->Reset();
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
	pMeshObject->CreateMesh();
	return pMeshObject;
}

void CD3D12Renderer::RenderMeshObject(void* pMeshObjectHandle, const XMMATRIX& worldMatrix, void* pTextureHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle = {};
	if (pTextureHandle)
	{
		srvDescriptorHandle = ((TextureHandle*)pTextureHandle)->SrvDescriptorHandle;
	}

	CBasicMeshObject* pMeshObj = (CBasicMeshObject*)pMeshObjectHandle;
	pMeshObj->Draw(m_pCommandList, worldMatrix, srvDescriptorHandle);
}

void CD3D12Renderer::DeleteBasicMeshObject(void* pMeshObjectHandle)
{
	CBasicMeshObject* pMeshObject = (CBasicMeshObject*)pMeshObjectHandle;
	delete pMeshObject;
}

void* CD3D12Renderer::CreateTiledTexture(UINT texWidth, UINT texHeight, BYTE r, BYTE g, BYTE b)
{
	TextureHandle* pTextureHandle = nullptr;

	BOOL bResult = FALSE;
	ID3D12Resource* pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};


	DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	BYTE* pImage = (BYTE*)malloc(texWidth * texHeight * 4);
	memset(pImage, 0, texWidth * texHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;

	for (UINT y = 0; y < texHeight; y++)
	{
		for (UINT x = 0; x < texWidth; x++)
		{

			URGBA* pDest = (URGBA*)(pImage + (x + y * texWidth) * 4);

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
	if (m_pResourceManager->CreateTexture(&pTexResource, texWidth, texHeight, texFormat, pImage))
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = texFormat;
		SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;

		if (m_pDescriptorAllocator->Allocate(&srv))
		{
			m_pD3DDevice->CreateShaderResourceView(pTexResource, &SRVDesc, srv);

			pTextureHandle = new TextureHandle;
			pTextureHandle->TextureResource = pTexResource;
			pTextureHandle->SrvDescriptorHandle = srv;
			bResult = TRUE;
		}
		else
		{
			pTexResource->Release();
			pTexResource = nullptr;
		}
	}
	free(pImage);
	pImage = nullptr;

	return pTextureHandle;
}

void CD3D12Renderer::DeleteTexture(void* pTextureHandle)
{
	TextureHandle* pTextureData = (TextureHandle*)pTextureHandle;

	ID3D12Resource* pTexResource = pTextureData->TextureResource;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = pTextureData->SrvDescriptorHandle;

	if (pTexResource)
	{
		pTexResource->Release();
	}
	m_pDescriptorAllocator->Free(srv);	

	delete pTextureData;
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
	return m_fenceValue;
}

void CD3D12Renderer::WaitForFenceValue() const
{
	const UINT64 expectedFenceValue = m_fenceValue;

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

	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator))))
	{
		__debugbreak();
	}

	// Create the command list.
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList))))
	{
		__debugbreak();
	}

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	m_pCommandList->Close();

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
	// ī�޶� ��ġ, ī�޶� ����, ���� ������ ����
	XMVECTOR eyePos = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f); // ī�޶� ��ġ
	XMVECTOR eyeDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // ī�޶� ���� (������ ���ϵ��� ����)
	XMVECTOR upDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // ���� ���� (�Ϲ������� y���� ���� ����)

	// view matrix
	m_viewMatrix = XMMatrixLookToLH(eyePos, eyeDir, upDir);

	// �þ߰� (FOV) ���� (���� ����)
	float fovY = XM_PIDIV4; // 90�� (�������� ��ȯ)

	// projection matrix
	float fAspectRatio = (float)m_viewportWidth / (float)m_viewportHeight;
	float fNear = 0.1f;
	float fFar = 1000.0f;
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNear, fFar);
}

void CD3D12Renderer::Cleanup()
{
	WaitForFenceValue();

	m_pDescriptorPool = nullptr;
	m_pConstantBufferPool = nullptr;

	if (m_pResourceManager)
	{
		delete m_pResourceManager;
		m_pResourceManager = nullptr;
	}

	if (m_pDescriptorAllocator)
	{
		delete m_pDescriptorAllocator;
		m_pDescriptorAllocator = nullptr;
	}

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
	if (m_pCommandList)
	{
		m_pCommandList->Release();
		m_pCommandList = nullptr;
	}
	if (m_pCommandAllocator)
	{
		m_pCommandAllocator->Release();
		m_pCommandAllocator = nullptr;
	}
}
