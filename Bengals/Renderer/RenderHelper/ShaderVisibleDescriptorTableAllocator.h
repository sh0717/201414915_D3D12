#pragma once

/**
 * Shader 에서 visible 인 SRV, CBV, UAV 전용 Descriptor pool
 *
 * 셰이더에 전달할 Descriptor 들을 담을 Table을 할당해줍니다.
 * 프레임마다 Reset 하여 index 0 부터 다시 할당합니다.
 */
class CShaderVisibleDescriptorTableAllocator
{
public:
	CShaderVisibleDescriptorTableAllocator() = default;
	~CShaderVisibleDescriptorTableAllocator() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT maxDescriptorCount);

	inline ID3D12DescriptorHeap* GetDescriptorHeap() const
	{
		return m_descriptorHeap.Get();
	};

	bool Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuDescriptorHandle, UINT descriptorCount);
	void Reset();

	inline UINT GetDescriptorSizeCbvSrvUav() const
	{
		return m_descriptorSizeCbvSrvUav;
	}

private:
	ID3D12Device5* m_pD3DDevice = nullptr;

	ComPtr<ID3D12DescriptorHeap> m_descriptorHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuDescriptorHandleForHeapStart = {};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptorHandleForHeapStart = {};

	UINT m_descriptorSizeCbvSrvUav = 0;
	INT m_allocatedDescriptorCount = 0;
	UINT m_maxDescriptorCount = 0;
};