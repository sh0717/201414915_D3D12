#pragma once

struct ConstantBufferContainer
{
	D3D12_CPU_DESCRIPTOR_HANDLE CbvDescriptorHandle;
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress;
	UINT8* SystemAddress;
};

/**
 * 프레임 단위 constant buffer 선형 할당기
 *
 * 단일 upload heap 위에 사전 생성된 CBV 배열을 보유하며,
 * draw call마다 Allocate()로 다음 슬롯을 반환하고,
 * 매 프레임 Reset()으로 오프셋을 0으로 되돌려 전체를 재사용합니다.
 */
class CConstantBufferPool
{
public:
	CConstantBufferPool() = default;
	~CConstantBufferPool() = default;

	bool Initialize(ID3D12Device5* pD3DDevice, UINT sizePerConstantBuffer, UINT maxCbvCount);

	ID3D12DescriptorHeap* GetDescriptorHeap() const
	{
		return m_cbvDescriptorHeap.Get();
	}

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