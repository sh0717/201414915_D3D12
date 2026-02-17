#pragma once

struct IndexedTriGroup;
class CD3D12Renderer;

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

	bool BeginCreateMesh(const void* pVertexList, UINT vertexCount, UINT vertexSize, UINT triGroupCount);
	bool InsertIndexedTriList(const WORD* pIndexList, UINT triCount, const WCHAR* texFileName);
	void EndCreateMesh();

	void Draw(ID3D12GraphicsCommandList* pCommandList, const XMMATRIX& worldMatrix);

	void Clean();
	void CleanSharedResource();
	void CleanTriGroups();

private: /*variable*/
	static constexpr UINT DescriptorCountPerObj = 1;
	static constexpr UINT DescriptorCountPerTriGroup = 1;
	static constexpr UINT MaxTriGroupCountPerObj = 8;
	static constexpr UINT MaxDescriptorCountForDraw = DescriptorCountPerObj + (MaxTriGroupCountPerObj * DescriptorCountPerTriGroup);

	static ID3D12RootSignature* m_pRootSignature;
	static ID3D12PipelineState* m_pPipelineStateObject;
	static UINT m_initRefCount;

	CD3D12Renderer* m_pRenderer = nullptr;

	ComPtr<ID3D12Resource> m_vertexBuffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView = {};

	std::unique_ptr<IndexedTriGroup[]> m_triGroupList;
	UINT m_maxTriGroupCount = 0;
	UINT m_triGroupCount = 0;
};
