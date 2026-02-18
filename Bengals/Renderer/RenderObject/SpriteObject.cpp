#include "pch.h"
#include "SpriteObject.h"
#include "../D3D12Renderer.h"
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "../../Types/typedef.h"
#include "../../../Util/D3DUtil.h"
#include "../RenderHelper/GpuDescriptorLinearAllocator.h"
#include "../RenderHelper/ConstantBufferPool.h"
#include "../Manager/CD3D12ResourceManager.h"

ID3D12RootSignature* CSpriteObject::m_pRootSignature = nullptr;
ID3D12PipelineState* CSpriteObject::m_pPipelineStateObject = nullptr;
ID3D12Resource* CSpriteObject::m_pVertexBuffer = nullptr;
D3D12_VERTEX_BUFFER_VIEW CSpriteObject::m_vertexBufferView = {};
ID3D12Resource* CSpriteObject::m_pIndexBuffer = nullptr;
D3D12_INDEX_BUFFER_VIEW CSpriteObject::m_indexBufferView = {};
UINT CSpriteObject::m_initRefCount = 0;

CSpriteObject::CSpriteObject(CD3D12Renderer* pRenderer)
{
	if (!Initialize(pRenderer))
	{
		__debugbreak();
	}
}

CSpriteObject::CSpriteObject(CD3D12Renderer* pRenderer, const WCHAR* wchTexFileName, const RECT* pRect)
{
	if (!Initialize(pRenderer, wchTexFileName, pRect))
	{
		__debugbreak();
	}
}

CSpriteObject::~CSpriteObject()
{
	Clean();
}

bool CSpriteObject::Initialize(CD3D12Renderer* pRenderer)
{
	if (!pRenderer)
	{
		__debugbreak();
		return false;
	}

	m_pRenderer = pRenderer;
	m_scale = { 1.0f, 1.0f };
	m_rect = {};

	if (m_initRefCount <= 0)
	{
		if (!InitRootSignature())
		{
			return false;
		}
		if (!InitPipelineState())
		{
			return false;
		}
		if (!InitSharedBuffers())
		{
			return false;
		}
	}

	m_initRefCount++;
	return true;
}

bool CSpriteObject::Initialize(CD3D12Renderer* pRenderer, const WCHAR* wchTexFileName, const RECT* pRect)
{
	if (!Initialize(pRenderer))
	{
		return false;
	}

	if (!wchTexFileName)
	{
		return true;
	}

	m_pTexHandle = (TextureHandle*)m_pRenderer->CreateTextureFromFile(wchTexFileName);
	if (!m_pTexHandle || !m_pTexHandle->TextureResource)
	{
		__debugbreak();
		return false;
	}

	D3D12_RESOURCE_DESC texDesc = m_pTexHandle->TextureResource->GetDesc();
	RECT fullRect = { 0, 0, (LONG)texDesc.Width, (LONG)texDesc.Height };
	auto ClampLong = [](LONG value, LONG minValue, LONG maxValue) -> LONG
	{
		if (value < minValue)
		{
			return minValue;
		}
		if (value > maxValue)
		{
			return maxValue;
		}
		return value;
	};

	m_rect = fullRect;
	if (pRect && pRect->right > pRect->left && pRect->bottom > pRect->top)
	{
		m_rect = *pRect;
	}

	m_rect.left = ClampLong(m_rect.left, 0, (LONG)texDesc.Width);
	m_rect.top = ClampLong(m_rect.top, 0, (LONG)texDesc.Height);
	m_rect.right = ClampLong(m_rect.right, m_rect.left + 1, (LONG)texDesc.Width);
	m_rect.bottom = ClampLong(m_rect.bottom, m_rect.top + 1, (LONG)texDesc.Height);

	return true;
}

bool CSpriteObject::InitRootSignature()
{
	if (!m_pRenderer)
	{
		return false;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (!pD3DDevice)
	{
		return false;
	}

	ComPtr<ID3DBlob> pSignature = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;

	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_ALL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	SetDefaultSamplerDesc(&sampler, 0);
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignature, &pError)))
	{
		__debugbreak();
		return false;
	}

	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
	{
		__debugbreak();
		return false;
	}

	return true;
}

