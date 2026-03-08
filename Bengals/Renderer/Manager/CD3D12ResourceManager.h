#pragma once

/**
 * Dedicated manager for GPU resource uploads.
 *
 * Owns a dedicated command queue, command list, and command allocator to upload
 * vertex/index buffers and textures in parallel with the main render queue.
 * The D3D device is borrowed from CD3D12Renderer and is not owned by this class.
 */

class CD3D12ResourceManager
{
public:
	CD3D12ResourceManager() = default;
	~CD3D12ResourceManager();

	bool Initialize(ID3D12Device5* pD3DDevice);

	HRESULT CreateVertexBuffer(const UINT sizePerVertex, const UINT vertexCount, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, const void* pInInitData);
	HRESULT CreateIndexBuffer(const UINT indexCount, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, const void* pInitData);

	void UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);
	bool CreateTexture(ID3D12Resource** ppOutResource, UINT width, UINT height, DXGI_FORMAT format, const BYTE* pInitImage);
	bool CreateTextureFromFile(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* inFileName);
	bool CreateTexturePair(ID3D12Resource** ppOutResource, ID3D12Resource** ppOutUploadBuffer, UINT Width, UINT Height, DXGI_FORMAT format);
private:
	UINT64 Fence();
	void WaitForFenceValue() const;

private:
	ID3D12Device5* m_pD3DDevice = nullptr; /*Don't have ownership*/

	ComPtr<ID3D12CommandQueue> m_commandQueue = nullptr;
	ComPtr<ID3D12GraphicsCommandList> m_commandList = nullptr;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator = nullptr;

	HANDLE m_fenceEvent = nullptr;
	ComPtr<ID3D12Fence> m_fence = nullptr;
	UINT64 m_fenceValue = 0;
};


