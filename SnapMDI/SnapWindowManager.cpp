#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"
#include "SnapPreviewWnd.h"
#include <algorithm>
#include <map>

#undef min
#undef max

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

SnapWndMsg::HandleResult CSnapWindowManager::PreWndMsg(SnapWndMsg& msg)
{
	switch (msg.message)
	{
	case WM_ENTERSIZEMOVE:
		// Do not handle it for system menu
		m_bEnterSizeMove = EnterSnapping(msg);
		if (m_bEnterSizeMove)
		{
			m_snapTarget = SnapTargetTypeUnknown;
			m_bCheckMoveAbortion = FALSE;
			m_bAborted = FALSE;
			GetCursorPos(&m_ptStart);
		}
		break;
	case WM_EXITSIZEMOVE:
		if (m_bEnterSizeMove)
		{
			if (m_bIsMoving)
			{
				ASSERT(msg.pHelper == m_pCurSnapWnd);
				StopMoving(m_bAborted);
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
	case WM_CHILDACTIVATE:
		if (m_bEnterSizeMove)
			m_bCheckMoveAbortion = TRUE;
		break;
	case WM_MOVE:
		// Whiling dragging the window around, users can press ESC to abort.
		// It seems that WM_MOVE after WM_CHILDACTIVATE indicates a restoration of window position
		if (m_bCheckMoveAbortion)
			m_bAborted = TRUE;
		break;
	case WM_NCHITTEST:
		return SnapWndMsg::HandleResult::NeedPostWndMsg;
	}
	return SnapWndMsg::HandleResult::Continue;
}

LRESULT CSnapWindowManager::PostWndMsg(SnapWndMsg& msg)
{
	switch (msg.message)
	{
	case WM_NCHITTEST:
		break;
	}
	return msg.result;
}

CSnapPreviewWnd* CSnapWindowManager::GetSnapPreview()
{
	if (!m_wndSnapPreview)
	{
		m_wndSnapPreview.reset(new CSnapPreviewWnd(this));
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
	m_curSnapWndMinMax.ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
	m_curSnapWndMinMax.ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
	m_curSnapWndMinMax.ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
	m_curSnapWndMinMax.ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);

	m_pCurSnapWnd->GetWnd()->SendMessage(WM_GETMINMAXINFO, 0, (LPARAM)&m_curSnapWndMinMax);

	ASSERT(m_curSnapWndMinMax.ptMinTrackSize.x <= m_curSnapWndMinMax.ptMaxTrackSize.x);
	ASSERT(m_curSnapWndMinMax.ptMinTrackSize.y <= m_curSnapWndMinMax.ptMaxTrackSize.y);
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
		if (OnSnapTo())
			OnAfterSnap();
	}

	m_pCurSnapWnd = nullptr;
	m_curGrid = { SnapGridType::None };
	m_vChildRects.clear();
}

BOOL CSnapWindowManager::OnSnapTo()
{
	CRect rectSnap = m_curGrid.rect;
	UINT nFlags = SWP_FRAMECHANGED;
	CWnd* pWnd = m_pCurSnapWnd->GetWnd();
	ASSERT(pWnd->GetParent() == m_pWndOwner);
	m_pWndOwner->ScreenToClient(&rectSnap);
	pWnd->SetWindowPos(nullptr, rectSnap.left, rectSnap.top, rectSnap.Width(), rectSnap.Height(), nFlags);
	return TRUE;
}

void CSnapWindowManager::OnAfterSnap()
{
	switch ((SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask))
	{
	case SnapTargetType::Owner:
		OnAfterSnapToOwner();
		break;
	case SnapTargetType::Child:
		OnAfterSnapToChild();
		break;
	case SnapTargetType::Custom:
		OnAfterSnapToCustom();
		break;
	}
}

void CSnapWindowManager::OnAfterSnapToOwner()
{
	if (m_vChildRects.empty())
		return;

	CDC* pDC = m_pWndOwner->GetDC();

	CDC memDC;
	memDC.CreateCompatibleDC(pDC);

	m_pWndOwner->ReleaseDC(pDC);

	for (auto& wnd : m_vChildRects)
	{
		LPBYTE lpBits = NULL;
		auto& rect = (CRect&)wnd.rect;
		HBITMAP hBmp = CDrawingManager::CreateBitmap_32(rect.Size(), (LPVOID*)&lpBits);
		if (hBmp == NULL)
		{
			return;
		}
		SendMessage(wnd.hWndChild, WM_PRINT, (WPARAM)memDC.GetSafeHdc(), (LPARAM)(PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN | PRF_NONCLIENT));
		//pWndChild->PrintWindow(&memDC, 0);
	}
}

void CSnapWindowManager::OnAfterSnapToChild()
{
	ASSERT(!m_vChildRects.empty());
	CRect rectSnap = m_curGrid.childInfo->rect;
	switch ((SnapGridType)((DWORD)m_curGrid.type & SnapGridSideMask))
	{
	case SnapGridType::Left:
		rectSnap.left = m_curGrid.rect.right;
		break;
	case SnapGridType::Top:
		rectSnap.top = m_curGrid.rect.bottom;
		break;
	case SnapGridType::Right:
		rectSnap.right = m_curGrid.rect.left;
		break;
	case SnapGridType::Bottom:
		rectSnap.bottom = m_curGrid.rect.top;
		break;
	}
	UINT nFlags = SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE;
	m_pWndOwner->ScreenToClient(&rectSnap);
	SetWindowPos(m_curGrid.childInfo->hWndChild, nullptr, rectSnap.left, rectSnap.top, rectSnap.Width(), rectSnap.Height(), nFlags);
}

void CSnapWindowManager::OnAfterSnapToCustom()
{

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

	struct WndBorderInfo
	{
		size_t	rectIdx;	// points to m_vChildRects
		bool	bFirst;		// first edge or the second
	};
	std::map<LONG, std::vector<WndBorderInfo>> wndWithSharedX, wndWithSharedY;

	while (pWndChild)
	{
		if (pWndChild != msg.pHelper->GetWnd())
		{
			ChildWndInfo childInfo = {pWndChild->GetSafeHwnd()};
			pWndChild->GetWindowRect(&childInfo.rect);
			auto& rect = childInfo.rect;
			if (rect.left == m_rcOwner.left)
				childInfo.states = (ChildWndInfo::StateFlag)((BYTE)childInfo.states | (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerLeft);
			else
				wndWithSharedX[rect.left].push_back({ m_vChildRects.size(), true });
			if (rect.top == m_rcOwner.top)
				childInfo.states = (ChildWndInfo::StateFlag)((BYTE)childInfo.states | (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerTop);
			else
				wndWithSharedY[rect.top].push_back({ m_vChildRects.size(), true });
			if (abs(rect.right - m_rcOwner.right) <= 1)
				childInfo.states = (ChildWndInfo::StateFlag)((BYTE)childInfo.states | (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerRight);
			else
				wndWithSharedX[rect.right].push_back({m_vChildRects.size(), false});
			if (abs(rect.bottom - m_rcOwner.bottom) <= 1)
				childInfo.states = (ChildWndInfo::StateFlag)((BYTE)childInfo.states | (BYTE)ChildWndInfo::StateFlag::BorderWithOwnerBottom);
			else
				wndWithSharedY[rect.bottom].push_back({m_vChildRects.size(), false});
			if (childInfo.states != ChildWndInfo::StateFlag::BorderWithNone)
			{
				targetType = (SnapTargetType)((DWORD)targetType | (DWORD)SnapTargetType::Child);
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
	if (m_vChildRects.size() >= 2 && ((DWORD)targetType & (DWORD)SnapTargetType::Child))
	{
		for (auto& sharedX : wndWithSharedX)
		{
			auto& wnds = sharedX.second;
			for (auto it1 = wnds.begin(); it1 != wnds.end(); ++it1)
			{
				auto& wnd1 = m_vChildRects[it1->rectIdx];
				if (wnd1.states != ChildWndInfo::StateFlag::BorderWithNone)
					continue;
				for (auto it2 = std::next(it1); it2 != wnds.end(); ++it2)
				{
					auto& wnd2 = m_vChildRects[it2->rectIdx];
					ChildWndInfo* pWnds[] = {&wnd1, &wnd2};
					if (wnd1.rect.top > wnd2.rect.top)
						std::swap(pWnds[0], pWnds[1]);
					bool bSnapable = false;
					if (it1->bFirst ^ it2->bFirst)
					{
						bSnapable = pWnds[1]->rect.top < pWnds[0]->rect.bottom;
					}
					else
					{
						bSnapable = pWnds[1]->rect.top == pWnds[0]->rect.bottom;
					}
					if (bSnapable)
					{
						wnd1.states = (ChildWndInfo::StateFlag)((BYTE)wnd1.states | (BYTE)ChildWndInfo::StateFlag::BorderWithSibling);
						wnd2.states = (ChildWndInfo::StateFlag)((BYTE)wnd2.states | (BYTE)ChildWndInfo::StateFlag::BorderWithSibling);
					}
				}
			}
		}
		for (auto& sharedY : wndWithSharedY)
		{
			auto& wnds = sharedY.second;
			for (auto it1 = wnds.begin(); it1 != wnds.end(); ++it1)
			{
				auto& wnd1 = m_vChildRects[it1->rectIdx];
				if (wnd1.states != ChildWndInfo::StateFlag::BorderWithNone)
					continue;
				for (auto it2 = std::next(it1); it2 != wnds.end(); ++it2)
				{
					auto& wnd2 = m_vChildRects[it2->rectIdx];
					ChildWndInfo* pWnds[] = { &wnd1, &wnd2 };
					if (wnd1.rect.left > wnd2.rect.left)
						std::swap(pWnds[0], pWnds[1]);
					bool bSnapable = false;
					if (it1->bFirst ^ it2->bFirst)
					{
						bSnapable = pWnds[1]->rect.left < pWnds[0]->rect.right;
					}
					else
					{
						bSnapable = pWnds[1]->rect.left == pWnds[0]->rect.right;
					}
					if (bSnapable)
					{
						wnd1.states = (ChildWndInfo::StateFlag)((BYTE)wnd1.states | (BYTE)ChildWndInfo::StateFlag::BorderWithSibling);
						wnd2.states = (ChildWndInfo::StateFlag)((BYTE)wnd2.states | (BYTE)ChildWndInfo::StateFlag::BorderWithSibling);
					}
				}
			}
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

BOOL CSnapWindowManager::IsSnappingApplicable(SnapTargetType target) const
{
	if (m_wndSnapPreview->CheckSnapSwitch())
		return false;
	return true;
}

auto CSnapWindowManager::GetSnapOwnerGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_rcOwner };
	if (!IsSnappingApplicable(SnapTargetType::Owner))
		return grid;
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	szGrid.cx = std::min(m_curSnapWndMinMax.ptMaxTrackSize.x, szGrid.cx);
	szGrid.cx = std::max(m_curSnapWndMinMax.ptMinTrackSize.x, szGrid.cx);
	szGrid.cy = std::min(m_curSnapWndMinMax.ptMaxTrackSize.y, szGrid.cy);
	szGrid.cy = std::max(m_curSnapWndMinMax.ptMinTrackSize.y, szGrid.cy);
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
	if (!IsSnappingApplicable(SnapTargetType::Child))
		return grid;
	for (auto& wnd : m_vChildRects)
	{
		if (wnd.states != ChildWndInfo::StateFlag::BorderWithNone)
		{
			if (PtInRect(&wnd.rect, pt))
				return GetSnapChildGridInfoEx(pt, wnd);
		}
	}
	return grid;
}

auto CSnapWindowManager::GetSnapChildGridInfoEx(CPoint pt, const ChildWndInfo& childInfo) const -> SnapGridInfo
{
	SnapGridInfo grid = { (SnapGridType)SnapTargetType::None, m_rcOwner };
	grid.rect = childInfo.rect;
	grid.childInfo = &childInfo;
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	szGrid.cx = std::max(m_curSnapWndMinMax.ptMinTrackSize.x, szGrid.cx);
	szGrid.cy = std::max(m_curSnapWndMinMax.ptMinTrackSize.y, szGrid.cy);
	auto halfGridCx = szGrid.cx / 2;
	if (pt.x < grid.rect.left + halfGridCx)
	{
		grid.type = SnapGridType::Left;
		grid.rect.right = grid.rect.left + szGrid.cx;
	}
	else if (pt.x > grid.rect.right - halfGridCx)
	{
		grid.type = SnapGridType::Right;
		grid.rect.left = grid.rect.right - szGrid.cx;
	}
	else if (pt.y < grid.rect.top + szGrid.cy)
	{
		grid.type = SnapGridType::Top;
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else
	{
		grid.type = SnapGridType::Bottom;
		grid.rect.top = grid.rect.bottom - szGrid.cy;
	}
	if (grid.type != SnapGridType::None)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapTargetType::Child);
	}
	return grid;
}

void CSnapWindowManager::OnSnapSwitch(bool bPressed)
{
	if (bPressed)
	{
		m_curGrid.type = SnapGridType::None;
		m_wndSnapPreview->Hide();
	}
	else
	{
		CPoint ptCurrent;
		GetCursorPos(&ptCurrent);
		OnMoving(ptCurrent);
	}
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

