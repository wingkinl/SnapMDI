#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"
#include "SnapPreviewWnd.h"

static int s_nHotRgnSize = -1;

#define SnapTargetTypeUnknown (CSnapWindowManager::SnapTargetType)0xff

CSnapWindowManager::CSnapWindowManager()
{
}

CSnapWindowManager::~CSnapWindowManager()
{

}

void CSnapWindowManager::InitSnap(CWnd* pWndOwner)
{
	ASSERT(!m_pWndOwner);
	m_pWndOwner = pWndOwner;
}

BOOL CSnapWindowManager::OnWndMsg(const SnapWndMsg& msg)
{
	switch (msg.message)
	{
	case WM_ENTERSIZEMOVE:
		// Do not handle it for system menu
		m_bEnterSizeMove = EnterSnapping(msg);
		if (m_bEnterSizeMove)
		{
			m_snapTarget = SnapTargetTypeUnknown;
			GetCursorPos(&m_ptStart);
		}
		break;
	case WM_EXITSIZEMOVE:
		if (m_bEnterSizeMove)
		{
			if (m_bIsMoving)
			{
				ASSERT(msg.pHelper == m_pCurSnapWnd);
				StopMoving();
				m_bIsMoving = FALSE;
			}
			m_snapTarget = SnapTargetType::None;
			m_bEnterSizeMove = FALSE;
		}
		break;
	case WM_MOVING:
		if (!m_bEnterSizeMove)
			break;
		if (m_snapTarget != SnapTargetType::None)
		{
			CPoint ptCurrent;
			GetCursorPos(&ptCurrent);
			if (!m_bIsMoving)
			{
				if (ptCurrent == m_ptStart)
					break;
				if (m_snapTarget == SnapTargetTypeUnknown)
				{
					m_snapTarget = InitMovingSnap(msg);
					if (m_snapTarget == SnapTargetType::None)
					{
						m_bEnterSizeMove = FALSE;
						break;
					}
				}
				m_bIsMoving = TRUE;
				StartMoving(msg.pHelper);
			}
			OnMoving(ptCurrent);
		}
		break;
	}
	return FALSE;
}

CSnapPreviewWnd* CSnapWindowManager::GetSnapPreview()
{
	if (!m_wndSnapPreview)
	{
		m_wndSnapPreview.reset(new CSnapPreviewWnd);
		m_wndSnapPreview->Create(m_pWndOwner);
	}
	return m_wndSnapPreview.get();
}

void CSnapWindowManager::PreSnapInitialize()
{
	if (s_nHotRgnSize < 0)
	{
		s_nHotRgnSize = GetSystemMetrics(SM_CYCAPTION);
	}
}

void CSnapWindowManager::StartMoving(CSnapWindowHelper* pWndHelper)
{
	auto pSnapWnd = GetSnapPreview();
	if (pSnapWnd)
	{
		m_pCurSnapWnd = pWndHelper;
		PreSnapInitialize();
		pSnapWnd->StartSnapping();
	}
	else
	{
		m_pCurSnapWnd = nullptr;
	}
}

void CSnapWindowManager::StopMoving(bool bAbort)
{
	if (!m_wndSnapPreview)
		return;
	m_wndSnapPreview->StopSnapping(bAbort);
	if (!bAbort && m_curGrid.type)
	{
		CRect rect;
		m_wndSnapPreview->GetSnapRect(rect);
		UINT nFlags = SWP_FRAMECHANGED;
		CWnd* pWnd = m_pCurSnapWnd->GetWnd();
		ASSERT(pWnd->GetParent() == m_pWndOwner);
		m_pWndOwner->ScreenToClient(rect);
		pWnd->SetWindowPos(nullptr, rect.left, rect.top, rect.Width(), rect.Height(), nFlags);
	}

	m_pCurSnapWnd = nullptr;
	m_curGrid = { 0 };
}

