#include <pch.h>
#include "DescriptorAllocator.h"




CDescriptorAllocator::~CDescriptorAllocator()
{
	Cleanup();
}

 bool CDescriptorAllocator::Initialize(ID3D12Device* pD3DDevice, UINT maxDescriptorCount, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	 bool bResult = false;

	 D3D12_DESCRIPTOR_HEAP_DESC commonHeapDesc = {};
	 commonHeapDesc.NumDescriptors = maxDescriptorCount;
	 commonHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	 commonHeapDesc.Flags = flags;

	 if (FAILED(pD3DDevice->CreateDescriptorHeap(&commonHeapDesc, IID_PPV_ARGS(&m_pDescriptorHeap))))
	 {
		 __debugbreak();
		 return bResult;
	 }

	 m_indexCreator.Initialize(maxDescriptorCount);
	 m_descriptorSize = pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	 return bResult = true;
}

bool CDescriptorAllocator::Allocate(D3D12_CPU_DESCRIPTOR_HANDLE* pOutCpuDescriptorHandle)
{
	bool bResult = false;

	DWORD foundIndex = m_indexCreator.Alloc();
	if (-1 != foundIndex)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), foundIndex, m_descriptorSize);
		*pOutCpuDescriptorHandle = descriptorHandle;
		bResult = TRUE;
	}
	return bResult;
}

void CDescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE inCpuDescriptorHandle)
{
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleForHeapStart = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	if (descriptorHandleForHeapStart.ptr > inCpuDescriptorHandle.ptr)
	{
		__debugbreak();
	}

	UINT index = (inCpuDescriptorHandle.ptr - descriptorHandleForHeapStart.ptr) / m_descriptorSize;
	m_indexCreator.Free(index);
}


void CDescriptorAllocator::Cleanup()
{
	if (m_pDescriptorHeap)
	{
		m_pDescriptorHeap->Release();
		m_pDescriptorHeap = nullptr;
	}
}

