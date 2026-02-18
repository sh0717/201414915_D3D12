#pragma once

#include "ConstantBufferPool.h"
#include "../../Types/typedef.h"

class CConstantBufferManager
{
public:
	CConstantBufferManager() = default;
	~CConstantBufferManager() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT maxCbvCountPerType);
	void Reset();

	CConstantBufferPool* GetConstantBufferPool(ConstantBufferType type) const;

private:
	std::unique_ptr<CConstantBufferPool> m_constantBufferPoolList[ConstantBufferTypeCount] = {};
};
