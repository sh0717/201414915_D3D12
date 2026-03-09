#pragma once

#include <array>

#include "ConstantBufferPool.h"
#include "Types/typedef.h"

class CConstantBufferManager
{
public:
	CConstantBufferManager() = default;
	~CConstantBufferManager() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT maxCbvCountPerType);
	void Reset();

	CConstantBufferPool* GetConstantBufferPool(const EConstantBufferType type) const;

private:
	std::array<std::unique_ptr<CConstantBufferPool>, static_cast<UINT>(EConstantBufferType::Count)> m_constantBufferPoolList = {};
};


