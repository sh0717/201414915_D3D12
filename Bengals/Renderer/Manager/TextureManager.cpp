#include "pch.h"
#include <d3d12.h>
#include "TextureManager.h"
#include "../../Types/typedef.h"
#include "../D3D12Renderer.h"
#include "CD3D12ResourceManager.h"
#include "../RenderHelper/CpuDescriptorFreeListAllocator.h"

CTextureManager::~CTextureManager()
{
	Cleanup();
}

bool CTextureManager::Initialize(CD3D12Renderer* pRenderer)
{
	m_pRenderer = pRenderer;
	m_pD3DDevice = pRenderer->GetD3DDevice();
	m_pResourceManager = pRenderer->GetResourceManager();
	m_pDescriptorAllocator = pRenderer->GetDescriptorAllocator();

	return true;
}

TextureHandle* CTextureManager::AllocTextureHandle()
{
	TextureHandle* pTexHandle = new TextureHandle{};
	pTexHandle->RefCount = 1;
	return pTexHandle;
}

DWORD CTextureManager::FreeTextureHandle(TextureHandle* pTexHandle)
{
	if (!pTexHandle)
	{
		return 0;
	}

	if (!pTexHandle->RefCount)
	{
		__debugbreak();
	}

	DWORD refCount = --pTexHandle->RefCount;
	if (refCount == 0)
	{
		if (pTexHandle->TextureResource)
		{
			pTexHandle->TextureResource->Release();
			pTexHandle->TextureResource = nullptr;
		}

		if (pTexHandle->pUploadBuffer)
		{
			pTexHandle->pUploadBuffer->Release();
			pTexHandle->pUploadBuffer = nullptr;
		}

		if (pTexHandle->SrvDescriptorHandle.ptr)
		{
			m_pDescriptorAllocator->Free(pTexHandle->SrvDescriptorHandle);
		}

		delete pTexHandle;
	}

	return refCount;
}

bool CTextureManager::CreateSrvForTexture(TextureHandle* pTexHandle, ID3D12Resource* pTexResource, DXGI_FORMAT format, UINT mipLevels)
{
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	if (!m_pDescriptorAllocator->Allocate(&srv))
	{
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = mipLevels;

	m_pD3DDevice->CreateShaderResourceView(pTexResource, &srvDesc, srv);

	pTexHandle->TextureResource = pTexResource;
	pTexHandle->SrvDescriptorHandle = srv;
	return true;
}

TextureHandle* CTextureManager::CreateTextureFromFile(const WCHAR* filePath)
{
	// dedup: 동일 파일이 이미 로드되어 있으면 참조 카운트 증가 후 반환
	std::wstring key(filePath);
	auto it = m_fileTextureMap.find(key);
	if (it != m_fileTextureMap.end())
	{
		it->second->RefCount++;
		return it->second;
	}

	ID3D12Resource* pTexResource = nullptr;
	D3D12_RESOURCE_DESC texDesc = {};

	if (!m_pResourceManager->CreateTextureFromFile(&pTexResource, &texDesc, filePath))
	{
		return nullptr;
	}

	TextureHandle* pTexHandle = AllocTextureHandle();
	pTexHandle->bFromFile = true;
	pTexHandle->FilePath = key;

	if (!CreateSrvForTexture(pTexHandle, pTexResource, texDesc.Format, texDesc.MipLevels))
	{
		pTexResource->Release();
		delete pTexHandle;
		return nullptr;
	}

	m_fileTextureMap[key] = pTexHandle;
	return pTexHandle;
}

TextureHandle* CTextureManager::CreateDynamicTexture(UINT texWidth, UINT texHeight)
{
	ID3D12Resource* pTexResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;

	DXGI_FORMAT texFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (!m_pResourceManager->CreateTexturePair(&pTexResource, &pUploadBuffer, texWidth, texHeight, texFormat))
	{
		return nullptr;
	}

	TextureHandle* pTexHandle = AllocTextureHandle();

	if (!CreateSrvForTexture(pTexHandle, pTexResource, texFormat, 1))
	{
		pTexResource->Release();
		pUploadBuffer->Release();
		delete pTexHandle;
		return nullptr;
	}

	pTexHandle->pUploadBuffer = pUploadBuffer;
	pTexHandle->bUpdated = false;
	return pTexHandle;
}

TextureHandle* CTextureManager::CreateStaticTexture(UINT texWidth, UINT texHeight, DXGI_FORMAT format, const BYTE* pInitImage)
{
	TextureHandle* pTexHandle = nullptr;
	ID3D12Resource* pTexResource = nullptr;

	if (m_pResourceManager->CreateTexture(&pTexResource, texWidth, texHeight, format, pInitImage))
	{
		pTexHandle = AllocTextureHandle();
		if (!CreateSrvForTexture(pTexHandle, pTexResource, format, 1))
		{
			pTexResource->Release();
			delete pTexHandle;
			pTexHandle = nullptr;
		}
	}

	return pTexHandle;
}

void CTextureManager::UpdateTextureWithImage(TextureHandle* pTexHandle, const BYTE* pSrcBits, UINT srcWidth, UINT srcHeight)
{
	ID3D12Resource* pDestTexResource = pTexHandle->TextureResource;
	ID3D12Resource* pUploadBuffer = pTexHandle->pUploadBuffer;

	D3D12_RESOURCE_DESC desc = pDestTexResource->GetDesc();
	if (srcWidth > desc.Width)
	{
		__debugbreak();
	}
	if (srcHeight > desc.Height)
	{
		__debugbreak();
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT rows = 0;
	UINT64 rowSize = 0;
	UINT64 totalBytes = 0;

	m_pD3DDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &rowSize, &totalBytes);

	BYTE* pMappedPtr = nullptr;
	CD3DX12_RANGE writeRange(0, 0);

	HRESULT hr = pUploadBuffer->Map(0, &writeRange, reinterpret_cast<void**>(&pMappedPtr));
	if (FAILED(hr))
		__debugbreak();

	const BYTE* pSrc = pSrcBits;
	BYTE* pDest = pMappedPtr;
	for (UINT y = 0; y < srcHeight; y++)
	{
		memcpy(pDest, pSrc, srcWidth * 4);
		pSrc += (srcWidth * 4);
		pDest += footprint.Footprint.RowPitch;
	}

	pUploadBuffer->Unmap(0, nullptr);

	pTexHandle->bUpdated = TRUE;
}

void CTextureManager::DeleteTexture(TextureHandle* pTexHandle)
{
	if (!pTexHandle)
	{
		return;
	}

	// 파일 텍스처이고 마지막 참조이면 맵에서 제거
	if (pTexHandle->bFromFile && pTexHandle->RefCount == 1)
	{
		m_fileTextureMap.erase(pTexHandle->FilePath);
	}

	FreeTextureHandle(pTexHandle);
}

void CTextureManager::Cleanup()
{
	if (!m_fileTextureMap.empty())
	{
		// texture resource leak
		__debugbreak();
	}
}
