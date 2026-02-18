#pragma once

class CGame;
class CD3D12Renderer;

enum class EMeshType : UINT8
{
	Box,
	Quad
};

class CGameObject
{
public:
	CGameObject();
	~CGameObject();

	bool	Initialize(CGame* pGame, EMeshType meshType);
	void	SetPosition(float x, float y, float z);
	void	SetScale(float x, float y, float z);
	void	SetRotationX(float rotX);
	void	SetRotationY(float rotY);
	float	GetRotationX() const { return m_rotX; }
	float	GetRotationY() const { return m_rotY; }
	void	Run();
	void	Render();

private:
	void*	CreateBoxMesh();
	void*	CreateQuadMesh();
	void	UpdateTransform();
	void	Cleanup();

private:
	CGame* m_pGame = nullptr;
	CD3D12Renderer* m_pRenderer = nullptr;
	void* m_pMeshObj = nullptr;

	XMVECTOR m_scale = { 1.0f, 1.0f, 1.0f, 0.0f };
	XMVECTOR m_pos = {};
	float m_rotX = 0.0f;
	float m_rotY = 0.0f;

	XMMATRIX m_scaleMatrix = {};
	XMMATRIX m_rotationMatrix = {};
	XMMATRIX m_translationMatrix = {};
	XMMATRIX m_worldMatrix = {};
	bool m_bUpdateTransform = false;
};
