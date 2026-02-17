#include "pch.h"
#include "IndexCreator.h"


CIndexCreator::CIndexCreator()
{


}

BOOL CIndexCreator::Initialize(DWORD dwNum)
{
	m_pdwIndexTable = new DWORD[dwNum];
	memset(m_pdwIndexTable, 0, sizeof(DWORD) * dwNum);
	m_dwMaxNum = dwNum;

	for (DWORD i = 0; i < m_dwMaxNum; i++)
	{
		m_pdwIndexTable[i] = i;

	}

	return TRUE;
}


DWORD CIndexCreator::Alloc()
{
	// 1. m_lAllocatedCount에서 1을 증가.
	// 2. m_lAllocatedCount-1 위치의 dwIndex를 꺼낸다.
	// 이 두 가지 액션이 필요한데 1과 2 사이에 다른 스레드가 Alloc을 호출하면 이미 할당된 인덱스를 얻는 문제가 발생한다.
	// 따라서 Alloc과 Free에서는 한 번에 끝나야 한다.

	DWORD		dwResult = -1;

	if (m_dwAllocatedCount >= m_dwMaxNum)
	{
		goto lb_return;
	}

	dwResult = m_pdwIndexTable[m_dwAllocatedCount];
	m_dwAllocatedCount++;

lb_return:
	return dwResult;

}
void CIndexCreator::Free(DWORD dwIndex)
{
	if (!m_dwAllocatedCount)
	{
		__debugbreak();
	}
	m_dwAllocatedCount--;
	m_pdwIndexTable[m_dwAllocatedCount] = dwIndex;
}

void CIndexCreator::Cleanup()
{
	if (m_pdwIndexTable)
	{
		delete[] m_pdwIndexTable;
		m_pdwIndexTable = nullptr;
	}
}

void CIndexCreator::Check()
{
	if (m_dwAllocatedCount)
		__debugbreak();
}


CIndexCreator::~CIndexCreator()
{
	Check();
	Cleanup();
}
