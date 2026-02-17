#pragma once

#include <DirectXMath.h>

using namespace DirectX;

struct VertexPos3Color4
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
};

struct VertexPos3Color4Tex2
{
	XMFLOAT3 Position;
	XMFLOAT4 Color;
	XMFLOAT2 TexCoord;
};

union URGBA
{
	struct
	{
		BYTE R;
		BYTE G;
		BYTE B;
		BYTE A;
	};
	BYTE ColorFactor[4];
};

struct ConstantBufferDefault
{
	XMMATRIX WorldMatrix;
	XMMATRIX ViewMatrix;
	XMMATRIX ProjectionMatrix;
};

struct TextureHandle
{
	ID3D12Resource* TextureResource;
	D3D12_CPU_DESCRIPTOR_HANDLE SrvDescriptorHandle;
};

struct IndexedTriGroup
{
	ComPtr<ID3D12Resource> IndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	UINT TriangleCount = 0;
	TextureHandle* pTexHandle = nullptr;
};