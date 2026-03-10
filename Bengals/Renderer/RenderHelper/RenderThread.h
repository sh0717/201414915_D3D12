#pragma once

#include <Windows.h>

class CD3D12Renderer;

enum ERenderThreadEventType
{
	RenderThreadEventTypeProcess = 0,
	RenderThreadEventTypeDestroy,
	RenderThreadEventTypeCount
};

struct RenderThreadDesc
{
	CD3D12Renderer* Renderer = nullptr;
	DWORD ThreadIndex = 0;
	HANDLE ThreadHandle = nullptr;
	HANDLE EventList[RenderThreadEventTypeCount] = {};
	HANDLE CompleteEvent = nullptr;
};

UINT WINAPI RenderThread(void* pArg);