bool CSpriteObject::InitPipelineState()
{
	if (!m_pRenderer)
	{
		return false;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (!pD3DDevice)
	{
		return false;
	}

	ComPtr<ID3DBlob> vertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> pixelShaderBlob = nullptr;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	if (FAILED(D3DCompileFromFile(L"Renderer/Shaders/SpriteShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, vertexShaderBlob.ReleaseAndGetAddressOf(), nullptr)))
	{
		__debugbreak();
		return false;
	}
	if (FAILED(D3DCompileFromFile(L"Renderer/Shaders/SpriteShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pixelShaderBlob.ReleaseAndGetAddressOf(), nullptr)))
	{
		__debugbreak();
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC{ D3D12_DEFAULT };
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPipelineStateObject))))
	{
		__debugbreak();
		return false;
	}

	return true;
}

bool CSpriteObject::InitSharedBuffers()
{
	if (!m_pRenderer)
	{
		return false;
	}

	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();
	if (!pResourceManager)
	{
		return false;
	}

	VertexPos3Color4Tex2 vertices[] =
	{
		{ { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
	};
	WORD indices[] = { 0, 1, 2, 0, 2, 3 };

	if (FAILED(pResourceManager->CreateVertexBuffer(
		static_cast<UINT>(sizeof(VertexPos3Color4Tex2)),
		static_cast<UINT>(_countof(vertices)),
		&m_vertexBufferView,
		&m_pVertexBuffer,
		vertices)))
	{
		__debugbreak();
		return false;
	}

	if (FAILED(pResourceManager->CreateIndexBuffer(
		static_cast<UINT>(_countof(indices)),
		&m_indexBufferView,
		&m_pIndexBuffer,
		indices)))
	{
		__debugbreak();
		return false;
	}

	return true;
}

void CSpriteObject::DrawWithTex(ID3D12GraphicsCommandList* pCommandList, XMFLOAT2 pos, XMFLOAT2 pixelSize, const RECT* pRect, float z, TextureHandle* pTexHandle)
{
	if (!pCommandList || !m_pRenderer || !pTexHandle || !pTexHandle->TextureResource)
	{
		return;
	}

	CGpuDescriptorLinearAllocator* pDescriptorAllocator = m_pRenderer->GetDescriptorPool();
	CConstantBufferPool* pConstantBufferPool = m_pRenderer->GetConstantBufferPool(ConstantBufferTypeSprite);
	if (!pDescriptorAllocator || !pConstantBufferPool)
	{
		return;
	}

	ConstantBufferContainer* pCB = pConstantBufferPool->Allocate();
	if (!pCB)
	{
		__debugbreak();
		return;
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBaseDescriptorHandle = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBaseDescriptorHandle = {};
	if (!pDescriptorAllocator->Allocate(&cpuBaseDescriptorHandle, &gpuBaseDescriptorHandle, DescriptorCountForDraw))
	{
		__debugbreak();
		return;
	}

	D3D12_RESOURCE_DESC texDesc = pTexHandle->TextureResource->GetDesc();
	const LONG texWidth = (LONG)texDesc.Width;
	const LONG texHeight = (LONG)texDesc.Height;
	auto ClampLong = [](LONG value, LONG minValue, LONG maxValue) -> LONG
	{
		if (value < minValue)
		{
			return minValue;
		}
		if (value > maxValue)
		{
			return maxValue;
		}
		return value;
	};

	RECT rect = { 0, 0, texWidth, texHeight };
	if (pRect && pRect->right > pRect->left && pRect->bottom > pRect->top)
	{
		rect = *pRect;
	}

	rect.left = ClampLong(rect.left, 0, texWidth);
	rect.top = ClampLong(rect.top, 0, texHeight);
	rect.right = ClampLong(rect.right, rect.left + 1, texWidth);
	rect.bottom = ClampLong(rect.bottom, rect.top + 1, texHeight);

	const float fTexWidth = static_cast<float>(texWidth);
	const float fTexHeight = static_cast<float>(texHeight);
	const float fSampleWidth = static_cast<float>(rect.right - rect.left);
	const float fSampleHeight = static_cast<float>(rect.bottom - rect.top);

	if (pixelSize.x <= 0.0f)
	{
		pixelSize.x = fSampleWidth;
	}
	if (pixelSize.y <= 0.0f)
	{
		pixelSize.y = fSampleHeight;
	}

	ConstantBufferSprite* pConstantBufferSprite = reinterpret_cast<ConstantBufferSprite*>(pCB->SystemAddress);
	pConstantBufferSprite->ScreenRes = XMFLOAT2(
		static_cast<float>(m_pRenderer->GetScreenWidth()),
		static_cast<float>(m_pRenderer->GetScreenHeight()));
	pConstantBufferSprite->Pos = pos;
	pConstantBufferSprite->Scale = XMFLOAT2(
		pixelSize.x / fTexWidth,
		pixelSize.y / fTexHeight);
	pConstantBufferSprite->TexSize = XMFLOAT2(fTexWidth, fTexHeight);
	pConstantBufferSprite->TexSamplePos = XMFLOAT2(static_cast<float>(rect.left), static_cast<float>(rect.top));
	pConstantBufferSprite->TexSampleSize = XMFLOAT2(fSampleWidth, fSampleHeight);
	pConstantBufferSprite->Z = z;
	pConstantBufferSprite->Alpha = 1.0f;
	pConstantBufferSprite->Reserved0 = 0.0f;
	pConstantBufferSprite->Reserved1 = 0.0f;

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorAllocator->GetDescriptorHeap();
	const UINT descriptorSize = pDescriptorAllocator->GetDescriptorSizeCbvSrvUav();

	CD3DX12_CPU_DESCRIPTOR_HANDLE destHandle(cpuBaseDescriptorHandle, SpriteDescriptorIndexCbv, descriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, destHandle, pCB->CbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	destHandle.Offset(1, descriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, destHandle, pTexHandle->SrvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	pCommandList->SetPipelineState(m_pPipelineStateObject);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	pCommandList->IASetIndexBuffer(&m_indexBufferView);
	pCommandList->SetGraphicsRootDescriptorTable(0, gpuBaseDescriptorHandle);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void CSpriteObject::Draw(
	ID3D12GraphicsCommandList* pCommandList,
	XMFLOAT2 pos,
	XMFLOAT2 pixelSize,
	float z)
{
	if (!m_pTexHandle)
	{
		return;
	}

	const float sampleWidth = static_cast<float>(m_rect.right - m_rect.left);
	const float sampleHeight = static_cast<float>(m_rect.bottom - m_rect.top);
	XMFLOAT2 drawPixelSize = { sampleWidth * m_scale.x, sampleHeight * m_scale.y };
	if (pixelSize.x > 0.0f)
	{
		drawPixelSize.x = pixelSize.x;
	}
	if (pixelSize.y > 0.0f)
	{
		drawPixelSize.y = pixelSize.y;
	}

	DrawWithTex(pCommandList, pos, drawPixelSize, &m_rect, z, m_pTexHandle);
}

void CSpriteObject::Clean()
{
	if (m_pRenderer)
	{
		CleanTexture();
		CleanSharedResource();
		m_pRenderer = nullptr;
	}
}

void CSpriteObject::CleanTexture()
{
	if (m_pTexHandle && m_pRenderer)
	{
		m_pRenderer->DeleteTexture(m_pTexHandle);
		m_pTexHandle = nullptr;
	}
}

void CSpriteObject::CleanSharedResource()
{
	if (m_initRefCount <= 0)
	{
		return;
	}

	const UINT refCount = --m_initRefCount;
	if (refCount <= 0)
	{
		if (m_pRootSignature)
		{
			m_pRootSignature->Release();
			m_pRootSignature = nullptr;
		}

		if (m_pPipelineStateObject)
		{
			m_pPipelineStateObject->Release();
			m_pPipelineStateObject = nullptr;
		}

		if (m_pVertexBuffer)
		{
			m_pVertexBuffer->Release();
			m_pVertexBuffer = nullptr;
			m_vertexBufferView = {};
		}

		if (m_pIndexBuffer)
		{
			m_pIndexBuffer->Release();
			m_pIndexBuffer = nullptr;
			m_indexBufferView = {};
		}
	}
}
