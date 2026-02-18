#pragma once

/**
 * D3DDevice, command allocator , commandlist , swap chain 등등 초기화시에 잡고
 * 그 이후에는 null check 생략
 */

class CD3D12ResourceManager;
class CCpuDescriptorFreeListAllocator;
class CConstantBufferManager;
class CTextureManager;

#include "RenderHelper/GpuDescriptorLinearAllocator.h"
#include "RenderHelper/ConstantBufferManager.h"

struct FrameContext
{

	ID3D12CommandAllocator* pCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* pCommandList = nullptr;
	std::unique_ptr<CGpuDescriptorLinearAllocator> DescriptorPool = nullptr;
	std::unique_ptr<CConstantBufferManager> ConstantBufferManager = nullptr;
	uint64_t LastFenceValue = 0;
};

class CD3D12Renderer
{
public:/*function*/
	CD3D12Renderer() = delete;
	CD3D12Renderer(HWND hWindow, bool bEnableDebugLayer = true, bool bEnableGbv = true);
	virtual ~CD3D12Renderer();

	void BeginRender();
	void EndRender();
	void Present();

	/* Getter function*/

	ID3D12Device5* GetD3DDevice() const
	{
		return m_pD3DDevice;
	}

	CD3D12ResourceManager* GetResourceManager() const
	{
		return m_resourceManager.get();
	}

	CGpuDescriptorLinearAllocator* GetDescriptorPool() const
	{
		return m_frameContexts[m_currentContextIndex].DescriptorPool.get();
	}

	CCpuDescriptorFreeListAllocator* GetDescriptorAllocator() const
	{
		return m_descriptorAllocator.get();
	}

	CConstantBufferPool* GetConstantBufferPool(ConstantBufferType type) const
	{
		return m_frameContexts[m_currentContextIndex].ConstantBufferManager->GetConstantBufferPool(type);
	}

	UINT GetScreenWidth() const
	{
		return m_viewportWidth;
	}

	UINT GetScreenHeight() const
	{
		return m_viewportHeight;
	}

	bool UpdateWindowSize(UINT backBufferWidth, UINT backBufferHeight);

	void* CreateBasicMeshObject();
	bool BeginCreateMesh(void* pMeshObjectHandle, const void* pVertexList, UINT vertexCount, UINT vertexSize, UINT triGroupCount);
	bool InsertTriGroup(void* pMeshObjectHandle, const WORD* pIndexList, UINT triCount, const WCHAR* wchTexFileName);
	void EndCreateMesh(void* pMeshObjectHandle);
	void RenderMeshObject(void* pMeshObjectHandle, const XMMATRIX& worldMatrix);
	void DeleteBasicMeshObject(void* pMeshObjectHandle);

	void* CreateSpriteObject();
	void* CreateSpriteObject(const WCHAR* wchTexFileName, int posX, int posY, int width, int height);
	void DeleteSpriteObject(void* pSpriteObjectHandle);
	void RenderSpriteWithTex(void* pSpriteObjectHandle, int posX, int posY, int width, int height, const RECT* pRect, float z, void* pTexHandle);
	void RenderSprite(void* pSpriteObjectHandle, int posX, int posY, int width, int height, float z);
	void UpdateTextureWithImage(void* pTexHandle, const BYTE* pSrcBits, UINT SrcWidth, UINT SrcHeight);

	void* CreateTiledTexture(UINT texWidth, UINT texHeight, BYTE r, BYTE g, BYTE b);
	void* CreateDynamicTexture(UINT texWidth, UINT texHeight);
	void* CreateTextureFromFile(const WCHAR* filePath);
	void DeleteTexture(void* pTextureHandle);

	void GetViewProjMatrix(XMMATRIX* pOutViewMatrix, XMMATRIX* pOutProjMatrix);
private:
	bool Initialize(HWND hWindow, bool bEnableDebugLayer = true, bool bEnableGbv = true);

	UINT64 SetFence();
	void WaitForFenceValue(uint64_t expectedFenceValue) const;

	void	CreateFence();
	bool	CreateCommandAllocatorAndCommandList();

	void	CreateRTVAndDSVDescriptorHeap();
	bool	CreateDepthStencil(UINT width, UINT height);

	void	InitializeCamera();

	void	Cleanup();
	void	CleanupFence();
	void	CleanupCommandAllocatorAndCommandList();
	void	CleanupDescriptorHeap();

private:

	static constexpr uint32_t SWAP_CHAIN_FRAME_COUNT = 3;
	static constexpr uint32_t MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;
	static constexpr uint32_t MAX_DRAW_COUNT_PER_FRAME = 4096;
	static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 4096;

	HWND m_windowHandle = nullptr;

	ID3D12Device5* m_pD3DDevice = nullptr;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;

	IDXGISwapChain3* m_pSwapChain = nullptr;
	UINT m_swapChainFlags = 0;

	ID3D12Resource* m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	ID3D12Resource* m_pDepthStencilBuffer = nullptr;

	ID3D12DescriptorHeap* m_pRtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_pDsvDescriptorHeap = nullptr;
	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;

	UINT m_currentRenderTargetIndex = 0;

	FrameContext m_frameContexts[MAX_PENDING_FRAME_COUNT] = {};
	DWORD m_currentContextIndex = 0;

	uint64_t m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;

	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	UINT m_viewportWidth = 0;
	UINT m_viewportHeight = 0;

	std::unique_ptr<CD3D12ResourceManager> m_resourceManager = nullptr;
	std::unique_ptr<CCpuDescriptorFreeListAllocator> m_descriptorAllocator = nullptr;
	std::unique_ptr<CTextureManager> m_textureManager = nullptr;

	XMMATRIX m_viewMatrix = {};
	XMMATRIX m_projectionMatrix = {};
};
