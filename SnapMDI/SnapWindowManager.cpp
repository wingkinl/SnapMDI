#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"
#include "SnapPreviewWnd.h"
#include "GhostDividerWnd.h"
#include <algorithm>

#undef min
#undef max

static int s_nHotRgnSize = -1;

#define SnapTargetTypeUnknown (CSnapWindowManager::SnapTargetType)0xff

#ifdef _DEBUG
class CPerformanceCheck
{
public:
	CPerformanceCheck()
	{
		m_nStartTime = GetTickCount();
	}
	~CPerformanceCheck()
	{
		DWORD dwTime = GetTickCount() - m_nStartTime;
		if (dwTime > 20)
		{
			// The code is too slow, try to improve it.
			// Otherwise dragging window feels laggy
			ASSERT(0);
		}
	}
	DWORD m_nStartTime;
};
#endif // _DEBUG

struct AdjacentWindowsHelper
{
	CSnapWindowManager* m_pManager;
	CWnd* m_pCurWnd;

	LONG m_nTolerance = 1;

	struct WndEdge
	{
		size_t	rectIdx;	// points to m_vChildRects
		bool	bFirst;		// first edge or the second
	};

	struct AdjacentWindows
	{
		LONG pos;
		std::vector<WndEdge>	wnds;
	};

	using ChildWndInfo = CSnapWindowManager::ChildWndInfo;
	using StickedWndDiv = CSnapWindowManager::StickedWndDiv;

	// first one in the array is for horizontal, the second one for vertical
	enum {Horizontal, Vertical};
	std::vector<AdjacentWindows>	m_adjacentWnds[2];
	std::vector<ChildWndInfo>		m_vChildRects;
	size_t							m_nStickToOwnerCount = 0;
public:
	CWnd* GetOwnerWnd() const { return m_pManager->m_pWndOwner; }
	CRect& GetOwnerRect() const { return m_pManager->m_rcOwner; }

	AdjacentWindows* FindAdjacentWindowByPosition(LONG pos, bool bVertical) const;

	void InsertWindowEdgeInfo(LONG pos, bool bVertical, size_t nRectIdx, bool bFirstEdge);

	inline bool MatchPosition(LONG p1, LONG p2) const
	{
		return std::abs(p1 - p2) <= m_nTolerance;
	}
};

auto AdjacentWindowsHelper::FindAdjacentWindowByPosition(LONG pos, bool bVertical) const -> AdjacentWindows*
{
	auto& awnds = m_adjacentWnds[bVertical];
	for (auto& wnds : awnds)
	{
		if (MatchPosition(wnds.pos, pos))
		{
			return const_cast<AdjacentWindows*>(&wnds);
		}
	}
	return nullptr;
}

