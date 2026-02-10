#pragma once

/**
 * D3DDevice, command allocator , commandlist , swap chain 등등 초기화시에 잡고
 * 그 이후에는 null check 생략
 */

class CD3D12ResourceManager;
class CShaderVisibleDescriptorTableAllocator;
class CConstantBufferPool;
class CDescriptorAllocator;

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

	inline ID3D12Device5* GetD3DDevice() const
	{
		return m_pD3DDevice;
	}
	
	inline CD3D12ResourceManager* GetResourceManager() const
	{
		return m_pResourceManager;
	}

	inline CShaderVisibleDescriptorTableAllocator* GetDescriptorPool() const
	{
		return m_pDescriptorPool.get();
	}

	inline CConstantBufferPool* GetConstantBufferPool() const
	{
		return m_pConstantBufferPool.get();
	}

	bool UpdateWindowSize(UINT backBufferWidth, UINT backBufferHeight);

	void* CreateBasicMeshObject();
	void RenderMeshObject(void* pMeshObjectHandle, const XMMATRIX& worldMatrix, void* pTextureHandle = nullptr);
	void DeleteBasicMeshObject(void* pMeshObjectHandle);

	void* CreateTiledTexture(UINT texWidth, UINT texHeight, BYTE r, BYTE g, BYTE b);
	void DeleteTexture(void* pTextureHandle);

	void GetViewProjMatrix(XMMATRIX* pOutViewMatrix, XMMATRIX* pOutProjMatrix);
private:/*function*/
	bool Initialize(HWND hWindow, bool bEnableDebugLayer = true, bool bEnableGbv = true);

	UINT64 SetFence();
	void WaitForFenceValue() const;

	void	CreateFence();
	bool	CreateCommandAllocatorAndCommandList();

	/**
	 * Render target view 와 depth stencil view 용도의 descriptor heap 을 생성
	 */
	void	CreateRTVAndDSVDescriptorHeap();
	bool	CreateDepthStencil(UINT width, UINT height);

	void	InitializeCamera();

	void	Cleanup();
	void	CleanupFence();
	void	CleanupCommandAllocatorAndCommandList();
	void	CleanupDescriptorHeap();

private: /*variable*/

	static constexpr uint32_t SWAP_CHAIN_FRAME_COUNT = 2;
	static constexpr uint32_t MAX_DRAW_COUNT_PER_FRAME = 4096;
	static constexpr uint32_t MAX_DESCRIPTOR_COUNT = 4096;

	HWND m_windowHandle = nullptr;

	ID3D12Device5* m_pD3DDevice = nullptr;
	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	ID3D12GraphicsCommandList* m_pCommandList = nullptr;
	ID3D12CommandAllocator* m_pCommandAllocator = nullptr;

	IDXGISwapChain3* m_pSwapChain = nullptr;
	UINT m_swapChainFlags = 0;

	ID3D12Resource* m_pRenderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	ID3D12Resource* m_pDepthStencilBuffer = nullptr;

	ID3D12DescriptorHeap* m_pRtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* m_pDsvDescriptorHeap = nullptr;
	UINT m_rtvDescriptorSize = 0;
	UINT m_dsvDescriptorSize = 0;

	UINT m_currentRenderTargetIndex = 0;

	uint64_t m_fenceValue = 0;
	HANDLE m_fenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;

	D3D12_VIEWPORT m_viewport = {};
	D3D12_RECT m_scissorRect = {};
	UINT m_viewportWidth = 0;
	UINT m_viewportHeight = 0;

	CD3D12ResourceManager* m_pResourceManager = nullptr;
	std::unique_ptr<CShaderVisibleDescriptorTableAllocator> m_pDescriptorPool = nullptr;
	std::unique_ptr<CConstantBufferPool> m_pConstantBufferPool = nullptr;
	CDescriptorAllocator* m_pDescriptorAllocator = nullptr;

	XMMATRIX m_viewMatrix = {};
	XMMATRIX m_projectionMatrix = {};
};
