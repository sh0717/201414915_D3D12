#include "pch.h"
#include "ShaderVisibleDescriptorTableAllocator.h"

bool CShaderVisibleDescriptorTableAllocator::Initialize(ID3D12Device5* pD3DDevice, UINT maxDescriptorCount)
{
	bool bResult = false;

	if (pD3DDevice == nullptr)
	{
		__debugbreak();
		return bResult;
	}

	m_pD3DDevice = pD3DDevice;
	m_maxDescriptorCount = maxDescriptorCount;
	m_descriptorSizeCbvSrvUav = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = m_maxDescriptorCount;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	const HRESULT hr = m_pD3DDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	if (m_descriptorHeap)
	{
		m_cpuDescriptorHandleForHeapStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_gpuDescriptorHandleForHeapStart = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	}
	else
	{
		__debugbreak();
		return bResult;
	}

	return bResult = true;
}

bool CShaderVisibleDescriptorTableAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pOutGpuDescriptorHandle, UINT descriptorCount)
{
	bool bResult = false;
	if ((m_allocatedDescriptorCount + descriptorCount) > m_maxDescriptorCount)
	{
		return bResult;
	}

	*pOutCpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE{ m_cpuDescriptorHandleForHeapStart, m_allocatedDescriptorCount, m_descriptorSizeCbvSrvUav };
	*pOutGpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE{ m_gpuDescriptorHandleForHeapStart, m_allocatedDescriptorCount, m_descriptorSizeCbvSrvUav };

	m_allocatedDescriptorCount += descriptorCount;

	return bResult = true;
}

void CShaderVisibleDescriptorTableAllocator::Reset()
{
	m_allocatedDescriptorCount = 0;
}