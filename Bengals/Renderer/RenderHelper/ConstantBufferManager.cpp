#include "pch.h"
#include "ConstantBufferManager.h"
#include "../../../Util/D3DUtil.h"

namespace
{
	constexpr std::array<ConstantBufferProperty, static_cast<UINT>(EConstantBufferType::Count)> ConstantBufferPropertyList =
	{{
		{ EConstantBufferType::Default, static_cast<UINT>(sizeof(ConstantBufferDefault)) },
		{ EConstantBufferType::Sprite, static_cast<UINT>(sizeof(ConstantBufferSprite)) }
	}};

	static_assert(
		ConstantBufferPropertyList.size() == static_cast<UINT>(EConstantBufferType::Count),
		"Constant buffer property list must match EConstantBufferType::Count.");
}

bool CConstantBufferManager::Initialize(ID3D12Device5* pD3DDevice, UINT maxCbvCountPerType)
{
	if (!pD3DDevice || maxCbvCountPerType == 0)
	{
		__debugbreak();
		return false;
	}

	for (const ConstantBufferProperty& property : ConstantBufferPropertyList)
	{
		const UINT poolIndex = static_cast<UINT>(property.Type);

		m_constantBufferPoolList[poolIndex] = std::make_unique<CConstantBufferPool>();
		if (!m_constantBufferPoolList[poolIndex]->Initialize(
			pD3DDevice,
			static_cast<UINT>(AlignConstantBufferSize(property.Size)),
			maxCbvCountPerType))
		{
			__debugbreak();
			return false;
		}
	}

	return true;
}

void CConstantBufferManager::Reset()
{
	for (std::unique_ptr<CConstantBufferPool>& constantBufferPool : m_constantBufferPoolList)
	{
		if (constantBufferPool)
		{
			constantBufferPool->Reset();
		}
	}
}

CConstantBufferPool* CConstantBufferManager::GetConstantBufferPool(const EConstantBufferType type) const
{
	const UINT poolIndex = static_cast<UINT>(type);
	if (poolIndex >= static_cast<UINT>(EConstantBufferType::Count))
	{
		__debugbreak();
		return nullptr;
	}

	return m_constantBufferPoolList[poolIndex].get();
}
