#pragma once

#include <d3d12.h>
#include <vector>

#include "Types/typedef.h"

class CD3D12Renderer;
class CCommandListPool;
class CBasicMeshObject;
class CSpriteObject;

enum class ERenderItemType
{
	MeshObject = 0,
	Sprite
};

struct RenderMeshItem
{
	CBasicMeshObject* pMeshObject = nullptr;
	XMMATRIX WorldMatrix = {};
};

struct RenderSpriteItem
{
	CSpriteObject* pSpriteObject = nullptr;
	XMFLOAT2 Pos = {};
	XMFLOAT2 PixelSize = {};
	RECT SampleRect = {};
	bool bUseRect = false;
	float Z = 0.0f;
	TextureHandle* pTexHandle = nullptr;
};

struct RenderItem
{
	ERenderItemType Type = ERenderItemType::MeshObject;
	union
	{
		RenderMeshItem MeshItem;
		RenderSpriteItem SpriteItem;
	};

	RenderItem()
		: MeshItem()
	{
	}
};

class CRenderQueue
{
public:
	bool Initialize(CD3D12Renderer* pRenderer, UINT maxItemCount);
	bool Add(const RenderItem& pRenderItem);

	UINT Process(
		CCommandListPool* pCommandListPool,
		ID3D12CommandQueue* pCommandQueue,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle,
		UINT processCountPerCommandList = 0);
	void Reset();

	UINT GetItemCount() const
	{
		return static_cast<UINT>(m_itemList.size());
	}

private:
	bool ProcessRenderItem(ID3D12GraphicsCommandList* pCommandList, ID3D12Device5* pD3DDevice, const RenderItem& renderItem);
	void SetupCommandListForDraw(
		ID3D12GraphicsCommandList* pCommandList,
		const D3D12_VIEWPORT& viewport,
		const D3D12_RECT& scissorRect,
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle,
		D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle) const;

private:
	CD3D12Renderer* m_pRenderer = nullptr;
	std::vector<RenderItem> m_itemList = {};
	UINT m_maxItemCount = 0;
};
