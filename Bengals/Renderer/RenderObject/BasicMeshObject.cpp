#include "pch.h"
#include "BasicMeshObject.h"
#include "../D3D12Renderer.h"
#include <d3dcompiler.h>
#include <d3dx12.h>
#include "../../Types/typedef.h"
#include "../../../Util/D3DUtil.h"
#include "../RenderHelper/GpuDescriptorLinearAllocator.h"
#include "../RenderHelper/ConstantBufferPool.h"
#include "../Manager/CD3D12ResourceManager.h"

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

	// RootParam 0: per-object CBV (b0)
	CD3DX12_DESCRIPTOR_RANGE rangesPerObj[1] = {};
	rangesPerObj[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	// RootParam 1: per-tri-group SRV (t0)
	CD3DX12_DESCRIPTOR_RANGE rangesPerTriGroup[1] = {};
	rangesPerTriGroup[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangesPerObj), rangesPerObj, D3D12_SHADER_VISIBILITY_ALL);
	rootParameters[1].InitAsDescriptorTable(_countof(rangesPerTriGroup), rangesPerTriGroup, D3D12_SHADER_VISIBILITY_ALL);

	CD3DX12_STATIC_SAMPLER_DESC sampler{0};
	//SetDefaultSamplerDesc(&sampler, 0);
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

bool CBasicMeshObject::BeginCreateMesh(const void* pVertexList, UINT vertexCount, UINT vertexSize, UINT triGroupCount)
{
	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	if (triGroupCount > MaxTriGroupCountPerObj)
	{
		__debugbreak();
		return false;
	}

	HRESULT hr = pResourceManager->CreateVertexBuffer(vertexSize, vertexCount, &m_vertexBufferView, m_vertexBuffer.ReleaseAndGetAddressOf(), pVertexList);
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}

	m_triGroupList = std::make_unique<IndexedTriGroup[]>(triGroupCount);
	m_maxTriGroupCount = triGroupCount;
	m_triGroupCount = 0;

	return true;
}

bool CBasicMeshObject::InsertIndexedTriList(const WORD* pIndexList, UINT triCount, const WCHAR* texFileName)
{
	if (m_triGroupCount >= m_maxTriGroupCount)
	{
		__debugbreak();
		return false;
	}

	CD3D12ResourceManager* pResourceManager = m_pRenderer->GetResourceManager();

	IndexedTriGroup& triGroup = m_triGroupList[m_triGroupCount];

	UINT indexCount = triCount * 3;
	HRESULT hr = pResourceManager->CreateIndexBuffer(indexCount, &triGroup.IndexBufferView, triGroup.IndexBuffer.ReleaseAndGetAddressOf(), pIndexList);
	if (FAILED(hr))
	{
		__debugbreak();
		return false;
	}
	triGroup.TriangleCount = triCount;

	triGroup.pTexHandle = (TextureHandle*)m_pRenderer->CreateTextureFromFile(texFileName);
	if (triGroup.pTexHandle == nullptr)
	{
		__debugbreak();
		return false;
	}

	m_triGroupCount++;
	return true;
}

void CBasicMeshObject::EndCreateMesh()
{
	
}

void CBasicMeshObject::Draw(ID3D12GraphicsCommandList* pCommandList, const XMMATRIX& worldMatrix)
{
	if (pCommandList == nullptr || m_pRenderer == nullptr || m_triGroupCount == 0)
	{
		__debugbreak();
		return;
	}

	ID3D12Device5* pD3DDevice = m_pRenderer->GetD3DDevice();
	CConstantBufferPool* pConstantBufferPool = m_pRenderer->GetConstantBufferPool(ConstantBufferTypeDefault);
	CGpuDescriptorLinearAllocator* pDescriptorAllocator = m_pRenderer->GetDescriptorPool();

	ID3D12DescriptorHeap* pDescriptorHeap = pDescriptorAllocator->GetDescriptorHeap();
	const UINT descriptorSize = pDescriptorAllocator->GetDescriptorSizeCbvSrvUav();

	// 1. descriptor table: CBV 1 + SRV N
	UINT requiredDescriptorCount = DescriptorCountPerObj + (m_triGroupCount * DescriptorCountPerTriGroup);

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuBaseDescriptorHandle = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuBaseDescriptorHandle = {};

	if (!pDescriptorAllocator->Allocate(&cpuBaseDescriptorHandle, &gpuBaseDescriptorHandle, requiredDescriptorCount))
	{
		__debugbreak();
		return;
	}

	// 2. constant buffer
	ConstantBufferContainer* pCB = pConstantBufferPool->Allocate();
	if (!pCB)
	{
		__debugbreak();
		return;
	}

	ConstantBufferDefault* pConstantBufferDefault = reinterpret_cast<ConstantBufferDefault*>(pCB->SystemAddress);

	XMMATRIX viewMatrix = {};
	XMMATRIX projectionMatrix = {};
	m_pRenderer->GetViewProjMatrix(&viewMatrix, &projectionMatrix);
	pConstantBufferDefault->WorldMatrix = XMMatrixTranspose(worldMatrix);
	pConstantBufferDefault->ViewMatrix = XMMatrixTranspose(viewMatrix);
	pConstantBufferDefault->ProjectionMatrix = XMMatrixTranspose(projectionMatrix);

	CD3DX12_CPU_DESCRIPTOR_HANDLE destHandle(cpuBaseDescriptorHandle, 0, descriptorSize);
	pD3DDevice->CopyDescriptorsSimple(1, destHandle, pCB->CbvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	destHandle.Offset(1, descriptorSize);
	for (UINT i = 0; i < m_triGroupCount; i++)
	{
		pD3DDevice->CopyDescriptorsSimple(1, destHandle, m_triGroupList[i].pTexHandle->SrvDescriptorHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		destHandle.Offset(1, descriptorSize);
	}

	pCommandList->SetGraphicsRootSignature(m_pRootSignature);
	pCommandList->SetDescriptorHeaps(1, &pDescriptorHeap);
	pCommandList->SetPipelineState(m_pPipelineStateObject);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

	pCommandList->SetGraphicsRootDescriptorTable(0, gpuBaseDescriptorHandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle(gpuBaseDescriptorHandle, DescriptorCountPerObj, descriptorSize);
	for (UINT i = 0; i < m_triGroupCount; i++)
	{
		pCommandList->SetGraphicsRootDescriptorTable(1, gpuSrvHandle);
		gpuSrvHandle.Offset(1, descriptorSize);

		IndexedTriGroup& triGroup = m_triGroupList[i];
		pCommandList->IASetIndexBuffer(&triGroup.IndexBufferView);
		pCommandList->DrawIndexedInstanced(triGroup.TriangleCount * 3, 1, 0, 0, 0);
	}
}

void CBasicMeshObject::Clean()
{
	if (m_pRenderer)
	{
		CleanTriGroups();
		CleanSharedResource();
		m_pRenderer = nullptr;
	}
}

void CBasicMeshObject::CleanTriGroups()
{
	if (m_triGroupList)
	{
		for (UINT i = 0; i < m_triGroupCount; i++)
		{
			IndexedTriGroup& triGroup = m_triGroupList[i];
			if (triGroup.pTexHandle)
			{
				m_pRenderer->DeleteTexture(triGroup.pTexHandle);
				triGroup.pTexHandle = nullptr;
			}
		}
		m_triGroupList.reset();
		m_triGroupCount = 0;
		m_maxTriGroupCount = 0;
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
