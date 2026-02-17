#include "pch.h"
#include "ConstantBufferPool.h"

bool CConstantBufferPool::Initialize(ID3D12Device5* pD3DDevice, UINT sizePerConstantBuffer, UINT maxCbvCount)
{
	if (!pD3DDevice)
	{
		__debugbreak();
		return false;
	}

	m_maxCbvCount = maxCbvCount;
	m_sizePerConstantBuffer = sizePerConstantBuffer;
	const UINT byteWidth = m_maxCbvCount * m_sizePerConstantBuffer;

	HRESULT hr = pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_constantBufferChunk.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_maxCbvCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_cbvDescriptorHeap.ReleaseAndGetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	CD3DX12_RANGE writeRange(0, 0);
	m_constantBufferChunk->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSystemAddressForStart));

	m_constantBufferContainerList = std::make_unique<ConstantBufferContainer[]>(m_maxCbvCount);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_constantBufferChunk->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_sizePerConstantBuffer;

	UINT8* pSystemMemPtr = m_pSystemAddressForStart;
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_cbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	const UINT descriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (DWORD i = 0; i < m_maxCbvCount; i++)
	{
		pD3DDevice->CreateConstantBufferView(&cbvDesc, descriptorHandle);

		m_constantBufferContainerList[i].CbvDescriptorHandle = descriptorHandle;
		m_constantBufferContainerList[i].GpuAddress = cbvDesc.BufferLocation;
		m_constantBufferContainerList[i].SystemAddress = pSystemMemPtr;

		descriptorHandle.Offset(1, descriptorSize);
		cbvDesc.BufferLocation += m_sizePerConstantBuffer;
		pSystemMemPtr += m_sizePerConstantBuffer;
	}

	return true;
}

ConstantBufferContainer* CConstantBufferPool::Allocate()
{
	if (m_allocatedCbvCount >= m_maxCbvCount)
	{
		return nullptr;
	}

	return &m_constantBufferContainerList[m_allocatedCbvCount++];
}

void CConstantBufferPool::Reset()
{
	m_allocatedCbvCount = 0;
}
