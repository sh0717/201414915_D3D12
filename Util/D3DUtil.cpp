#include "pch.h"
#include <dxgi1_4.h>
#include <d3d12.h>
#include <d3dx12.h>
#include "D3DUtil.h"

static void EnsureDebugConsole()
{
#ifdef _DEBUG
	static bool s_consoleReady = false;
	if (s_consoleReady)
	{
		return;
	}
	s_consoleReady = true;

	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		AllocConsole();
	}

	FILE* fpOut = nullptr;
	freopen_s(&fpOut, "CONOUT$", "w", stdout);
	FILE* fpErr = nullptr;
	freopen_s(&fpErr, "CONOUT$", "w", stderr);

	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);
#endif
}

void DebugLog(const char* fmt, ...)
{
#ifdef _DEBUG
	EnsureDebugConsole();
	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	fprintf(stdout, "\n");
	va_end(args);
#else
	(void)fmt;
#endif
}

void DebugLogW(const wchar_t* fmt, ...)
{
#ifdef _DEBUG
	EnsureDebugConsole();
	va_list args;
	va_start(args, fmt);
	vfwprintf(stdout, fmt, args);
	fputws(L"\n", stdout);
	va_end(args);
#else
	(void)fmt;
#endif
}



void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	IDXGIAdapter1* adapter = nullptr;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	*ppAdapter = adapter;
}

void GetSoftwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	IDXGIAdapter1* adapter = nullptr;
	*ppAdapter = nullptr;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
			{
				*ppAdapter = adapter;
				break;
			}
		}
	}
}

void SetDebugLayerInfo(ID3D12Device* InD3DDevice)
{
	if (InD3DDevice == nullptr)
	{
		assert(false, "D3DUtil::SetDebugLayerInfo ,D3DDevice is not valid");
		return;
	}

	ID3D12InfoQueue*	pInfoQueue = nullptr;
	InD3DDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
	if (pInfoQueue)
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			// Workarounds for debug layer issues on hybrid-graphics systems
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = (UINT)_countof(hide);
		filter.DenyList.pIDList = hide;
		pInfoQueue->AddStorageFilterEntries(&filter);

		pInfoQueue->Release();
		pInfoQueue = nullptr;
	}
}

void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* pOutSamplerDesc, UINT RegisterIndex)
{
	if (!pOutSamplerDesc)
	{
		__debugbreak();
	}

	pOutSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pOutSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pOutSamplerDesc->MipLODBias = 0.0f;
	pOutSamplerDesc->MaxAnisotropy = 16;
	pOutSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	pOutSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	pOutSamplerDesc->MinLOD = -FLT_MAX;
	pOutSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
	pOutSamplerDesc->ShaderRegister = RegisterIndex;
	pOutSamplerDesc->RegisterSpace = 0;
	pOutSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
}

void UpdateTexture(ID3D12Device* pD3DDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource)
{
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint[MAX_SUB_RESOURCE_NUM] = {};
	UINT	Rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	RowSize[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	TotalBytes = 0;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (Desc.MipLevels > (UINT)_countof(Footprint))
		__debugbreak();

	pD3DDevice->GetCopyableFootprints(&Desc, 0, Desc.MipLevels, 0, Footprint, Rows, RowSize, &TotalBytes);

	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	for (DWORD i = 0; i < Desc.MipLevels; i++)
	{

		D3D12_TEXTURE_COPY_LOCATION	destLocation = {};
		destLocation.PlacedFootprint = Footprint[i];
		destLocation.pResource = pDestTexResource;
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION	srcLocation = {};
		srcLocation.PlacedFootprint = Footprint[i];
		srcLocation.pResource = pSrcTexResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

}

