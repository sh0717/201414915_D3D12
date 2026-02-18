#pragma once

#include <vector>

class CD3D12Renderer;
class CGameObject;
enum class EMeshType : UINT8;

class CGame
{
public:
	CGame();
	~CGame();

	bool	Initialize(HWND hWnd, bool bEnableDebugLayer, bool bEnableGBV);
	void	Run();
	bool	Update(ULONGLONG curTick);
	void	OnKeyDown(UINT nChar, UINT uiScanCode);
	void	OnKeyUp(UINT nChar, UINT uiScanCode);
	bool	UpdateWindowSize(UINT backBufferWidth, UINT backBufferHeight);

	CD3D12Renderer* GetRenderer() const { return m_renderer.get(); }

private:
	void	Render();
	CGameObject* CreateGameObject(EMeshType meshType);
	void	DeleteGameObject(CGameObject* pGameObj);
	void	UpdateDynamicTexture();
	void	Cleanup();

private:
	std::unique_ptr<CD3D12Renderer> m_renderer = nullptr;
	HWND m_windowHandle = nullptr;

	// Game Objects
	std::vector<std::unique_ptr<CGameObject>> m_gameObjects;

	// Camera input
	bool m_bShiftKeyDown = false;
	float m_camOffsetX = 0.0f;
	float m_camOffsetY = 0.0f;
	float m_camOffsetZ = 0.0f;

	// Sprite
	void* m_pSpriteObject0 = nullptr;
	void* m_pSpriteObjCommon = nullptr;

	// Dynamic Texture
	void* m_pDynamicTexHandle = nullptr;
	BYTE* m_pDynamicImage = nullptr;
	UINT m_dynamicImageWidth = 0;
	UINT m_dynamicImageHeight = 0;
	UINT m_tileCount = 0;
	UINT m_tileColorR = 0;
	UINT m_tileColorG = 0;
	UINT m_tileColorB = 0;

	// Timing / FPS
	ULONGLONG m_previousFrameCheckTick = 0;
	ULONGLONG m_previousUpdateTick = 0;
	UINT m_frameCount = 0;
};
