#pragma once

#include <array>
#include <memory>
#include <vector>

/**
 * D3DDevice, command allocator , commandlist , swap chain 등등 초기화시에 잡고
 * 그 이후에는 null check 생략
 */

class CRenderQueue;
class CD3D12ResourceManager;
class CPersistentCpuDescriptorAllocator;
class CConstantBufferManager;
class CTextureManager;

#include "RenderHelper/FrameGpuDescriptorAllocator.h"
#include "RenderHelper/CommandListPool.h"
#include "RenderHelper/ConstantBufferManager.h"
#include "RenderHelper/RenderThread.h"

struct RenderThreadContext
{
	RenderThreadContext() = default;
	~RenderThreadContext();
	RenderThreadContext(RenderThreadContext&&) noexcept = default;
	RenderThreadContext& operator=(RenderThreadContext&&) noexcept = default;
	RenderThreadContext(const RenderThreadContext&) = delete;
	RenderThreadContext& operator=(const RenderThreadContext&) = delete;

	std::unique_ptr<CRenderQueue> RenderQueue = nullptr;
	std::unique_ptr<CFrameGpuDescriptorAllocator> GpuDescriptorAllocator = nullptr;
	std::unique_ptr<CConstantBufferManager> ConstantBufferManager = nullptr;
	std::unique_ptr<CCommandListPool> CommandListPool = nullptr;
	std::vector<ID3D12CommandList*> RecordedCommandListArray = {};
};

struct FrameContext
{
	std::unique_ptr<CCommandListPool> CommandListPool = nullptr;
	std::vector<RenderThreadContext> RenderThreadContextList = {};
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

	CFrameGpuDescriptorAllocator* GetFrameGpuDescriptorAllocator(DWORD renderThreadIndex) const
	{
		const FrameContext& ctx = m_frameContexts[m_currentContextIndex];
		if (renderThreadIndex >= ctx.RenderThreadContextList.size())
		{
			return nullptr;
		}

		return ctx.RenderThreadContextList[renderThreadIndex].GpuDescriptorAllocator.get();
	}

	CPersistentCpuDescriptorAllocator* GetPersistentCpuDescriptorAllocator() const
	{
		return m_persistentCpuDescriptorAllocator.get();
	}

	CConstantBufferPool* GetConstantBufferPool(EConstantBufferType type, DWORD renderThreadIndex) const
	{
		const FrameContext& ctx = m_frameContexts[m_currentContextIndex];
		if (renderThreadIndex >= ctx.RenderThreadContextList.size())
		{
			return nullptr;
		}

		if (!ctx.RenderThreadContextList[renderThreadIndex].ConstantBufferManager)
		{
			return nullptr;
		}

		return ctx.RenderThreadContextList[renderThreadIndex].ConstantBufferManager->GetConstantBufferPool(type);
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

	void SetCameraPos(float x, float y, float z);
	void MoveCamera(float dx, float dy, float dz);
	void ProcessByThread(DWORD renderThreadIndex);

private:
	bool Initialize(HWND hWindow, bool bEnableDebugLayer = true, bool bEnableGbv = true);

	UINT64 SetFence();
	void WaitForFenceValue(uint64_t expectedFenceValue) const;

	void	CreateFence();
	bool	InitializeFrameContexts();
	bool	InitializeCommandListPool(FrameContext& ctx);
	bool	InitializeRenderThreadContext(RenderThreadContext& renderThreadContext);
	bool	InitializeRenderThreadPool();

	bool	InitializeFramebufferResources(UINT width, UINT height);
	bool	CreateFramebufferDescriptorHeaps();
	bool	CreateRenderTargetViews();
	bool	CreateDepthStencilBufferAndView(UINT width, UINT height);

	void	InitializeCamera();

	CRenderQueue* GetCurrentRenderQueue() const;
	void	AdvanceRenderThreadIndex();

	void	Cleanup();
	void	CleanupFence();
	void	CleanupFrameContexts();
	void	CleanupCommandListPool(FrameContext& ctx);
	void	CleanupRenderThreadContext(RenderThreadContext& renderThreadContext);
	void	CleanupRenderThreadPool();
	void	CleanupFramebufferResources();
	void	CleanupFramebufferDescriptorHeaps();

private:

	static constexpr uint32_t SwapChainFrameCount = 3;
	static constexpr uint32_t MaxPendingFrameCount = SwapChainFrameCount - 1;
	static constexpr uint32_t MaxCommandListCountPerFrame = 256;
	static constexpr uint32_t MaxDrawCountPerFrame = 4096;
	static constexpr uint32_t MaxRenderThreadCount = 8;
	static constexpr uint32_t MaxDescriptorCount = 4096;

	HWND m_windowHandle = nullptr;

	ID3D12Device5* m_pD3DDevice = nullptr;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;

	IDXGISwapChain3* m_pSwapChain = nullptr;
	UINT m_swapChainFlags = 0;

	std::array<ID3D12Resource*, SwapChainFrameCount> m_pRenderTargets = {};
	ID3D12Resource* m_pDepthStencilBuffer = nullptr;

	ID3D12DescriptorHeap* m_pRtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_pDsvDescriptorHeap = nullptr;
	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;

	UINT m_currentRenderTargetIndex = 0;

	std::array<FrameContext, MaxPendingFrameCount> m_frameContexts = {};
	DWORD m_currentContextIndex = 0;

	uint64_t m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;

	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	UINT m_viewportWidth = 0;
	UINT m_viewportHeight = 0;

	std::unique_ptr<CD3D12ResourceManager> m_resourceManager = nullptr;
	std::unique_ptr<CPersistentCpuDescriptorAllocator> m_persistentCpuDescriptorAllocator = nullptr;
	std::unique_ptr<CTextureManager> m_textureManager = nullptr;
	std::vector<RenderThreadDesc> m_renderThreadDescList = {};
	DWORD m_renderThreadCount = 1;
	DWORD m_currentRenderThreadIndex = 0;

	XMVECTOR m_cameraPos = {};
	XMVECTOR m_cameraDir = {};
	XMMATRIX m_viewMatrix = {};
	XMMATRIX m_projectionMatrix = {};
};



