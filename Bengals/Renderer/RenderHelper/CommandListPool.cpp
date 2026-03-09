#include "pch.h"
#include "CommandListPool.h"

CCommandListPool::CCommandListPool() = default;
CCommandListPool::~CCommandListPool()
{
	Cleanup();
}

bool CCommandListPool::Initialize(ID3D12Device5* pD3DDevice, D3D12_COMMAND_LIST_TYPE type, UINT maxCommandListCount)
{
	m_pD3DDevice = pD3DDevice;
	m_commandListType = type;
	m_maxCommandListCount = maxCommandListCount;
	m_allCommandListEntryList.clear();
	m_allCommandListEntryList.reserve(m_maxCommandListCount);
	m_availableCommandListEntryList.clear();
	m_availableCommandListEntryList.reserve(m_maxCommandListCount);
	m_allocatedCommandListEntryList.clear();
	m_allocatedCommandListEntryList.reserve(m_maxCommandListCount);
	m_pCurrentCommandListEntry = nullptr;
	return (m_pD3DDevice != nullptr && m_maxCommandListCount > 0);
}

ID3D12GraphicsCommandList* CCommandListPool::GetCurrentCommandList()
{
	if (!m_pCurrentCommandListEntry)
	{
		m_pCurrentCommandListEntry = AllocateCommandList();
		if (!m_pCurrentCommandListEntry)
		{
			__debugbreak();
			return nullptr;
		}
	}

	if (m_pCurrentCommandListEntry->bClosed)
	{
		__debugbreak();
		return nullptr;
	}

	return m_pCurrentCommandListEntry->CommandList.Get();
}

void CCommandListPool::Close()
{
	if (!m_pCurrentCommandListEntry)
	{
		__debugbreak();
		return;
	}

	if (m_pCurrentCommandListEntry->bClosed)
	{
		__debugbreak();
		return;
	}

	if (FAILED(m_pCurrentCommandListEntry->CommandList->Close()))
	{
		__debugbreak();
		return;
	}

	m_pCurrentCommandListEntry->bClosed = true;
	m_pCurrentCommandListEntry = nullptr;
}

void CCommandListPool::CloseAndExecute(ID3D12CommandQueue* pCommandQueue)
{
	if (!pCommandQueue)
	{
		__debugbreak();
		return;
	}

	if (!m_pCurrentCommandListEntry)
	{
		__debugbreak();
		return;
	}

	if (m_pCurrentCommandListEntry->bClosed)
	{
		__debugbreak();
		return;
	}

	if (FAILED(m_pCurrentCommandListEntry->CommandList->Close()))
	{
		__debugbreak();
		return;
	}

	m_pCurrentCommandListEntry->bClosed = true;
	ID3D12CommandList* pCommandList = m_pCurrentCommandListEntry->CommandList.Get();
	pCommandQueue->ExecuteCommandLists(1, &pCommandList);
	m_pCurrentCommandListEntry = nullptr;
}

void CCommandListPool::Reset()
{
	if (m_pCurrentCommandListEntry)
	{
		__debugbreak();
		return;
	}

	for (CommandListEntry* pCommandListEntry : m_allocatedCommandListEntryList)
	{
		if (!pCommandListEntry || !pCommandListEntry->CommandAllocator || !pCommandListEntry->CommandList)
		{
			__debugbreak();
			continue;
		}

		if (FAILED(pCommandListEntry->CommandAllocator->Reset()))
		{
			__debugbreak();
			continue;
		}

		if (FAILED(pCommandListEntry->CommandList->Reset(pCommandListEntry->CommandAllocator.Get(), nullptr)))
		{
			__debugbreak();
			continue;
		}

		pCommandListEntry->bClosed = false;
		m_availableCommandListEntryList.push_back(pCommandListEntry);
	}

	m_allocatedCommandListEntryList.clear();
}

UINT CCommandListPool::GetTotalCommandListCount() const
{
	return static_cast<UINT>(m_allCommandListEntryList.size());
}

UINT CCommandListPool::GetAvailableCommandListCount() const
{
	return static_cast<UINT>(m_availableCommandListEntryList.size());
}

UINT CCommandListPool::GetAllocatedCommandListCount() const
{
	return static_cast<UINT>(m_allocatedCommandListEntryList.size());
}

bool CCommandListPool::AddCommandList()
{
	if (!m_pD3DDevice || m_allCommandListEntryList.size() >= m_maxCommandListCount)
	{
		__debugbreak();
		return false;
	}

	ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;

	if (FAILED(m_pD3DDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(commandAllocator.ReleaseAndGetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	if (FAILED(m_pD3DDevice->CreateCommandList(0, m_commandListType, commandAllocator.Get(), nullptr, IID_PPV_ARGS(commandList.ReleaseAndGetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	m_allCommandListEntryList.emplace_back();
	CommandListEntry& commandListEntry = m_allCommandListEntryList.back();
	commandListEntry.CommandAllocator = commandAllocator;
	commandListEntry.CommandList = commandList;
	commandListEntry.bClosed = false;
	m_availableCommandListEntryList.push_back(&commandListEntry);
	return true;
}

CCommandListPool::CommandListEntry* CCommandListPool::AllocateCommandList()
{
	if (m_availableCommandListEntryList.empty())
	{
		if (!AddCommandList())
		{
			return nullptr;
		}
	}

	CommandListEntry* pCommandListEntry = m_availableCommandListEntryList.back();
	m_availableCommandListEntryList.pop_back();
	m_allocatedCommandListEntryList.push_back(pCommandListEntry);
	return pCommandListEntry;
}

void CCommandListPool::Cleanup()
{
	m_availableCommandListEntryList.clear();
	m_allocatedCommandListEntryList.clear();
	m_allCommandListEntryList.clear();
	m_pCurrentCommandListEntry = nullptr;
	m_maxCommandListCount = 0;
	m_pD3DDevice = nullptr;
}
