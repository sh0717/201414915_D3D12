#pragma once

class CD3D12Renderer;
struct TextureHandle;

class CSpriteObject
{
	friend class CD3D12Renderer;

public: /*function*/
	CSpriteObject() = delete;
	explicit CSpriteObject(CD3D12Renderer* pRenderer);
	CSpriteObject(CD3D12Renderer* pRenderer, const WCHAR* wchTexFileName, const RECT* pRect);
	~CSpriteObject();

	bool Initialize(CD3D12Renderer* pRenderer);
	bool Initialize(CD3D12Renderer* pRenderer, const WCHAR* wchTexFileName, const RECT* pRect);

	void DrawWithTex(
		ID3D12GraphicsCommandList* pCommandList,
		XMFLOAT2 pos,
		XMFLOAT2 pixelSize,
		const RECT* pRect,
		float z,
		TextureHandle* pTexHandle);

	void Draw(
		ID3D12GraphicsCommandList* pCommandList,
		XMFLOAT2 pos,
		XMFLOAT2 pixelSize,
		float z);

private: /*function*/
	bool InitRootSignature();
	bool InitPipelineState();
	bool InitSharedBuffers();

	void Clean();
	void CleanTexture();
	void CleanSharedResource();

private: /*variable*/
	enum ESpriteDescriptorIndex
	{
		SpriteDescriptorIndexCbv = 0,
		SpriteDescriptorIndexTex = 1
	};

	static constexpr UINT DescriptorCountForDraw = 2;

	static ID3D12RootSignature* m_pRootSignature;
	static ID3D12PipelineState* m_pPipelineStateObject;
	static ID3D12Resource* m_pVertexBuffer;
	static D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	static ID3D12Resource* m_pIndexBuffer;
	static D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	static UINT m_initRefCount;

	CD3D12Renderer* m_pRenderer = nullptr;
	TextureHandle* m_pTexHandle = nullptr;
	RECT m_rect = {};
	XMFLOAT2 m_scale = { 1.0f, 1.0f };
};
