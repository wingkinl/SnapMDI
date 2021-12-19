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

	if (!bAbort && m_curGrid.type != SnapGridType::None)
	{
		OnFinshSnapping();
	}

	m_pCurSnapWnd = nullptr;
	m_curGrid = { SnapGridType::None };
	m_vChildRects.clear();
}

void CSnapWindowManager::OnFinshSnapping()
{
	CRect rectSnap = m_curGrid.rect;
	UINT nFlags = SWP_FRAMECHANGED;
	CWnd* pWnd = m_pCurSnapWnd->GetWnd();
	ASSERT(pWnd->GetParent() == m_pWndOwner);
	m_pWndOwner->ScreenToClient(rectSnap);
	pWnd->SetWindowPos(nullptr, rectSnap.left, rectSnap.top, rectSnap.Width(), rectSnap.Height(), nFlags);
}

void CSnapWindowManager::OnMoving(CPoint pt)
{
	if (!m_wndSnapPreview)
		return;
	auto grid = GetSnapGridInfo(pt);
	if (m_curGrid.type != grid.type || !m_curGrid.rect.EqualRect(&grid.rect))
	{
		m_curGrid = grid;
		switch ((SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask))
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
	m_vChildRects.clear();
	m_vChildRects.reserve(20);
	size_t nCount = 0;
	CWnd* pWndChild = m_pWndOwner->GetWindow(GW_CHILD);
	// It is reasonable to assume that the active window must be the first child
	ASSERT(pWndChild && pWndChild == msg.pHelper->GetWnd());
	SnapTargetType targetType = SnapTargetType::Owner;
	while (pWndChild)
	{
		if (pWndChild != msg.pHelper->GetWnd())
		{
			ChildWndInfo childInfo = {pWndChild->GetSafeHwnd()};
			pWndChild->GetWindowRect(&childInfo.rect);
			auto& rect = childInfo.rect;
			if (rect.left == m_rcOwner.left)
				childInfo.states |= (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerLeft;
			if (rect.top == m_rcOwner.top)
				childInfo.states |= (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerTop;
			if (rect.right == m_rcOwner.right)
				childInfo.states |= (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerRight;
			if (rect.bottom == m_rcOwner.bottom)
				childInfo.states |= (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerBottom;
			if (childInfo.states)
			{
				targetType = SnapTargetType::Child;
				// TEMP test
				childInfo.states |= (BYTE)ChildWndInfo::StateFlag::BorderWithSibling;
			}
			m_vChildRects.emplace_back(childInfo);
		}
		pWndChild = pWndChild->GetNextWindow(GW_HWNDNEXT);
		if (GetTickCount() - startTime > 20)
		{
			TRACE("CSnapWindowManager some child windows are skipped to avoid lagging.\r\n");
			break;
		}
	}
	return targetType;
}

auto CSnapWindowManager::GetSnapGridInfo(CPoint pt) const -> SnapGridInfo
{
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Child)
	{
		auto grid = GetSnapChildGridInfo(pt);
		if (grid.type != SnapGridType::None)
			return grid;
	}
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Owner)
	{
		return GetSnapOwnerGridInfo(pt);
	}
	return { SnapGridType::None };
}

auto CSnapWindowManager::GetSnapOwnerGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_rcOwner };
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	if (pt.x < grid.rect.left + s_nHotRgnSize)
	{
		grid.type = SnapGridType::Left;
		grid.rect.right = grid.rect.left + szGrid.cx;
	}
	else if (pt.x > grid.rect.right - s_nHotRgnSize)
	{
		grid.type = SnapGridType::Right;
		grid.rect.left = grid.rect.right - szGrid.cx;
	}
	if (pt.y < grid.rect.top + s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Top);
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else if (pt.y > grid.rect.bottom - s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Bottom);
		grid.rect.top = grid.rect.bottom - szGrid.cy;
	}
	if (grid.type != SnapGridType::None)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapTargetType::Owner);
	}
	return grid;
}

auto CSnapWindowManager::GetSnapChildGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_rcOwner };
	if (GetKeyState(VK_SHIFT) < 0)
	{
		for (auto& wnd : m_vChildRects)
		{
			if (wnd.states & (BYTE)ChildWndInfo::StateFlag::BorderWithSibling)
			{
				if (PtInRect(&wnd.rect, pt))
					return GetSnapChildGridInfoEx(pt, wnd);
			}
		}
	}
	return grid;
}

auto CSnapWindowManager::GetSnapChildGridInfoEx(CPoint pt, const ChildWndInfo& childInfo) const -> SnapGridInfo
{
	SnapGridInfo grid = { (SnapGridType)SnapTargetType::Child, m_rcOwner };
	grid.rect = childInfo.rect;
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	auto halfGridCx = szGrid.cx / 2;
	if (pt.x < grid.rect.left + halfGridCx)
	{
		grid.rect.right = grid.rect.left + szGrid.cx;
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Left);
	}
	else if (pt.x > grid.rect.right - halfGridCx)
	{
		grid.rect.left = grid.rect.right - szGrid.cx;
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Right);
	}
	else if (pt.y < grid.rect.top + szGrid.cy)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Top);
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Bottom);
		grid.rect.top = grid.rect.bottom - szGrid.cy;
	}
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

