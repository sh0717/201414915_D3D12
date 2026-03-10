#include "pch.h"
#include <process.h>

#include "RenderThread.h"
#include "../D3D12Renderer.h"

UINT WINAPI RenderThread(void* pArg)
{
	RenderThreadDesc* pDesc = static_cast<RenderThreadDesc*>(pArg);
	if (!pDesc || !pDesc->Renderer)
	{
		_endthreadex(0);
		return 0;
	}

	const HANDLE* pEventList = pDesc->EventList;
	while (true)
	{
		DWORD eventIndex = WaitForMultipleObjects(RenderThreadEventTypeCount, pEventList, FALSE, INFINITE);
		switch (eventIndex)
		{
		case WAIT_OBJECT_0 + RenderThreadEventTypeProcess:
			pDesc->Renderer->ProcessByThread(pDesc->ThreadIndex);
			if (pDesc->CompleteEvent)
			{
				SetEvent(pDesc->CompleteEvent);
			}
			break;

		case WAIT_OBJECT_0 + RenderThreadEventTypeDestroy:
			_endthreadex(0);
			return 0;

		default:
			__debugbreak();
			break;
		}
	}
}