void AdjacentWindowsHelper::InsertWindowEdgeInfo(LONG pos, bool bVertical, size_t nRectIdx, bool bFirstEdge)
{
	auto pWnds = FindAdjacentWindowByPosition(pos, bVertical);
	if (!pWnds)
	{
		auto& awnds = m_adjacentWnds[bVertical];
		awnds.emplace_back();
		pWnds = &awnds.back();
	}
	pWnds->pos = pos;
	pWnds->wnds.emplace_back(WndEdge{ nRectIdx, bFirstEdge });
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct SnapWindowsHelper : public AdjacentWindowsHelper
{
	void InitChildWndInfosForSnap();

	void CheckSnapableWindows();
};

void SnapWindowsHelper::InitChildWndInfosForSnap()
{
	auto pWndOwner = GetOwnerWnd();
	CWnd* pWndChild = pWndOwner->GetWindow(GW_CHILD);
	// It is reasonable to assume that the active window must be the first child
	ASSERT(pWndChild && pWndChild == m_pCurWnd);
	auto& rcOwner = GetOwnerRect();

	pWndOwner->GetClientRect(&rcOwner);
	pWndOwner->ClientToScreen(&rcOwner);

	m_vChildRects.clear();
	m_vChildRects.reserve(20);

	for (; pWndChild; pWndChild = pWndChild->GetNextWindow(GW_HWNDNEXT))
	{
		if (pWndChild == m_pCurWnd)
			continue;
		ChildWndInfo childInfo = { pWndChild->GetSafeHwnd() };
		pWndChild->GetWindowRect(&childInfo.rect);
		auto& rect = childInfo.rect;
		size_t rectIdx = m_vChildRects.size();

		if (MatchPosition(rect.left, rcOwner.left))
			childInfo.bAdjacentToOwner = true;
		else
			InsertWindowEdgeInfo(rect.left, true, rectIdx, true);

		if (MatchPosition(rect.top, rcOwner.top))
			childInfo.bAdjacentToOwner = true;
		else
			InsertWindowEdgeInfo(rect.top, false, rectIdx, true);

		if (MatchPosition(rect.right, rcOwner.right))
			childInfo.bAdjacentToOwner = true;
		else
			InsertWindowEdgeInfo(rect.right, true, rectIdx, false);

		if (MatchPosition(rect.bottom, rcOwner.bottom))
			childInfo.bAdjacentToOwner = true;
		else
			InsertWindowEdgeInfo(rect.bottom, false, rectIdx, false);

		if (childInfo.bAdjacentToOwner)
		{
			++m_nStickToOwnerCount;
		}
		m_vChildRects.emplace_back(childInfo);
	}
}

void SnapWindowsHelper::CheckSnapableWindows()
{
	ASSERT(m_vChildRects.size() >= 2 && m_nStickToOwnerCount);
	for (int ii = 0; ii < _countof(m_adjacentWnds); ++ii)
	{
		for (auto& adjWnds : m_adjacentWnds[ii])
		{
	 		auto& wnds = adjWnds.wnds;
	 		if (wnds.size() < 2)
	 			continue;
			size_t divMinOff = 0, divMaxOff = 0;
			if (ii == Vertical)
			{
				divMinOff = offsetof(RECT, top);
				divMaxOff = offsetof(RECT, bottom);
			}
			else
			{
				divMinOff = offsetof(RECT, left);
				divMaxOff = offsetof(RECT, right);
			}
			std::sort(wnds.begin(), wnds.end(), [&](auto& w1, auto& w2) {
				auto& rect1 = m_vChildRects[w1.rectIdx].rect;
				auto& rect2 = m_vChildRects[w2.rectIdx].rect;
				auto v1 = *(LONG*)((BYTE*)&rect1 + divMinOff);
				auto v2 = *(LONG*)((BYTE*)&rect2 + divMinOff);
				return v1 < v2;
			});
	 		for (size_t ii = 1; ii < wnds.size(); ++ii)
	 		{
	 			auto& wnd1 = wnds[ii - 1];
	 			auto& wnd2 = wnds[ii];
	 			auto& wi1 = m_vChildRects[wnd1.rectIdx];
	 			auto& wi2 = m_vChildRects[wnd2.rectIdx];
				if (wi1.bAdjacentToOwner || wi1.bAdjacentToSibling
					|| wi2.bAdjacentToOwner || wi2.bAdjacentToSibling)
					continue;
	 			bool bSticked = false;
	 			auto v1Max = *(LONG*)((BYTE*)&wi1.rect + divMaxOff);
	 			auto v2Min = *(LONG*)((BYTE*)&wi2.rect + divMinOff);
	 			if (wnd1.bFirst ^ wnd2.bFirst)
	 			{
	 				bSticked = v2Min < v1Max;
	 			}
	 			else
	 			{
	 				bSticked = v2Min == v1Max;
	 			}
				if (bSticked)
				{
					wi1.bAdjacentToSibling = true;
					wi2.bAdjacentToSibling = true;
				}
	 		}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct DivideWindowsHelper : public AdjacentWindowsHelper
{
	void InitChildWndInfosForDivider();

	void CheckStickedWindows(StickedWndDiv& div);

	int m_nNCHittestRes = HTNOWHERE;
	bool m_bVertical = false;
	LONG m_nCurDivPos = 0;

	size_t m_nStickedChildCount = 0;
};

void DivideWindowsHelper::InitChildWndInfosForDivider()
{
	m_bVertical = m_nNCHittestRes == HTLEFT || m_nNCHittestRes == HTRIGHT;
	auto pWndOwner = GetOwnerWnd();
	auto& rcOwner = GetOwnerRect();

	pWndOwner->GetClientRect(&rcOwner);
	pWndOwner->ClientToScreen(&rcOwner);

	CRect rcCurChild;
	m_pCurWnd->GetWindowRect(&rcCurChild);

	m_vChildRects.clear();
	m_vChildRects.reserve(20);

	size_t divPosOff1 = 0, divPosOff2 = 0;
	if (m_bVertical)
	{
		divPosOff1 = offsetof(RECT, left);
		divPosOff2 = offsetof(RECT, right);
	}
	else
	{
		divPosOff1 = offsetof(RECT, top);
		divPosOff2 = offsetof(RECT, bottom);
	}
	switch (m_nNCHittestRes)
	{
	case HTLEFT:
		m_nCurDivPos = rcCurChild.left;
		break;
	case HTRIGHT:
		m_nCurDivPos = rcCurChild.right;
		break;
	case HTTOP:
		m_nCurDivPos = rcCurChild.top;
		break;
	case HTBOTTOM:
		m_nCurDivPos = rcCurChild.bottom;
		break;
	}

	CWnd* pWndChild = pWndOwner->GetWindow(GW_CHILD);
	for (; pWndChild; pWndChild = pWndChild->GetNextWindow(GW_HWNDNEXT))
	{
		ChildWndInfo childInfo = { pWndChild->GetSafeHwnd() };
		pWndChild->GetWindowRect(&childInfo.rect);
		auto& rect = childInfo.rect;

		auto pos1 = *(LONG*)((BYTE*)&rect + divPosOff1);
		auto pos2 = *(LONG*)((BYTE*)&rect + divPosOff2);

		size_t rectIdx = m_vChildRects.size();

		if (MatchPosition(m_nCurDivPos, pos1))
		{
			InsertWindowEdgeInfo(m_nCurDivPos, m_bVertical, rectIdx, true);
			++m_nStickedChildCount;
		}
		else if (MatchPosition(m_nCurDivPos, pos2))
		{
			InsertWindowEdgeInfo(m_nCurDivPos, m_bVertical, rectIdx, false);
			++m_nStickedChildCount;
		}

		m_vChildRects.emplace_back(childInfo);
	}
}

void DivideWindowsHelper::CheckStickedWindows(StickedWndDiv& div)
{
	size_t divMinOff = 0, divMaxOff = 0;
	if (m_bVertical)
	{
		divMinOff = offsetof(RECT, top);
		divMaxOff = offsetof(RECT, bottom);
		div.pos.x = m_nCurDivPos;
	}
	else
	{
		divMinOff = offsetof(RECT, left);
		divMaxOff = offsetof(RECT, right);
		div.pos.y = m_nCurDivPos;
	}
	div.vertical = m_bVertical;

	HWND hCurWnd = m_pCurWnd->GetSafeHwnd();

	ASSERT(m_adjacentWnds[m_bVertical].size() == 1);
	
	for (auto& adjWnds : m_adjacentWnds[m_bVertical])
	{
		auto& wnds = adjWnds.wnds;
		if (wnds.size() < 2)
			continue;
		std::sort(wnds.begin(), wnds.end(), [&](auto& w1, auto& w2) {
			auto& rect1 = m_vChildRects[w1.rectIdx].rect;
			auto& rect2 = m_vChildRects[w2.rectIdx].rect;
			auto v1 = *(LONG*)((BYTE*)&rect1 + divMinOff);
			auto v2 = *(LONG*)((BYTE*)&rect2 + divMinOff);
			return v1 < v2;
		});

		auto& nCurDivPos2 = m_bVertical ? div.pos.y : div.pos.x;
		bool bFoundCurWnd = FALSE;
		bool bNewSticky = true;

		for (size_t ii = 1; ii < wnds.size(); ++ii)
		{
			auto& wnd1 = wnds[ii - 1];
			auto& wnd2 = wnds[ii];
			auto& wi1 = m_vChildRects[wnd1.rectIdx];
			auto& wi2 = m_vChildRects[wnd2.rectIdx];

			bool bSticked = false;
			auto v1Min = *(LONG*)((BYTE*)&wi1.rect + divMinOff);
			auto v1Max = *(LONG*)((BYTE*)&wi1.rect + divMaxOff);
			auto v2Min = *(LONG*)((BYTE*)&wi2.rect + divMinOff);
			if (wnd1.bFirst ^ wnd2.bFirst)
			{
				bSticked = v2Min < v1Max;
			}
			else
			{
				bSticked = v2Min == v1Max;
			}

			if (!bFoundCurWnd)
			{
				bFoundCurWnd = wi1.hWndChild == hCurWnd;
			}

			if (bNewSticky)
			{
				bNewSticky = false;
				nCurDivPos2 = v1Min;
				div.length = v1Max - v1Min;
				div.wndIds.clear();
				div.wndIds.push_back(wnd1.rectIdx);
			}

			if (bSticked)
			{
				div.wndIds.push_back(wnd2.rectIdx);
				auto v2Max = *(LONG*)((BYTE*)&wi2.rect + divMaxOff);
				auto len = v2Max - v1Min;
				if (len > div.length)
				{
					div.length = len;
				}
			}
			else
			{
				if (bFoundCurWnd)
					break;
				bNewSticky = true;
			}
			if (!bFoundCurWnd && ii == wnds.size() - 1)
			{
				bFoundCurWnd = wi2.hWndChild == hCurWnd;
				ASSERT(bFoundCurWnd);
				if (bNewSticky)
				{
					nCurDivPos2 = v2Min;
					auto v2Max = *(LONG*)((BYTE*)&wi2.rect + divMaxOff);
					div.length = v2Max - v2Min;
					div.wndIds.clear();
					div.wndIds.push_back(wnd2.rectIdx);
				}
			}
		}
		ASSERT(bFoundCurWnd);
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSnapWindowManager::CSnapWindowManager()
{
}

CSnapWindowManager::~CSnapWindowManager()
{

}

CSnapWindowManager::TimerMap CSnapWindowManager::s_mapTimers;

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
				StopMoving(m_bAborted || !CanDoSnapping(m_snapTarget));
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
	case WM_NCMOUSEMOVE:
		HandleNCMouseMove(msg);
		break;
	case WM_NCMOUSELEAVE:
		HandleNCMouseLeave(msg);
		break;
	}
	return SnapWndMsg::HandleResult::Continue;
}

CSnapPreviewWnd* CSnapWindowManager::GetSnapPreview()
{
	if (!m_wndSnapPreview)
	{
		m_wndSnapPreview.reset(new CSnapPreviewWnd());
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

	EnableSnapSwitchCheck(true);
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
	EnableSnapSwitchCheck(false);

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
#if 0
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
#endif
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
	SnapWindowsHelper helper;
	helper.m_pManager = this;
	helper.m_pCurWnd = msg.pHelper->GetWnd();

	helper.InitChildWndInfosForSnap();
	SnapTargetType targetType = SnapTargetType::Owner;
	bool bCheckAdjacentChild = helper.m_nStickToOwnerCount > 0;
	if (bCheckAdjacentChild)
		targetType = (SnapTargetType)((DWORD)targetType | (DWORD)SnapTargetType::Child);

	if (helper.m_vChildRects.size() >= 2 && bCheckAdjacentChild 
		&& helper.m_nStickToOwnerCount < helper.m_vChildRects.size())
	{
		helper.CheckSnapableWindows();
	}

	m_vChildRects.swap(helper.m_vChildRects);

	return targetType;
}

auto CSnapWindowManager::GetSnapGridInfo(CPoint pt) const -> SnapGridInfo
{
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Owner)
	{
		auto grid = GetSnapOwnerGridInfo(pt);
		if (grid.type != SnapGridType::None)
			return grid;
	}
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Child)
	{
		auto grid = GetSnapChildGridInfo(pt);
		if (grid.type != SnapGridType::None)
			return grid;
	}
	return { SnapGridType::None };
}

BOOL CSnapWindowManager::CanDoSnapping(SnapTargetType target) const
{
	if (CheckSnapSwitch())
		return false;
	return true;
}

auto CSnapWindowManager::GetSnapOwnerGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_rcOwner };
	if (!CanDoSnapping(SnapTargetType::Owner))
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
	if (!CanDoSnapping(SnapTargetType::Child))
		return grid;
	for (auto& wnd : m_vChildRects)
	{
		if (PtInRect(&wnd.rect, pt))
		{
			if (wnd.bAdjacentToOwner || wnd.bAdjacentToSibling)
				return GetSnapChildGridInfoEx(pt, wnd);
			break;
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

enum
{
	SnapSwitchCheckInterval = 100,
	SnapSWitchVirtualkey	= VK_SHIFT,
};

void CSnapWindowManager::EnableSnapSwitchCheck(bool bEnable)
{
	if (bEnable)
	{
		if (!m_nTimerIDSnapSwitch)
		{
			m_nTimerIDSnapSwitch = SetTimer(0, SnapSwitchCheckInterval);
			m_bSwitchPressed = CheckSnapSwitch();
		}
	}
	else if (m_nTimerIDSnapSwitch)
	{
		KillTimer(m_nTimerIDSnapSwitch);
		m_nTimerIDSnapSwitch = 0;
	}
}

bool CSnapWindowManager::CheckSnapSwitch() const
{
	bool bPressed = GetAsyncKeyState(SnapSWitchVirtualkey) < 0;
	return bPressed;
}

void CALLBACK CSnapWindowManager::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	CSnapWindowManager* pManager = s_mapTimers[idEvent];
	if (pManager)
	{
		pManager->OnTimer(idEvent, dwTime);
	}
}

void CSnapWindowManager::OnTimer(UINT_PTR nIDEvent, DWORD dwTime)
{
	if (nIDEvent == m_nTimerIDSnapSwitch)
	{
		bool bSwitchKeyPressed = CheckSnapSwitch();
		if (bSwitchKeyPressed ^ m_bSwitchPressed)
		{
			OnSnapSwitch(bSwitchKeyPressed);
			m_bSwitchPressed = bSwitchKeyPressed;
		}
	}
}

UINT_PTR CSnapWindowManager::SetTimer(UINT_PTR nID, UINT uElapse)
{
	nID = ::SetTimer(nullptr, nID, uElapse, TimerProc);
	s_mapTimers[nID] = this;
	return nID;
}

void CSnapWindowManager::KillTimer(UINT_PTR nID)
{
	::KillTimer(nullptr, nID);
	auto ref = s_mapTimers[nID];
	ASSERT(ref == this);
	s_mapTimers.erase(nID);
}

void CSnapWindowManager::HandleNCMouseMove(SnapWndMsg& msg)
{
	m_nNCHittestRes = (int)msg.wp;
	bool bShowDivider = false;
	switch (msg.wp)
	{
	case HTLEFT:
	case HTTOP:
	case HTRIGHT:
	case HTBOTTOM:
		bShowDivider = true;
		break;
	}
	if (bShowDivider)
	{
		CheckShowGhostDivider(msg);
	}
	else
	{
		HideLastGhostDivider();
	}
}

void CSnapWindowManager::HandleNCMouseLeave(SnapWndMsg& msg)
{
	HideLastGhostDivider();
}

void CSnapWindowManager::CheckShowGhostDivider(SnapWndMsg& msg)
{
	DivideWindowsHelper helper;
	helper.m_pManager = this;
	helper.m_pCurWnd = msg.pHelper->GetWnd();
	helper.m_nNCHittestRes = m_nNCHittestRes;
	helper.InitChildWndInfosForDivider();

	if (helper.m_nStickedChildCount < 2)
		return;

	StickedWndDiv div;
	helper.CheckStickedWindows(div);

	if (div.wndIds.size() < 2)
		return;

	auto it = m_mGhostDividerWnds.find(div.pos);
	CGhostDividerWnd* pWnd = nullptr;
	if (it == m_mGhostDividerWnds.end())
	{
		m_vChildRects.swap(helper.m_vChildRects);
		std::swap(m_div, div);

		pWnd = new CGhostDividerWnd(helper.m_bVertical);
		pWnd->Create(m_pWndOwner);
		m_mGhostDividerWnds[div.pos].reset(pWnd);
	}
	else
	{
		pWnd = it->second.get();
	}
	pWnd->Show(div.pos, div.length);
}

static bool operator<(const POINT& l, const POINT& r)
{
	return (l.x < r.x || (l.x == r.x && l.y < r.y));
}

void CSnapWindowManager::HideLastGhostDivider()
{
	auto it = m_mGhostDividerWnds.find(m_div.pos);
	if (it == m_mGhostDividerWnds.end())
		return;
	auto& wnd = it->second;
	if (wnd && wnd->IsWindowVisible())
	{
		wnd->Hide();
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

