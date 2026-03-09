#pragma once

#include <vector>

#include "Types/typedef.h"

class CD3D12Renderer;
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

	UINT Process(ID3D12GraphicsCommandList* pCommandList);
	void Reset();

	UINT GetItemCount() const
	{
		return static_cast<UINT>(m_itemList.size());
	}

private:
	CD3D12Renderer* m_pRenderer = nullptr;
	std::vector<RenderItem> m_itemList = {};
	UINT m_maxItemCount = 0;
};
