#pragma once

#include <unordered_map>
#include <string>

class CD3D12Renderer;
class CD3D12ResourceManager;
class CCpuDescriptorFreeListAllocator;
struct TextureHandle;

class CTextureManager
{
public:
	CTextureManager() = default;
	~CTextureManager();

	bool Initialize(CD3D12Renderer* pRenderer);

	TextureHandle* CreateTextureFromFile(const WCHAR* filePath);
	TextureHandle* CreateDynamicTexture(UINT texWidth, UINT texHeight);
	TextureHandle* CreateStaticTexture(UINT texWidth, UINT texHeight, DXGI_FORMAT format, const BYTE* pInitImage);

	void UpdateTextureWithImage(TextureHandle* pTexHandle, const BYTE* pSrcBits, UINT srcWidth, UINT srcHeight);
	void DeleteTexture(TextureHandle* pTexHandle);

private:
	TextureHandle* AllocTextureHandle();
	DWORD FreeTextureHandle(TextureHandle* pTexHandle);
	bool CreateSrvForTexture(TextureHandle* pTexHandle, ID3D12Resource* pTexResource, DXGI_FORMAT format, UINT mipLevels);
	void Cleanup();

private:
	CD3D12Renderer* m_pRenderer = nullptr;
	ID3D12Device5* m_pD3DDevice = nullptr;
	CD3D12ResourceManager* m_pResourceManager = nullptr;
	CCpuDescriptorFreeListAllocator* m_pDescriptorAllocator = nullptr;

	std::unordered_map<std::wstring, TextureHandle*> m_fileTextureMap;
};
