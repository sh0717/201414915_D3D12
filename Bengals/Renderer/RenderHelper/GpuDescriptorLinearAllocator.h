#pragma once

/**
 * GPU-visible CBV/SRV/UAV descriptor heap의 프레임 단위 선형 할당기
 *
 * draw call마다 연속된 descriptor table을 bump 방식으로 할당하고,
 * 매 프레임 Reset()으로 오프셋을 0으로 되돌려 heap 전체를 재사용합니다.
 */
class CGpuDescriptorLinearAllocator
{
public:
	CGpuDescriptorLinearAllocator() = default;
	~CGpuDescriptorLinearAllocator() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT maxDescriptorCount);

	ID3D12DescriptorHeap* GetDescriptorHeap() const
	{
		return m_descriptorHeap.Get();
	};

	bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuDescriptorHandle,const UINT descriptorCount);
	void Reset();

	UINT GetDescriptorSizeCbvSrvUav() const
	{
		return m_descriptorSizeCbvSrvUav;
	}

private:

	ComPtr<ID3D12DescriptorHeap> m_descriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandleForHeapStart = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandleForHeapStart = {};

	UINT m_descriptorSizeCbvSrvUav = 0;
	UINT m_allocatedDescriptorCount = 0;
	UINT m_maxDescriptorCount = 0;
};