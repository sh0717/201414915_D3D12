#pragma once

#include <DirectXMath.h>
#include <string>

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

union RGBA
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

struct ConstantBufferSprite
{
	XMFLOAT2 ScreenRes;
	XMFLOAT2 Pos;
	XMFLOAT2 Scale;
	XMFLOAT2 TexSize;
	XMFLOAT2 TexSamplePos;
	XMFLOAT2 TexSampleSize;
	float Z;
	float Alpha;
	float Reserved0;
	float Reserved1;
};

enum class EConstantBufferType
{
	Default = 0,
	Sprite,
	Count
};

struct ConstantBufferProperty
{
	EConstantBufferType Type;
	UINT Size;
};

struct TextureHandle
{
	ID3D12Resource* TextureResource = nullptr;
	ID3D12Resource* pUploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE SrvDescriptorHandle = {};
	bool bUpdated = false;
	DWORD RefCount = 0;
	bool bFromFile = false;
	std::wstring FilePath;
};

struct IndexedTriGroup
{
	ComPtr<ID3D12Resource> IndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};
	UINT TriangleCount = 0;
	TextureHandle* pTexHandle = nullptr;
};
