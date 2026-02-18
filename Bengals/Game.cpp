#include "pch.h"
#include <algorithm>
#include <DirectXMath.h>
#include "Renderer/D3D12Renderer.h"
#include "GameObject.h"
#include "Game.h"

CGame::CGame()
{

}

CGame::~CGame()
{
	Cleanup();
}

bool CGame::Initialize(HWND hWnd, bool bEnableDebugLayer, bool bEnableGBV)
{
	m_windowHandle = hWnd;
	m_renderer = std::make_unique<CD3D12Renderer>(hWnd, bEnableDebugLayer, bEnableGBV);
	if (!m_renderer)
	{
		__debugbreak();
		return false;
	}

	// 100 box objects at random positions
	const UINT GameObjCount = 100;
	for (UINT i = 0; i < GameObjCount; i++)
	{
		CGameObject* pGameObj = CreateGameObject(EMeshType::Box);
		if (pGameObj)
		{
			float x = (float)((rand() % 21) - 10);
			float y = 0.0f;
			float z = (float)((rand() % 21) - 10);
			pGameObj->SetPosition(x, y, z);
			float rad = (rand() % 181) * (3.1415f / 180.0f);
			pGameObj->SetRotationY(rad);
		}
	}

	m_renderer->SetCameraPos(0.0f, 0.0f, -10.0f);

	// Dynamic texture
	m_dynamicImageWidth = 512;
	m_dynamicImageHeight = 256;
	m_pDynamicImage = (BYTE*)malloc(m_dynamicImageWidth * m_dynamicImageHeight * 4);
	UINT* pDest = (UINT*)m_pDynamicImage;
	for (UINT y = 0; y < m_dynamicImageHeight; y++)
	{
		for (UINT x = 0; x < m_dynamicImageWidth; x++)
		{
			pDest[x + m_dynamicImageWidth * y] = 0xff0000ff;
		}
	}
	m_pDynamicTexHandle = m_renderer->CreateDynamicTexture(m_dynamicImageWidth, m_dynamicImageHeight);

	// Sprite objects
	m_pSpriteObject0 = m_renderer->CreateSpriteObject(
		L"../Resources/Image/sprite_1024x1024.dds",
		512, 512, 512, 512);
	m_pSpriteObjCommon = m_renderer->CreateSpriteObject();

	return true;
}

void CGame::Run()
{
	m_frameCount++;

	ULONGLONG curTick = GetTickCount64();
	Update(curTick);
	Render();

	if (curTick - m_previousFrameCheckTick > 1000)
	{
		m_previousFrameCheckTick = curTick;

		WCHAR wchTxt[64];
		swprintf_s(wchTxt, L"FPS:%u", m_frameCount);
		SetWindowText(m_windowHandle, wchTxt);

		m_frameCount = 0;
	}
}

bool CGame::Update(ULONGLONG curTick)
{
	if (curTick - m_previousUpdateTick < 16)
	{
		return false;
	}
	m_previousUpdateTick = curTick;

	// Camera movement
	if (m_camOffsetX != 0.0f || m_camOffsetY != 0.0f || m_camOffsetZ != 0.0f)
	{
		m_renderer->MoveCamera(m_camOffsetX, m_camOffsetY, m_camOffsetZ);
	}

	// Update all game objects (dirty flag check)
	for (auto& pObj : m_gameObjects)
	{
		if (pObj)
		{
			pObj->Run();
		}
	}

	// Dynamic texture tile animation
	UpdateDynamicTexture();

	return true;
}

void CGame::OnKeyDown(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = true;
		break;
	case 'W':
		if (m_bShiftKeyDown)
			m_camOffsetY = 0.05f;
		else
			m_camOffsetZ = 0.05f;
		break;
	case 'S':
		if (m_bShiftKeyDown)
			m_camOffsetY = -0.05f;
		else
			m_camOffsetZ = -0.05f;
		break;
	case 'A':
		m_camOffsetX = -0.05f;
		break;
	case 'D':
		m_camOffsetX = 0.05f;
		break;
	}
}

