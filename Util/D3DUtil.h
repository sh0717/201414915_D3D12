#pragma once

void DebugLog(const char* fmt, ...);
void DebugLogW(const wchar_t* fmt, ...);



void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
void GetSoftwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
void SetDebugLayerInfo(ID3D12Device* InD3DDevice);
void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* pOutSamplerDesc, UINT RegisterIndex);

void UpdateTexture(ID3D12Device* pD3DDevice, ID3D12GraphicsCommandList* pCommandList, ID3D12Resource* pDestTexResource, ID3D12Resource* pSrcTexResource);

class CD3D12Renderer;
void* CreateBoxMeshObject(CD3D12Renderer* pRenderer);
void* CreateQuadMeshObject(CD3D12Renderer* pRenderer);

inline size_t AlignConstantBufferSize(size_t size)
{
	size_t aligned_size = (size + 255) & (~255);
	return aligned_size;
}
