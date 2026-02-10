#include "pch.h"
#include "ConstantBufferPool.h"

CConstantBufferPool::~CConstantBufferPool()
{
	Cleanup();
}

bool CConstantBufferPool::Initialize(ID3D12Device5* pD3DDevice, UINT sizePerConstantBuffer, UINT maxCbvCount)
{
	bool bResult = false;
	m_maxCbvCount = maxCbvCount;
	m_sizePerConstantBuffer = sizePerConstantBuffer;
	const UINT byteWidth = m_maxCbvCount * m_sizePerConstantBuffer;

	HRESULT hr = pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteWidth),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pConstantBufferChunk));
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_maxCbvCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_pCbvDescriptorHeap))))
	{
		__debugbreak();
		return bResult;
	}

	CD3DX12_RANGE writeRange(0, 0);
	m_pConstantBufferChunk->Map(0, &writeRange, reinterpret_cast<void**>(&m_pSystemAddressForStart));

	m_pConstantBufferContainerList = new ConstantBufferContainer[m_maxCbvCount];

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pConstantBufferChunk->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_sizePerConstantBuffer;

	UINT8* pSystemMemPtr = m_pSystemAddressForStart;
	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_pCbvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	const UINT descriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (DWORD i = 0; i < m_maxCbvCount; i++)
	{
		pD3DDevice->CreateConstantBufferView(&cbvDesc, descriptorHandle);

		m_pConstantBufferContainerList[i].CbvDescriptorHandle = descriptorHandle;
		m_pConstantBufferContainerList[i].GpuAddress = cbvDesc.BufferLocation;
		m_pConstantBufferContainerList[i].SystemAddress = pSystemMemPtr;

		descriptorHandle.Offset(1, descriptorSize);
		cbvDesc.BufferLocation += m_sizePerConstantBuffer;
		pSystemMemPtr += m_sizePerConstantBuffer;
	}

	return bResult = true;
}

ConstantBufferContainer* CConstantBufferPool::Allocate()
{
	ConstantBufferContainer* pConstantBufferContainer = nullptr;

	if (m_allocatedCbvCount >= m_maxCbvCount)
	{
		return pConstantBufferContainer;
	}

	pConstantBufferContainer = m_pConstantBufferContainerList + m_allocatedCbvCount;
	m_allocatedCbvCount++;

	return pConstantBufferContainer;
}

void CConstantBufferPool::Reset()
{
	m_allocatedCbvCount = 0;
}

void CConstantBufferPool::Cleanup()
{
	if (m_pConstantBufferContainerList)
	{
		delete[] m_pConstantBufferContainerList;
		m_pConstantBufferContainerList = nullptr;
	}

	if (m_pConstantBufferChunk)
	{
		m_pConstantBufferChunk->Release();
		m_pConstantBufferChunk = nullptr;
	}

	if (m_pCbvDescriptorHeap)
	{
		m_pCbvDescriptorHeap->Release();
		m_pCbvDescriptorHeap = nullptr;
	}
}