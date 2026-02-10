#include "pch.h"
#include "BasicMeshObject.h"
#include "../D3D12Renderer.h"
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "../../Types/typedef.h"
#include "../../../Util/D3DUtil.h"
#include "../RenderHelper/ShaderVisibleDescriptorTableAllocator.h"
#include "../RenderHelper/ConstantBufferPool.h"
#include "../ResourceManager/CD3D12ResourceManager.h"

ID3D12RootSignature* CBasicMeshObject::m_pRootSignature = nullptr;
ID3D12PipelineState* CBasicMeshObject::m_pPipelineStateObject = nullptr;
UINT CBasicMeshObject::m_initRefCount = 0;

CBasicMeshObject::CBasicMeshObject(CD3D12Renderer* pRenderer)
{
	Initialize(pRenderer);
}

CBasicMeshObject::~CBasicMeshObject()
{
	Clean();
}

bool CBasicMeshObject::Initialize(CD3D12Renderer* pRenderer)
{
	m_pRenderer = pRenderer;

	if (m_pRenderer)
	{
		if (m_initRefCount <= 0)
		{
			const bool bInitRootSignature = InitRootSignature();
			const bool bInitPipelineState = InitPipelineState();
			UNREFERENCED_PARAMETER(bInitRootSignature);
			UNREFERENCED_PARAMETER(bInitPipelineState);
		}

		m_initRefCount++;
	}

	return m_initRefCount > 0;
}

bool CBasicMeshObject::InitRootSignature()
{
	bool bResult = false;

	if (m_pRenderer == nullptr)
	{
		return bResult;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();

	ComPtr<ID3DBlob> pSignature = nullptr;
	ComPtr<ID3DBlob> pError = nullptr;

	CD3DX12_DESCRIPTOR_RANGE ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // b0
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

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
	}

	if (FAILED(pD3DDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&m_pRootSignature))))
	{
		__debugbreak();
	}

	return bResult = true;
}

bool CBasicMeshObject::InitPipelineState()
{
	bool bResult = false;
	if (m_pRenderer == nullptr)
	{
		__debugbreak();
		return bResult;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	if (pD3DDevice == nullptr)
	{
		return bResult;
	}

	ComPtr<ID3DBlob> vertexShaderBlob = nullptr;
	ComPtr<ID3DBlob> pixelShaderBlob = nullptr;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif
	if (FAILED(D3DCompileFromFile(L"Renderer/Shaders/DefaultShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, vertexShaderBlob.ReleaseAndGetAddressOf(), nullptr)))
	{
		__debugbreak();
		return bResult;
	}
	if (FAILED(D3DCompileFromFile(L"Renderer/Shaders/DefaultShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, pixelShaderBlob.ReleaseAndGetAddressOf(), nullptr)))
	{
		__debugbreak();
		return bResult;
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
		return bResult;
	}

	return bResult = true;
}

bool CBasicMeshObject::CreateMesh()
{
	bool bResult = false;
	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CD3D12ResourceManager* pD3D12ResourceManager = m_pRenderer->GetResourceManager();

	if (pD3DDevice == nullptr || pD3D12ResourceManager == nullptr)
	{
		__debugbreak();
		return false;
	}

	VertexPos3Color4Tex2 vertices[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 0.f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 0.f, 1.0f, 1.0f }, { 1.f, 1.f } },
		{ { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.f, 1.f } },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.f, 0.5f } }
	};

	WORD indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	HRESULT hr = pD3D12ResourceManager->CreateVertexBuffer(sizeof(VertexPos3Color4Tex2), _countof(vertices), &m_vertexBufferView, &m_pVertexBuffer, vertices);
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	hr = pD3D12ResourceManager->CreateIndexBuffer(_countof(indices), &m_indexBufferView, &m_pIndexBuffer, indices);
	if (FAILED(hr))
	{
		__debugbreak();
		return bResult;
	}

	return bResult = SUCCEEDED(hr);
}

void CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMMATRIX& worldMatrix, D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle)
{
	if (pCommandList == nullptr)
	{
		__debugbreak();
		return;
	}

	if (m_pRenderer == nullptr)
	{
		__debugbreak();
		return;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CConstantBufferPool* pConstantBufferPool = m_pRenderer->GetConstantBufferPool();
	CShaderVisibleDescriptorTableAllocator* pDescriptorPool = m_pRenderer->GetDescriptorPool();
	assert(pConstantBufferPool);
	assert(pDescriptorPool);

	ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorPool->GetDescriptorHeap();
	const UINT descriptorSizeCbvSrvUav = pDescriptorPool->GetDescriptorSizeCbvSrvUav();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};

	const bool bAllocateDescriptorTable = pDescriptorPool->Allocate(&cpuDescriptorTable, &gpuDescriptorTable, DescriptorCountPerDraw);
	if (bAllocateDescriptorTable == false)
	{
		__debugbreak();
		return;
	}

	ConstantBufferContainer* pConstantBufferContainer = pConstantBufferPool->Allocate();
	if (!pConstantBufferContainer)
	{
		__debugbreak();
		return;
	}

	ConstantBufferDefault* pConstantBufferDefault = reinterpret_cast<ConstantBufferDefault*>(pConstantBufferContainer->SystemAddress);

	XMMATRIX viewMatrix = {};
	XMMATRIX projectionMatrix = {};
	m_pRenderer->GetViewProjMatrix(&viewMatrix, &projectionMatrix);
	pConstantBufferDefault->ViewMatrix = XMMatrixTranspose(viewMatrix);
	pConstantBufferDefault->ProjectionMatrix = XMMatrixTranspose(projectionMatrix);
	pConstantBufferDefault->WorldMatrix = XMMatrixTranspose(worldMatrix);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDest(cpuDescriptorTable, BasicMeshDescriptorIndexCbv, descriptorSizeCbvSrvUav);
	pD3DDevice->CopyDescriptorsSimple(1, cbvDest, pConstantBufferContainer->CbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (srvDescriptorHandle.ptr)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvDest(cpuDescriptorTable, BasicMeshDescriptorIndexSrv, descriptorSizeCbvSrvUav);
		pD3DDevice->CopyDescriptorsSimple(1, srvDest, srvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	pCommandList->SetGraphicsRootDescriptorTable(0, gpuDescriptorTable);

	pCommandList->SetPipelineState(m_pPipelineStateObject);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	pCommandList->IASetIndexBuffer(&m_indexBufferView);
	pCommandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void CBasicMeshObject::Clean()
{
	if (m_pVertexBuffer)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	}

	if (m_pIndexBuffer)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	}

	if (m_pRenderer)
	{
		CleanSharedResource();
		m_pRenderer = nullptr;
	}
}

void CBasicMeshObject::CleanSharedResource()
{
	if (m_initRefCount <= 0 || m_pRenderer == nullptr)
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
	}
}
