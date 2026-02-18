#include "pch.h"
#include <DirectXMath.h>
#include "Renderer/D3D12Renderer.h"
#include "Game.h"
#include "GameObject.h"
#include "Types/typedef.h"

CGameObject::CGameObject()
{
	m_scaleMatrix = XMMatrixIdentity();
	m_rotationMatrix = XMMatrixIdentity();
	m_translationMatrix = XMMatrixIdentity();
	m_worldMatrix = XMMatrixIdentity();
}

CGameObject::~CGameObject()
{
	Cleanup();
}

bool CGameObject::Initialize(CGame* pGame, EMeshType meshType)
{
	m_pGame = pGame;
	m_pRenderer = pGame->GetRenderer();

	switch (meshType)
	{
	case EMeshType::Box:
		m_pMeshObj = CreateBoxMesh();
		break;
	case EMeshType::Quad:
		m_pMeshObj = CreateQuadMesh();
		break;
	}

	return m_pMeshObj != nullptr;
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_pos = XMVectorSet(x, y, z, 1.0f);
	m_translationMatrix = XMMatrixTranslation(x, y, z);
	m_bUpdateTransform = true;
}

void CGameObject::SetScale(float x, float y, float z)
{
	m_scale = XMVectorSet(x, y, z, 1.0f);
	m_scaleMatrix = XMMatrixScaling(x, y, z);
	m_bUpdateTransform = true;
}

void CGameObject::SetRotationX(float rotX)
{
	m_rotX = rotX;
	m_bUpdateTransform = true;
}

void CGameObject::SetRotationY(float rotY)
{
	m_rotY = rotY;
	m_bUpdateTransform = true;
}

void CGameObject::Run()
{
	if (m_bUpdateTransform)
	{
		UpdateTransform();
		m_bUpdateTransform = false;
	}
}

void CGameObject::Render()
{
	if (m_pMeshObj)
	{
		m_pRenderer->RenderMeshObject(m_pMeshObj, m_worldMatrix);
	}
}

void* CGameObject::CreateBoxMesh()
{
	static constexpr float HalfBoxLen = 0.25f;
	const XMFLOAT3 worldPosList[8] =
	{
		{ -HalfBoxLen, HalfBoxLen, HalfBoxLen },
		{ -HalfBoxLen, -HalfBoxLen, HalfBoxLen },
		{ HalfBoxLen, -HalfBoxLen, HalfBoxLen },
		{ HalfBoxLen, HalfBoxLen, HalfBoxLen },
		{ -HalfBoxLen, HalfBoxLen, -HalfBoxLen },
		{ -HalfBoxLen, -HalfBoxLen, -HalfBoxLen },
		{ HalfBoxLen, -HalfBoxLen, -HalfBoxLen },
		{ HalfBoxLen, HalfBoxLen, -HalfBoxLen }
	};

	const WORD worldPosIndexList[36] =
	{
		// +z
		3, 0, 1, 3, 1, 2,
		// -z
		4, 7, 6, 4, 6, 5,
		// -x
		0, 4, 5, 0, 5, 1,
		// +x
		7, 3, 2, 7, 2, 6,
		// +y
		0, 3, 7, 0, 7, 4,
		// -y
		2, 1, 5, 2, 5, 6
	};

	const XMFLOAT2 texCoordList[36] =
	{
		// +z
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
		// -z
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
		// -x
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
		// +x
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
		// +y
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f },
		// -y
		{ 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f },
		{ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }
	};

	VertexPos3Color4Tex2 vertexList[_countof(worldPosIndexList)] = {};
	WORD indexList[_countof(worldPosIndexList)] = {};
	for (UINT i = 0; i < _countof(worldPosIndexList); i++)
	{
		const WORD worldPosIndex = worldPosIndexList[i];
		vertexList[i].Position = worldPosList[worldPosIndex];
		vertexList[i].Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		vertexList[i].TexCoord = texCoordList[i];
		indexList[i] = static_cast<WORD>(i);
	}

	const WCHAR* textureFileNameList[6] =
	{
		L"../Resources/Image/tex_00.dds",
		L"../Resources/Image/tex_01.dds",
		L"../Resources/Image/tex_02.dds",
		L"../Resources/Image/tex_03.dds",
		L"../Resources/Image/tex_04.dds",
		L"../Resources/Image/tex_05.dds"
	};

	void* pMeshObject = m_pRenderer->CreateBasicMeshObject();
	if (!pMeshObject)
	{
		return nullptr;
	}

	bool bResult = m_pRenderer->BeginCreateMesh(
		pMeshObject,
		vertexList,
		_countof(vertexList),
		sizeof(VertexPos3Color4Tex2),
		6);

	if (bResult)
	{
		for (UINT i = 0; i < 6; i++)
		{
			bResult = m_pRenderer->InsertTriGroup(pMeshObject, indexList + i * 6, 2, textureFileNameList[i]);
			if (!bResult)
			{
				break;
			}
		}
	}

	if (!bResult)
	{
		m_pRenderer->DeleteBasicMeshObject(pMeshObject);
		return nullptr;
	}

	m_pRenderer->EndCreateMesh(pMeshObject);
	return pMeshObject;
}

void* CGameObject::CreateQuadMesh()
{
	VertexPos3Color4Tex2 vertexList[] =
	{
		{ { -0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 0.25f, 0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } }
	};

	WORD indexList[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	void* pMeshObject = m_pRenderer->CreateBasicMeshObject();
	if (!pMeshObject)
	{
		return nullptr;
	}

	bool bResult = m_pRenderer->BeginCreateMesh(
		pMeshObject,
		vertexList,
		_countof(vertexList),
		sizeof(VertexPos3Color4Tex2),
		1);

	if (bResult)
	{
		bResult = m_pRenderer->InsertTriGroup(pMeshObject, indexList, 2, L"../Resources/Image/tex_06.dds");
	}

	if (!bResult)
	{
		m_pRenderer->DeleteBasicMeshObject(pMeshObject);
		return nullptr;
	}

	m_pRenderer->EndCreateMesh(pMeshObject);
	return pMeshObject;
}

void CGameObject::UpdateTransform()
{
	XMMATRIX rotX = XMMatrixRotationX(m_rotX);
	XMMATRIX rotY = XMMatrixRotationY(m_rotY);
	m_rotationMatrix = XMMatrixMultiply(rotX, rotY);

	// World = Scale * Rotation * Translation
	m_worldMatrix = XMMatrixMultiply(m_scaleMatrix, m_rotationMatrix);
	m_worldMatrix = XMMatrixMultiply(m_worldMatrix, m_translationMatrix);
}

void CGameObject::Cleanup()
{
	if (m_pMeshObj)
	{
		m_pRenderer->DeleteBasicMeshObject(m_pMeshObj);
		m_pMeshObj = nullptr;
	}
}
