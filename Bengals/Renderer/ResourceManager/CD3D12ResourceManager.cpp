#include "pch.h"
#include "CD3D12ResourceManager.h"

#include <cassert>


CD3D12ResourceManager::~CD3D12ResourceManager()
{
	Clean();
}

bool CD3D12ResourceManager::Initialize(ID3D12Device5* pD3DDevice)
{
	bool bResult = false;

	m_pD3DDevice = pD3DDevice;
	assert(m_pD3DDevice);

	HRESULT hr;

	D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = m_pD3DDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_pCommandQueue));
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	hr = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_pCommandAllocator));

	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	hr = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator, nullptr, IID_PPV_ARGS(&m_pCommandList));

	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}
	m_pCommandList->Close();


	hr = m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	m_fenceValue = 0;
	return bResult = true;
}

HRESULT CD3D12ResourceManager::CreateVertexBuffer(UINT sizePerVertex, UINT vertexCount, D3D12_VERTEX_BUFFER_VIEW* pOutVertexBufferView, ID3D12Resource** ppOutBuffer, void* pInInitData)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};

	ID3D12Resource* pVertexBufferInGPU = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT bufferSize = sizePerVertex * vertexCount;

	assert(m_pD3DDevice);
	hr = m_pD3DDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), 
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pVertexBufferInGPU));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	if (pInInitData)
	{
		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON,
			nullptr, IID_PPV_ARGS(&pUploadBuffer));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}
		
		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		hr = pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		memcpy(pVertexDataBegin, pInInitData, bufferSize);
		pUploadBuffer->Unmap(0, nullptr);

		hr = m_pCommandAllocator->Reset();
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		hr = m_pCommandList->Reset(m_pCommandAllocator, nullptr);
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBufferInGPU, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pVertexBufferInGPU, 0, pUploadBuffer.Get(), 0, bufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pVertexBufferInGPU, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();
	}

	vertexBufferView.BufferLocation = pVertexBufferInGPU->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = bufferSize;
	vertexBufferView.StrideInBytes = sizePerVertex;

	*pOutVertexBufferView = vertexBufferView;
	*ppOutBuffer = pVertexBufferInGPU;

	return hr;
}

HRESULT CD3D12ResourceManager::CreateIndexBuffer(UINT indexCount, D3D12_INDEX_BUFFER_VIEW* pOutIndexBufferView, ID3D12Resource** ppOutBuffer, void* pInitData)
{
	HRESULT hr = S_OK;


	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};

	ID3D12Resource* pIndexBuffer = nullptr;
	ComPtr<ID3D12Resource> pUploadBuffer = nullptr;
	UINT bufferSize = indexCount * sizeof(WORD);

	assert(m_pD3DDevice);
	hr = m_pD3DDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&pIndexBuffer));

	if (FAILED(hr))
	{
		__debugbreak();
		return hr;
	}

	if (pInitData)
	{
		hr = m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_COMMON,
			nullptr, IID_PPV_ARGS(pUploadBuffer.ReleaseAndGetAddressOf()));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		hr = pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin));

		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		memcpy(pIndexDataBegin, pInitData, bufferSize);
		pUploadBuffer->Unmap(0, nullptr);

		hr = m_pCommandAllocator->Reset();
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		hr = m_pCommandList->Reset(m_pCommandAllocator, nullptr);
		if (FAILED(hr))
		{
			__debugbreak();
			return hr;
		}

		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
		m_pCommandList->CopyBufferRegion(pIndexBuffer, 0, pUploadBuffer.Get(), 0, bufferSize);
		m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pIndexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));
		m_pCommandList->Close();

		ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
		m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		Fence();
		WaitForFenceValue();
	}

	indexBufferView.BufferLocation = pIndexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = bufferSize;


	*pOutIndexBufferView = indexBufferView;
	*ppOutBuffer = pIndexBuffer;

	return hr;
}

void CD3D12ResourceManager::UpdateTextureForWrite(ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource)
{
	const DWORD MAX_SUB_RESOURCE_NUM = 32;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint[MAX_SUB_RESOURCE_NUM] = {};
	UINT	Rows[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	RowSize[MAX_SUB_RESOURCE_NUM] = {};
	UINT64	TotalBytes = 0;

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (Desc.MipLevels > (UINT)_countof(Footprint))
		__debugbreak();

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, Desc.MipLevels, 0, Footprint, Rows, RowSize, &TotalBytes);

	if (FAILED(m_pCommandAllocator->Reset()))
		__debugbreak();

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator, nullptr)))
		__debugbreak();

	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
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

		m_pCommandList->CopyTextureRegion(&destLocation, 0, 0, 0, &srcLocation, nullptr);
	}
	m_pCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(pDestTexResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE));
	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	Fence();
	WaitForFenceValue();
}

BOOL CD3D12ResourceManager::CreateTexture(ID3D12Resource** ppOutResource, UINT width, UINT height, DXGI_FORMAT format,
	const BYTE* pInitImage)
{
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;	// ex) DXGI_FORMAT_R8G8B8A8_UNORM, etc...
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
		IID_PPV_ARGS(&pTexResource))))
	{
		__debugbreak();
	}

	if (pInitImage)
	{
		D3D12_RESOURCE_DESC Desc = pTexResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
		UINT	Rows = 0;
		UINT64	RowSize = 0;
		UINT64	TotalBytes = 0;

		m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

		BYTE* pMappedPtr = nullptr;
		CD3DX12_RANGE writeRange(0, 0);

		UINT64 uploadBufferSize = GetRequiredIntermediateSize(pTexResource, 0, 1);

		if (FAILED(m_pD3DDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&pUploadBuffer))))
		{
			__debugbreak();
		}

		HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
		if (FAILED(hr))
			__debugbreak();

		const BYTE* pSrc = pInitImage;
		BYTE* pDest = pMappedPtr;
		for (UINT y = 0; y < height; y++)
		{
			memcpy(pDest, pSrc, width * 4);
			pSrc += (width * 4);
			pDest += Footprint.Footprint.RowPitch;
		}
		// Unmap
		pUploadBuffer->Unmap(0, nullptr);

		UpdateTextureForWrite(pTexResource, pUploadBuffer);

		pUploadBuffer->Release();
		pUploadBuffer = nullptr;

	}
	*ppOutResource = pTexResource;

	return TRUE;
}

UINT64 CD3D12ResourceManager::Fence()
{
	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence, m_fenceValue);
	return m_fenceValue;
}

void CD3D12ResourceManager::WaitForFenceValue() const
{
	UINT64 expectedFence = m_fenceValue;
	if (m_pFence->GetCompletedValue() < expectedFence)
	{
		m_pFence->SetEventOnCompletion(expectedFence, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
}

void CD3D12ResourceManager::Clean()
{
	WaitForFenceValue();

	if (m_pCommandQueue)
	{
		m_pCommandQueue->Release();
		m_pCommandQueue = nullptr;
	}

	if (m_pCommandList)
	{
		m_pCommandList->Release();
		m_pCommandList = nullptr;
	}

	if (m_pCommandAllocator)
	{
		m_pCommandAllocator->Release();
		m_pCommandAllocator = nullptr;
	}

	if (m_fenceEvent)
	{
		CloseHandle(m_fenceEvent);
		m_fenceEvent = nullptr;
	}

	if (m_pFence)
	{
		m_pFence->Release();
		m_pFence = nullptr;
	}
}

