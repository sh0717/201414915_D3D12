#include "pch.h"

#include "RenderQueue.h"
#include "../../../Util/D3DUtil.h"
#include "../D3D12Renderer.h"
#include "../RenderObject/BasicMeshObject.h"
#include "../RenderObject/SpriteObject.h"

bool CRenderQueue::Initialize(CD3D12Renderer* pRenderer, UINT maxItemCount)
{
	if (!pRenderer || maxItemCount == 0)
	{
		return false;
	}

	m_pRenderer = pRenderer;
	m_maxItemCount = maxItemCount;
	m_itemList.clear();
	m_itemList.reserve(maxItemCount);
	return true;
}

bool CRenderQueue::Add(const RenderItem& pRenderItem)
{
	if (m_itemList.size() >= m_maxItemCount)
	{
		return false;
	}

	m_itemList.push_back(pRenderItem);
	return true;
}

UINT CRenderQueue::Process(ID3D12GraphicsCommandList* pCommandList)
{
	if (!m_pRenderer || !pCommandList)
	{
		return 0;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (!pD3DDevice)
	{
		return 0;
	}

	UINT processedItemCount = 0;

	for (RenderItem& renderItem : m_itemList)
	{
		switch (renderItem.Type)
		{
		case ERenderItemType::MeshObject:
			if (!renderItem.MeshItem.pMeshObject)
			{
				continue;
			}

			renderItem.MeshItem.pMeshObject->Draw(pCommandList, renderItem.MeshItem.WorldMatrix);
			processedItemCount++;
			break;

		case ERenderItemType::Sprite:
		{
			CSpriteObject* pSpriteObject = renderItem.SpriteItem.pSpriteObject;
			if (!pSpriteObject)
			{
				continue;
			}

			TextureHandle* pTextureHandle = renderItem.SpriteItem.pTexHandle;
			if (pTextureHandle && pTextureHandle->pUploadBuffer && pTextureHandle->bUpdated)
			{
				UpdateTexture(pD3DDevice, pCommandList, pTextureHandle->TextureResource, pTextureHandle->pUploadBuffer);
				pTextureHandle->bUpdated = false;
			}

			if (pTextureHandle)
			{
				const RECT* pRect = renderItem.SpriteItem.bUseRect ? &renderItem.SpriteItem.SampleRect : nullptr;
				pSpriteObject->DrawWithTex(
					pCommandList,
					renderItem.SpriteItem.Pos,
					renderItem.SpriteItem.PixelSize,
					pRect,
					renderItem.SpriteItem.Z,
					pTextureHandle);
			}
			else
			{
				pSpriteObject->Draw(
					pCommandList,
					renderItem.SpriteItem.Pos,
					renderItem.SpriteItem.PixelSize,
					renderItem.SpriteItem.Z);
			}

			processedItemCount++;
			break;
		}

		default:
			__debugbreak();
			break;
		}
	}

	return processedItemCount;
}

void CRenderQueue::Reset()
{
	m_itemList.clear();
}
