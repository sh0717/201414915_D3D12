#include "pch.h"

#include "RenderQueue.h"
#include "CommandListPool.h"
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

UINT CRenderQueue::Process(
	DWORD renderThreadIndex,
	CCommandListPool* pCommandListPool,
	std::vector<ID3D12CommandList*>& recordedCommandListArray,
	const D3D12_VIEWPORT& viewport,
	const D3D12_RECT& scissorRect,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle,
	D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle,
	UINT processCountPerCommandList)
{
	if (!m_pRenderer || !pCommandListPool)
	{
		return 0;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (!pD3DDevice)
	{
		return 0;
	}

	const UINT maxProcessCountPerCommandList =
		(processCountPerCommandList == 0) ? static_cast<UINT>(m_itemList.size()) : processCountPerCommandList;

	UINT commandListCapacity = 1;
	if (processCountPerCommandList > 0)
	{
		commandListCapacity = (static_cast<UINT>(m_itemList.size()) + processCountPerCommandList - 1) / processCountPerCommandList;
	}

	recordedCommandListArray.clear();
	recordedCommandListArray.reserve(commandListCapacity);

	ID3D12GraphicsCommandList* pCommandList = nullptr;
	UINT processedItemCount = 0;
	UINT processedItemCountPerCommandList = 0;

	for (const RenderItem& renderItem : m_itemList)
	{
		if (!pCommandList)
		{
			pCommandList = pCommandListPool->GetCurrentCommandList();
			if (!pCommandList)
			{
				__debugbreak();
				return processedItemCount;
			}

			SetupCommandListForDraw(pCommandList, viewport, scissorRect, rtvDescriptorHandle, dsvDescriptorHandle);
		}

		if (!ProcessRenderItem(pCommandList, pD3DDevice, renderThreadIndex, renderItem))
		{
			continue;
		}

		processedItemCount++;
		processedItemCountPerCommandList++;

		if (processedItemCountPerCommandList >= maxProcessCountPerCommandList)
		{
			pCommandListPool->Close();
			recordedCommandListArray.push_back(pCommandList);
			pCommandList = nullptr;
			processedItemCountPerCommandList = 0;
		}
	}

	if (pCommandList)
	{
		pCommandListPool->Close();
		if (processedItemCountPerCommandList > 0)
		{
			recordedCommandListArray.push_back(pCommandList);
		}
	}

	return processedItemCount;
}

bool CRenderQueue::ProcessRenderItem(ID3D12GraphicsCommandList* pCommandList, ID3D12Device5* pD3DDevice, DWORD renderThreadIndex, const RenderItem& renderItem)
{
	if (!pCommandList || !pD3DDevice)
	{
		return false;
	}

	switch (renderItem.Type)
	{
	case ERenderItemType::MeshObject:
		if (!renderItem.MeshItem.pMeshObject)
		{
			return false;
		}

		renderItem.MeshItem.pMeshObject->Draw(pCommandList, renderThreadIndex, renderItem.MeshItem.WorldMatrix);
		return true;

	case ERenderItemType::Sprite:
	{
		CSpriteObject* pSpriteObject = renderItem.SpriteItem.pSpriteObject;
		if (!pSpriteObject)
		{
			return false;
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
				renderThreadIndex,
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
				renderThreadIndex,
				renderItem.SpriteItem.Pos,
				renderItem.SpriteItem.PixelSize,
				renderItem.SpriteItem.Z);
		}

		return true;
	}

	default:
		__debugbreak();
		return false;
	}
}

void CRenderQueue::SetupCommandListForDraw(
	ID3D12GraphicsCommandList* pCommandList,
	const D3D12_VIEWPORT& viewport,
	const D3D12_RECT& scissorRect,
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptorHandle,
	D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle) const
{
	if (!pCommandList)
	{
		return;
	}

	pCommandList->RSSetViewports(1, &viewport);
	pCommandList->RSSetScissorRects(1, &scissorRect);
	pCommandList->OMSetRenderTargets(1, &rtvDescriptorHandle, FALSE, &dsvDescriptorHandle);
}

void CRenderQueue::Reset()
{
	m_itemList.clear();
}
