#include "pch.h"
#include "GpuDescriptorLinearAllocator.h"

bool CGpuDescriptorLinearAllocator::Initialize(ID3D12Device5* pD3DDevice, UINT maxDescriptorCount)
{
	if (pD3DDevice == nullptr)
	{
		__debugbreak();
		return false;
	}

	m_maxDescriptorCount = maxDescriptorCount;
	m_descriptorSizeCbvSrvUav = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_maxDescriptorCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	const HRESULT hr = pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	m_cpuDescriptorHandleForHeapStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_gpuDescriptorHandleForHeapStart = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	return true;
}

bool CGpuDescriptorLinearAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuDescriptorHandle, const UINT descriptorCount)
{
	if ((m_allocatedDescriptorCount + descriptorCount) > m_maxDescriptorCount)
	{
		return false;
	}

	*pOutCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_cpuDescriptorHandleForHeapStart, static_cast<INT>(m_allocatedDescriptorCount), m_descriptorSizeCbvSrvUav };
	*pOutGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_gpuDescriptorHandleForHeapStart, static_cast<INT>(m_allocatedDescriptorCount), m_descriptorSizeCbvSrvUav };

	m_allocatedDescriptorCount += descriptorCount;

	return true;
}

void CGpuDescriptorLinearAllocator::Reset()
{
	m_allocatedDescriptorCount = 0;
}