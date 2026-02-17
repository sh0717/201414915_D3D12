#include "pch.h"
#include "CD3D12ResourceManager.h"
#include <DDSTextureLoader.h>


CD3D12ResourceManager::~CD3D12ResourceManager()
{
	WaitForFenceValue();

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}
}

bool CD3D12ResourceManager::Initialize(ID3D12Device5* pD3DDevice)
{
	if (!pD3DDevice)
	{
		__debugbreak();
		return false;
	}
	m_pD3DDevice = pD3DDevice;
	HRESULT hr;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = m_pD3DDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	hr = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	hr = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}
	m_commandList->Close();

	hr = m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (!m_fenceEvent)
	{
		__debugbreak();
		return false;
	}

	m_fenceValue = 0;
	return true;
}

HRESULT CD3D12ResourceManager::CreateVertexBuffer(const UINT sizePerVertex, const UINT vertexCount, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, const void* pInInitData)
{
	*pOutVertexBufferView = {};
	*ppOutBuffer = nullptr;

	if (!m_pD3DDevice || !pInInitData)
	{
		__debugbreak();
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	UINT bufferSize = sizePerVertex * vertexCount;

	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(vertexBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}
	
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}
	
	UINT8* pVertexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	hr = uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	memcpy(pVertexDataBegin, pInInitData, bufferSize);
	uploadBuffer->Unmap(0, nullptr);

	hr = m_commandAllocator->Reset();
	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	m_commandList->CopyBufferRegion(vertexBuffer.Get(), 0, uploadBuffer.Get(), 0, bufferSize);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	/////////////////
	Fence();
	WaitForFenceValue();
	////////////////

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = bufferSize;
	vertexBufferView.StrideInBytes = sizePerVertex;

	*pOutVertexBufferView = vertexBufferView;
	*ppOutBuffer = vertexBuffer.Detach();

	return hr;
}

HRESULT CD3D12ResourceManager::CreateIndexBuffer(const UINT indexCount, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, const void* pInitData)
{
	*pOutIndexBufferView = {};
	*ppOutBuffer = nullptr;

	if (!m_pD3DDevice || !pInitData)
	{
		__debugbreak();
		return E_FAIL;
	}

	HRESULT hr = S_OK;

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	ComPtr<ID3D12Resource> indexBuffer;
	ComPtr<ID3D12Resource> uploadBuffer;
	UINT bufferSize = indexCount * sizeof(WORD);

	hr = m_pD3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(indexBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf()));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	UINT8* pIndexDataBegin = nullptr;
	CD3DX12_RANGE readRange(0, 0);
	hr = uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	memcpy(pIndexDataBegin, pInitData, bufferSize);
	uploadBuffer->Unmap(0, nullptr);

	hr = m_commandAllocator->Reset();
	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	hr = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	m_commandList->CopyBufferRegion(indexBuffer.Get(), 0, uploadBuffer.Get(), 0, bufferSize);
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
	m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = bufferSize;

	*pOutIndexBufferView = indexBufferView;
	*ppOutBuffer = indexBuffer.Detach();

	return hr;
}

