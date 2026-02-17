#pragma once
#include "../Util/IndexCreator.h"

/**
 * CPU-only CBV/SRV/UAV descriptor heap의 free-list 할당기
 *
 * CIndexCreator 기반 인덱스 재사용으로 개별 descriptor를 Allocate/Free하며,
 * 텍스처 등 리소스 수명과 동일하게 유지되는 영구 descriptor를 관리합니다.
 */
class CCpuDescriptorFreeListAllocator
{
public:
	CCpuDescriptorFreeListAllocator() = default;
	~CCpuDescriptorFreeListAllocator() = default;

	bool Initialize(ID3D12Device* pD3DDevice, const UINT maxDescriptorCount);

	bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE inCpuDescriptorHandle);

private:
	ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap = nullptr;
	CIndexCreator m_indexCreator;

	UINT m_descriptorSize = 0;
};

