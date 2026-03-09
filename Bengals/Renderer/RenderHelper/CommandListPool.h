#pragma once

#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

class CCommandListPool
{
public:
	struct CommandListEntry
	{
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator = nullptr;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = nullptr;
		bool bClosed = false;
	};

public:
	CCommandListPool();
	~CCommandListPool();

	bool Initialize(ID3D12Device5* pD3DDevice, D3D12_COMMAND_LIST_TYPE type, UINT maxCommandListCount);

	ID3D12GraphicsCommandList* GetCurrentCommandList();
	void Close();
	void CloseAndExecute(ID3D12CommandQueue* pCommandQueue);
	void Reset();

	UINT GetTotalCommandListCount() const;
	UINT GetAvailableCommandListCount() const;
	UINT GetAllocatedCommandListCount() const;

private:
	bool AddCommandList();
	CommandListEntry* AllocateCommandList();
	void Cleanup();

private:
	ID3D12Device5* m_pD3DDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE m_commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	UINT m_maxCommandListCount = 0;

	std::vector<CommandListEntry> m_allCommandListEntryList = {};
	std::vector<CommandListEntry*> m_availableCommandListEntryList = {};
	std::vector<CommandListEntry*> m_allocatedCommandListEntryList = {};
	CommandListEntry* m_pCurrentCommandListEntry = nullptr;
};