void CSnapWindowManager::OnMoving(CPoint pt)
{
	if (!m_wndSnapPreview)
		return;
	auto grid = GetSnapGridInfo(pt);
	if (m_curGrid.type != grid.type || (SnapTargetType)(grid.type & (DWORD)SnapGridType::SnapTargetMask) == SnapTargetType::Custom)
	{
		m_curGrid = grid;
		switch ((SnapTargetType)(m_curGrid.type & (DWORD)SnapGridType::SnapTargetMask))
		{
		case SnapTargetType::None:
			m_wndSnapPreview->Hide();
			break;
		default:
			m_wndSnapPreview->ShowAt(m_pCurSnapWnd->GetWnd(), grid.rect);
			break;
		}
	}
}

BOOL CSnapWindowManager::EnterSnapping(const SnapWndMsg& msg) const
{
	if (GetKeyState(VK_LBUTTON) > 0)
		return FALSE;
	if (msg.pHelper->GetWnd()->IsIconic())
		return FALSE;
	return TRUE;
}

auto CSnapWindowManager::InitMovingSnap(const SnapWndMsg& msg) -> SnapTargetType
{
	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);
	auto startTime = GetTickCount();
	CWnd* pWndChild = m_pWndOwner->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		if (pWndChild != msg.pHelper->GetWnd())
		{
			CRect rect;
			pWndChild->GetWindowRect(&rect);
			if (rect.left == m_rcOwner.left || rect.top == m_rcOwner.top
				|| rect.right == m_rcOwner.right || rect.bottom == m_rcOwner.bottom)
				return SnapTargetType::Child;
		}
		pWndChild = pWndChild->GetNextWindow(GW_HWNDNEXT);
		if (GetTickCount() - startTime > 20)
			break;
	}
	return SnapTargetType::Owner;
}

auto CSnapWindowManager::GetSnapGridInfo(CPoint pt) const -> SnapGridInfo
{
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Child)
	{
		return GetSnapChildGridInfo(pt);
	}
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Owner)
	{
		return GetSnapOwnerGridInfo(pt);
	}
	return { (DWORD)SnapTargetType::None };
}

CSnapWindowManager::SnapGridInfo CSnapWindowManager::GetSnapOwnerGridInfo(CPoint pt) const
{
	SnapGridInfo grid = { (DWORD)SnapTargetType::None, m_rcOwner };
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	if (pt.x < grid.rect.left + s_nHotRgnSize)
	{
		grid.type = (DWORD)SnapGridType::Left;
		grid.rect.right = grid.rect.left + szGrid.cx;
	}
	else if (pt.x > grid.rect.right - s_nHotRgnSize)
	{
		grid.type = (DWORD)SnapGridType::Right;
		grid.rect.left = grid.rect.right - szGrid.cx;
	}
	if (pt.y < grid.rect.top + s_nHotRgnSize)
	{
		grid.type = (grid.type | (DWORD)SnapGridType::Top);
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else if (pt.y > grid.rect.bottom - s_nHotRgnSize)
	{
		grid.type = ((DWORD)grid.type | (DWORD)SnapGridType::Bottom);
		grid.rect.top = grid.rect.bottom - szGrid.cy;
	}
	if (grid.type)
	{
		grid.type = ((DWORD)grid.type | (DWORD)SnapTargetType::Owner);
	}
	return grid;
}

CSnapWindowManager::SnapGridInfo CSnapWindowManager::GetSnapChildGridInfo(CPoint pt) const
{
	SnapGridInfo grid = { (DWORD)SnapTargetType::None, m_rcOwner };
	return grid;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSnapWindowHelper::CSnapWindowHelper()
{
}

CSnapWindowHelper::~CSnapWindowHelper()
{

}

void CSnapWindowHelper::InitSnap(CSnapWindowManager* pManager, CWnd* pWnd)
{
	ASSERT(!m_pManager && !m_pWnd);
	m_pManager = pManager;
	m_pWnd = pWnd;
}