void CGame::OnKeyUp(UINT nChar, UINT uiScanCode)
{
	switch (nChar)
	{
	case VK_SHIFT:
		m_bShiftKeyDown = false;
		break;
	case 'W':
	case 'S':
		m_camOffsetY = 0.0f;
		m_camOffsetZ = 0.0f;
		break;
	case 'A':
	case 'D':
		m_camOffsetX = 0.0f;
		break;
	}
}

bool CGame::UpdateWindowSize(UINT backBufferWidth, UINT backBufferHeight)
{
	bool bResult = false;
	if (m_renderer)
	{
		bResult = m_renderer->UpdateWindowSize(backBufferWidth, backBufferHeight);
	}
	return bResult;
}

void CGame::Render()
{
	m_renderer->BeginRender();

	// Render all game objects
	for (auto& pObj : m_gameObjects)
	{
		pObj->Render();
	}

	// Sprite
	m_renderer->RenderSprite(m_pSpriteObject0, 100, 80, 100, 100, 0.f);

	// Dynamic texture sprite
	m_renderer->RenderSpriteWithTex(m_pSpriteObjCommon, 0, 0, 100, 100, nullptr, 0.f, m_pDynamicTexHandle);

	m_renderer->EndRender();
	m_renderer->Present();
}

CGameObject* CGame::CreateGameObject(EMeshType meshType)
{
	auto pGameObj = std::make_unique<CGameObject>();
	if (!pGameObj->Initialize(this, meshType))
	{
		return nullptr;
	}
	m_gameObjects.push_back(std::move(pGameObj));
	return m_gameObjects.back().get();
}

void CGame::DeleteGameObject(CGameObject* pGameObj)
{
	auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
		[pGameObj](const std::unique_ptr<CGameObject>& obj) { return obj.get() == pGameObj; });

	if (it != m_gameObjects.end())
	{
		m_gameObjects.erase(it);
	}
}

void CGame::UpdateDynamicTexture()
{
	const UINT TileWidth = 16;
	const UINT TileHeight = 16;
	UINT tileWidthCount = m_dynamicImageWidth / TileWidth;
	UINT tileHeightCount = m_dynamicImageHeight / TileHeight;

	if (m_tileCount >= tileWidthCount * tileHeightCount)
	{
		m_tileCount = 0;
	}

	UINT tileY = m_tileCount / tileWidthCount;
	UINT tileX = m_tileCount % tileWidthCount;
	UINT startX = tileX * TileWidth;
	UINT startY = tileY * TileHeight;

	UINT r = m_tileColorR;
	UINT g = m_tileColorG;
	UINT b = m_tileColorB;

	UINT* pDest = (UINT*)m_pDynamicImage;
	for (UINT y = 0; y < TileHeight; y++)
	{
		for (UINT x = 0; x < TileWidth; x++)
		{
			pDest[(startX + x) + (startY + y) * m_dynamicImageWidth] = 0xff000000 | (b << 16) | (g << 8) | r;
		}
	}

	m_tileCount++;
	m_tileColorR += 8;
	if (m_tileColorR > 255)
	{
		m_tileColorR = 0;
		m_tileColorG += 8;
	}
	if (m_tileColorG > 255)
	{
		m_tileColorG = 0;
		m_tileColorB += 8;
	}
	if (m_tileColorB > 255)
	{
		m_tileColorB = 0;
	}

	//다이나믹 텍스쳐 데이터 업로드 버퍼에 반영
	m_renderer->UpdateTextureWithImage(m_pDynamicTexHandle, m_pDynamicImage, m_dynamicImageWidth, m_dynamicImageHeight);
}

void CGame::Cleanup()
{
	// Game objects must be destroyed before the renderer
	m_gameObjects.clear();

	if (m_pDynamicImage)
	{
		free(m_pDynamicImage);
		m_pDynamicImage = nullptr;
	}

	if (m_renderer)
	{
		if (m_pSpriteObject0)
		{
			m_renderer->DeleteSpriteObject(m_pSpriteObject0);
			m_pSpriteObject0 = nullptr;
		}
		if (m_pSpriteObjCommon)
		{
			m_renderer->DeleteSpriteObject(m_pSpriteObjCommon);
			m_pSpriteObjCommon = nullptr;
		}
		if (m_pDynamicTexHandle)
		{
			m_renderer->DeleteTexture(m_pDynamicTexHandle);
			m_pDynamicTexHandle = nullptr;
		}
	}
}
