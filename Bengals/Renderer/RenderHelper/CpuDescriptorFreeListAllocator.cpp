#include "pch.h"
#include "CpuDescriptorFreeListAllocator.h"

bool CCpuDescriptorFreeListAllocator::Initialize(ID3D12Device* pD3DDevice, const UINT maxDescriptorCount)
{
	if (pD3DDevice == nullptr)
	{
		__debugbreak();
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	commonHeapDesc.NumDescriptors = maxDescriptorCount;
	commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	commonHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(m_pDescriptorHeap.ReleaseAndGetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	m_indexCreator.Initialize(maxDescriptorCount);
	m_descriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	return true;
}

bool CCpuDescriptorFreeListAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle)
{
	DWORD foundIndex = m_indexCreator.Alloc();
	if (foundIndex == -1)
	{
		return false;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), foundIndex, m_descriptorSize);
	*pOutCpuDescriptorHandle = descriptorHandle;
	return true;
}

void CCpuDescriptorFreeListAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE inCpuDescriptorHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleForHeapStart = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	if (descriptorHandleForHeapStart.ptr > inCpuDescriptorHandle.ptr)
	{
		__debugbreak();
	}

	UINT index = (inCpuDescriptorHandle.ptr - descriptorHandleForHeapStart.ptr) / m_descriptorSize;
	m_indexCreator.Free(index);
}


