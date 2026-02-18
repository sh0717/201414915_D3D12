#include "pch.h"
#include "ConstantBufferManager.h"
#include "../../../Util/D3DUtil.h"

bool CConstantBufferManager::Initialize(ID3D12Device5* pD3DDevice, UINT maxCbvCountPerType)
{
	if (!pD3DDevice)
	{
		__debugbreak();
		return false;
	}

	static constexpr ConstantBufferProperty ConstantBufferPropertyList[] =
	{
		{ EConstantBufferType::Default, static_cast<UINT>(sizeof(ConstantBufferDefault)) },
		{ EConstantBufferType::Sprite, static_cast<UINT>(sizeof(ConstantBufferSprite)) }
	};

	for (UINT i = 0; i < _countof(ConstantBufferPropertyList); i++)
	{
		const ConstantBufferProperty& property = ConstantBufferPropertyList[i];
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
	for (UINT i = 0; i < static_cast<UINT>(EConstantBufferType::Count); i++)
	{
		if (m_constantBufferPoolList[i])
		{
			m_constantBufferPoolList[i]->Reset();
		}
	}
}

CConstantBufferPool* CConstantBufferManager::GetConstantBufferPool(EConstantBufferType type) const
{
	const UINT poolIndex = static_cast<UINT>(type);
	if (poolIndex >= static_cast<UINT>(EConstantBufferType::Count))
	{
		__debugbreak();
		return nullptr;
	}

	return m_constantBufferPoolList[poolIndex].get();
}
