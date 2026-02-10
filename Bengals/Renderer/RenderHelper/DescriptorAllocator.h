#pragma once
#include "../../../Util/IndexCreator.h"

class CDescriptorAllocator
{
public:
	CDescriptorAllocator() = default;
	~CDescriptorAllocator();

	bool Initialize(ID3D12Device* pD3DDevice, UINT maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE inCpuDescriptorHandle);
private:
	void Cleanup();

	

private:
	ID3D12DescriptorHeap* m_pDescriptorHeap = nullptr;
	CIndexCreator m_indexCreator;

	UINT m_descriptorSize = 0;
	
	

};

