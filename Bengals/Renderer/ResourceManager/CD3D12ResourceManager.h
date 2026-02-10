#pragma once

/*
 * this class make vertex buffer(in GPU) and shader resource  with initializing data.
 *
 *	it has its own command list and command queue  
 *	D3DDevice is from D3D12Renderer 
 */

class CD3D12ResourceManager
{
public:
	CD3D12ResourceManager() = default;
	~CD3D12ResourceManager();

	bool Initialize(ID3D12Device5* pD3DDevice);

	/*Each CreateVertexBuffer and CreateIndexBuffer function call execute command list*/
	HRESULT CreateVertexBuffer(UINT sizePerVertex, UINT vertexCount, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pInInitData);
	HRESULT CreateIndexBuffer(UINT indexCount , D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer , void* pInitData);

	void	UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);
	BOOL	CreateTexture(ID3D12Resource** ppOutResource, UINT width, UINT height, DXGI_FORMAT format, const BYTE* pInitImage);
private:
	UINT64 Fence();
	void WaitForFenceValue() const;

	void Clean();

private:
	ID3D12Device5* m_pD3DDevice = nullptr; /*Don't have ownership*/

	ID3D12CommandQueue* m_pCommandQueue = nullptr;
	ID3D12GraphicsCommandList* m_pCommandList = nullptr;
	ID3D12CommandAllocator* m_pCommandAllocator = nullptr;

	HANDLE m_fenceEvent = nullptr;
	ID3D12Fence* m_pFence = nullptr;
	UINT64 m_fenceValue = 0;
};

