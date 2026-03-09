#pragma once

struct ConstantBufferContainer
{
	D3D12_CPU_DESCRIPTOR_HANDLE CbvDescriptorHandle = {};
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = {};
	UINT8* SystemAddress = {};
};

/**
 * Frame-local linear allocator for constant buffers.
 *
 * Owns a prebuilt CBV array on top of a single upload heap,
 * returns the next slot on each Allocate() call,
 * and reuses the entire allocation range by resetting the offset to zero every frame.
 */
class CConstantBufferPool
{
public:
	CConstantBufferPool() = default;
	~CConstantBufferPool() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT sizePerConstantBuffer, UINT maxCbvCount);

	ConstantBufferContainer* Allocate();
	void Reset();

private:
	std::unique_ptr<ConstantBufferContainer[]> m_constantBufferContainerList = nullptr;

	UINT m_allocatedCbvCount = 0;
	UINT m_maxCbvCount = 0;
	UINT m_sizePerConstantBuffer = 0;

	ComPtr<ID3D12Resource> m_constantBufferChunk = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_cbvDescriptorHeap = nullptr;
	UINT8* m_pSystemAddressForStart = nullptr;
};
