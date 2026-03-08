#pragma once

/**
 * Per-frame linear allocator for a GPU-visible CBV/SRV/UAV descriptor heap.
 *
 * It allocates contiguous descriptor table ranges with a bump-pointer scheme
 * and reuses the entire heap by resetting the allocation offset every frame.
 */
class CFrameGpuDescriptorAllocator
{
public:
	CFrameGpuDescriptorAllocator() = default;
	~CFrameGpuDescriptorAllocator() = default;

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