void CD3D12ResourceManager::UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource)
{
	constexpr UINT MAX_SUB_RESOURCE_COUNT = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint[MAX_SUB_RESOURCE_COUNT] = {};
	UINT	rows[MAX_SUB_RESOURCE_COUNT] = {};
	UINT64	rowSize[MAX_SUB_RESOURCE_COUNT] = {};
	UINT64	totalBytes = 0;

	D3D12_RESOURCE_DESC desc = pDestTexResource->GetDesc();
	if (desc.MipLevels > static_cast<UINT>(_countof(footprint)))
		__debugbreak();

	m_pD3DDevice->GetCopyableFootprints(&desc, 0, desc.MipLevels, 0, footprint, rows, rowSize, &totalBytes);

	if (FAILED(m_commandAllocator->Reset()))
		__debugbreak();

	if (FAILED(m_commandList->Reset(m_commandAllocator.Get(), nullptr)))
		__debugbreak();

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	for (DWORD i = 0; i < desc.MipLevels; i++)
	{
		D3D12_TEXTURE_COPY_LOCATION	destLocation = {};
		destLocation.PlacedFootprint = footprint[i];
		destLocation.pResource = pDestTexResource;
		destLocation.SubresourceIndex = i;
		destLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

		D3D12_TEXTURE_COPY_LOCATION	srcLocation = {};
		srcLocation.PlacedFootprint = footprint[i];
		srcLocation.pResource = pSrcTexResource;
		srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

		m_commandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
	m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();
}

bool CD3D12ResourceManager::CreateTexture(ID3D12Resource** ppOutResource, UINT width, UINT height, DXGI_FORMAT format, const BYTE* pInitImage)
{
	if (!ppOutResource)
	{
		__debugbreak();
		return false;
	}

	ComPtr<ID3D12Resource> texResource;
	ComPtr<ID3D12Resource> uploadBuffer;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(texResource.ReleaseAndGetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	if (pInitImage)
	{
		D3D12_RESOURCE_DESC Desc = texResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
		UINT	Rows = 0;
		UINT64	RowSize = 0;
		UINT64	TotalBytes = 0;

		m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

		BYTE* pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		UINT64 uploadBufferSize = GetRequiredIntermediateSize(texResource.Get(), 0, 1);

		if (FAILED(m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf()))))
		{
			__debugbreak();
			return false;
		}

		HRESULT hr = uploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
		if (FAILED(hr))
		{
			__debugbreak();
			return false;
		}

		const BYTE* pSrc = pInitImage;
		BYTE* pDest = pMappedPtr;
		for (UINT y = 0; y < height; y++)
		{
			memcpy(pDest, pSrc, width * 4);
			pSrc += (width * 4);
			pDest += Footprint.Footprint.RowPitch;
		}
		uploadBuffer->Unmap(0, nullptr);

		UpdateTextureForWrite(texResource.Get(), uploadBuffer.Get());
	}
	*ppOutResource = texResource.Detach();

	return true;
}

bool CD3D12ResourceManager::CreateTextureFromFile(ID3D12Resource** ppOutResource, D3D12_RESOURCE_DESC* pOutDesc, const WCHAR* inFileName)
{
	if (!ppOutResource || !pOutDesc)
	{
		__debugbreak();
		return false;
	}

	*ppOutResource = nullptr;
	*pOutDesc = {};

	ComPtr<ID3D12Resource> texResource;
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;

	HRESULT hr = DirectX::LoadDDSTextureFromFile(
		m_pD3DDevice,
		inFileName,
		texResource.ReleaseAndGetAddressOf(),
		ddsData,
		subresources);
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	UINT subresourceCount = static_cast<UINT>(subresources.size());
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(texResource.Get(), 0, subresourceCount);

	ComPtr<ID3D12Resource> uploadBuffer;
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf()));
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	if (FAILED(m_commandAllocator->Reset()))
	{
		__debugbreak();
		return false;
	}
	if (FAILED(m_commandList->Reset(m_commandAllocator.Get(), nullptr)))
	{
		__debugbreak();
		return false;
	}

	m_commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			texResource.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	UpdateSubresources(
		m_commandList.Get(),
		texResource.Get(),
		uploadBuffer.Get(),
		0, 0,
		subresourceCount,
		subresources.data());

	m_commandList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			texResource.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));

	m_commandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();

	*pOutDesc = texResource->GetDesc();
	*ppOutResource = texResource.Detach();
	return true;
}

UINT64 CD3D12ResourceManager::Fence()
{
	m_fenceValue++;
	m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
	return m_fenceValue;
}

void CD3D12ResourceManager::WaitForFenceValue() const
{
	UINT64 expectedFence = m_fenceValue;
	if (m_fence->GetCompletedValue() < expectedFence)
	{
		m_fence->SetEventOnCompletion(expectedFence, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

