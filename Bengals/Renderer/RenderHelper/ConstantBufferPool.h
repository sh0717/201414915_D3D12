#pragma once

struct ConstantBufferContainer
{
	D3D12_CPU_DESCRIPTOR_HANDLE CbvDescriptorHandle;
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
	UINT8* SystemAddress;
};

/**
 * 매 프레임마다 Reset 을 호출해 처음부터 할당하도록 합니다.
 */
class CConstantBufferPool
{
public:
	CConstantBufferPool() = default;
	~CConstantBufferPool();

	bool Initialize(ID3D12Device5* pD3DDevice, UINT sizePerConstantBuffer, UINT maxCbvCount);

	inline ID3D12DescriptorHeap* GetDescriptorHeap() const
	{
		return m_pCbvDescriptorHeap;
	}

	ConstantBufferContainer* Allocate();
	void Reset();

private:
	void Cleanup();

private:
	ConstantBufferContainer* m_pConstantBufferContainerList = nullptr;

	UINT m_allocatedCbvCount = 0;
	UINT m_maxCbvCount = 0;
	UINT m_sizePerConstantBuffer = 0;

	ID3D12Resource* m_pConstantBufferChunk = nullptr;
	ID3D12DescriptorHeap* m_pCbvDescriptorHeap = nullptr;
	UINT8* m_pSystemAddressForStart = nullptr;
};