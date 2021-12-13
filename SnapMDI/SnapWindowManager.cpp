#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"

CSnapWindowManager::CSnapWindowManager()
{

}

CSnapWindowManager::~CSnapWindowManager()
{

}

BOOL CSnapWindowManager::OnWndMsg(CWnd* pWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_ENTERSIZEMOVE:
		break;
	case WM_MOVING:
		break;
	}
	return FALSE;
}

void CSnapWindowManager::Create(CWnd* pWndOwner)
{
	m_wndSnapPreview.Create(pWndOwner);
}

CSnapWindowHelper::CSnapWindowHelper(CWnd* pWnd)
{
	m_pWnd = pWnd;
}

CSnapWindowHelper::~CSnapWindowHelper()
{

}

BOOL CSnapWindowHelper::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_ENTERSIZEMOVE:
		// Do not handle it for system menu
		m_bEnable = GetKeyState(VK_LBUTTON) < 0;
		if (m_bEnable)
		{
			GetCursorPos(&m_ptStart);
		}
		break;
	case WM_EXITSIZEMOVE:
		m_bEnable = FALSE;
		break;
	case WM_MOVING:
		if (m_bEnable)
		{
			CPoint ptCurrent;
			GetCursorPos(&ptCurrent);
			if (ptCurrent.x != m_ptStart.x || ptCurrent.y != m_ptStart.y)
			{
				if (!m_bIsMoving)
				{
					m_bIsMoving = TRUE;
				}
			}
		}
		break;
	}
	return FALSE;
}
