#pragma once

class CBasicMeshObject
{
	friend class CD3D12Renderer;

public:/*function*/
	CBasicMeshObject() = delete;
	CBasicMeshObject(CD3D12Renderer* pRenderer);
	~CBasicMeshObject();

	bool Initialize(CD3D12Renderer* pRenderer);

private: /*function*/
	bool InitRootSignature();
	bool InitPipelineState();

	/**
	 * Create vertex buffer and index buffer
	 */
	bool CreateMesh();

	void Draw(ID3D12GraphicsCommandList* pCommandList, const XMMATRIX& worldMatrix, D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle);

	void Clean();
	void CleanSharedResource();

private: /*variable*/
	enum BasicMeshDescriptorIndex
	{
		BasicMeshDescriptorIndexCbv = 0,
		BasicMeshDescriptorIndexSrv = 1,
		BasicMeshDescriptorCount = 2,
	};

	static constexpr UINT DescriptorCountPerDraw = BasicMeshDescriptorCount;

	static ID3D12RootSignature* m_pRootSignature;
	static ID3D12PipelineState* m_pPipelineStateObject;
	static UINT m_initRefCount;

	CD3D12Renderer* m_pRenderer = nullptr;

	ComPtr<ID3D12Resource> m_vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};

	ComPtr<ID3D12Resource> m_indexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView = {};
};