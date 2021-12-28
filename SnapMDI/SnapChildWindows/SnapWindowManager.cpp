#include SW_PCH_FNAME
#include "SnapWindowManager.h"
#include "SnapPreviewWnd.h"
#include "GhostDividerWnd.h"
#include "DividePreviewWnd.h"
#include "SnapAssistWnd.h"
#include <algorithm>

#undef min
#undef max

namespace SnapChildWindows
{

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

	using WndEdge = CSnapWindowManager::WndEdge;

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

	inline bool ShouldIncludeWindow(CWnd* pChildWnd) const
	{
		return m_pManager->ShouldIncludeWindow(pChildWnd);
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
		if (pWndChild == m_pCurWnd || !ShouldIncludeWindow(pWndChild))
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
	 		for (size_t jj = 1; jj < wnds.size(); ++jj)
	 		{
	 			auto& wnd1 = wnds[jj - 1];
	 			auto& wnd2 = wnds[jj];
	 			auto& wi1 = m_vChildRects[wnd1.rectIdx];
	 			auto& wi2 = m_vChildRects[wnd2.rectIdx];
				if (wi1.bAdjacentToOwner || wi1.bAdjacentToSibling
					|| wi2.bAdjacentToOwner || wi2.bAdjacentToSibling)
					continue;
	 			auto v1Max = *(LONG*)((BYTE*)&wi1.rect + divMaxOff);
	 			auto v2Min = *(LONG*)((BYTE*)&wi2.rect + divMinOff);
	 			bool bSticked = v2Min <= v1Max;
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

	using DividerWndInfo = CSnapWindowManager::DividerWndInfo;

	bool MatchDividerWindow(const StickedWndDiv& div, const DividerWndInfo& wi) const;

	int m_nNCHittestRes = HTNOWHERE;
	bool m_bVertical = false;
	LONG m_nCurDivPos = 0;

	int m_nCurWndIndex = -1;	// index to m_vChildRects

	size_t m_nStickedChildCount = 0;
};

void DivideWindowsHelper::InitChildWndInfosForDivider()
{
	m_bVertical = m_nNCHittestRes == HTLEFT || m_nNCHittestRes == HTRIGHT;
	auto pWndOwner = GetOwnerWnd();

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

	enum {MatchNone, MatchFirstEdge, MatchSecondEdge};

	CWnd* pWndChild = pWndOwner->GetWindow(GW_CHILD);
	for (; pWndChild; pWndChild = pWndChild->GetNextWindow(GW_HWNDNEXT))
	{
		if (!ShouldIncludeWindow(pWndChild))
			continue;
		ChildWndInfo childInfo = { pWndChild->GetSafeHwnd() };
		pWndChild->GetWindowRect(&childInfo.rect);
		auto& rect = childInfo.rect;

		auto pos1 = *(LONG*)((BYTE*)&rect + divPosOff1);
		auto pos2 = *(LONG*)((BYTE*)&rect + divPosOff2);

		auto match = MatchNone;
		if (MatchPosition(m_nCurDivPos, pos1))
		{
			match = MatchFirstEdge;
		}
		else if (MatchPosition(m_nCurDivPos, pos2))
		{
			match = MatchSecondEdge;
		}
		if (match != MatchNone)
		{
			size_t rectIdx = m_vChildRects.size();
			if (childInfo.hWndChild == m_pCurWnd->GetSafeHwnd())
				m_nCurWndIndex = (int)rectIdx;

			InsertWindowEdgeInfo(m_nCurDivPos, m_bVertical, rectIdx, match == MatchFirstEdge);
			++m_nStickedChildCount;
			m_vChildRects.emplace_back(childInfo);
		}
	}
	ASSERT(m_vChildRects.empty() || m_nCurWndIndex >= 0);
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
			auto& wndEdge1 = wnds[ii - 1];
			auto& wndEdge2 = wnds[ii];
			auto& wi1 = m_vChildRects[wndEdge1.rectIdx];
			auto& wi2 = m_vChildRects[wndEdge2.rectIdx];

			auto v1Min = *(LONG*)((BYTE*)&wi1.rect + divMinOff);
			auto v1Max = *(LONG*)((BYTE*)&wi1.rect + divMaxOff);
			auto v2Min = *(LONG*)((BYTE*)&wi2.rect + divMinOff);

			if (!bFoundCurWnd)
			{
				bFoundCurWnd = wi1.hWndChild == hCurWnd;
			}

			if (bNewSticky)
			{
				bNewSticky = false;
				nCurDivPos2 = v1Min;
				div.length = v1Max - v1Min;
				div.wnds.clear();
				div.wnds.push_back(wndEdge1);
			}

			bool bSticked = v2Min <= nCurDivPos2 + div.length;

			if (bSticked)
			{
				div.wnds.push_back(wndEdge2);
				auto v2Max = *(LONG*)((BYTE*)&wi2.rect + divMaxOff);
				auto len = v2Max - nCurDivPos2;
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
					div.wnds.clear();
					div.wnds.push_back(wndEdge2);
				}
			}
		}
		ASSERT(bFoundCurWnd);
	}
}

bool DivideWindowsHelper::MatchDividerWindow(const StickedWndDiv& div, const DividerWndInfo& wi) const
{
	if (div.vertical ^ wi.wnd->IsVertical())
		return false;
	size_t posOff1 = 0, posOff2 = 0;
	if (div.vertical)
	{
		posOff1 = offsetof(POINT, x);
		posOff2 = offsetof(POINT, y);
	}
	else
	{
		posOff1 = offsetof(POINT, y);
		posOff2 = offsetof(POINT, x);
	}
	auto& divPos2 = *(LONG*)((BYTE*)&div.pos + posOff2);
	auto& wiPos2 = *(LONG*)((BYTE*)&wi.pos + posOff2);
	if (divPos2 != wiPos2)
		return false;
	auto& divPos1 = *(LONG*)((BYTE*)&div.pos + posOff1);
	auto& wiPos1 = *(LONG*)((BYTE*)&wi.pos + posOff1);
	if (!MatchPosition(divPos1, wiPos1))
		return false;
	return true;
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

BOOL CSnapWindowManager::PreWndMsg(SnapWndMsg& msg)
{
	if (!IsEnabled())
		return FALSE;
	switch (msg.message)
	{
	case WM_ENTERSIZEMOVE:
		HandleEnterSizeMove(msg);
		break;
	case WM_EXITSIZEMOVE:
		if (m_bEnterSizeMove)
			HandleExitSizeMove(msg);
		break;
	case WM_MOVING:
		if (m_bEnterSizeMove)
			HandleMoving(msg);
		break;
	case WM_SIZING:
		if (m_bEnterSizeMove)
			HandleSizing(msg);
		break;
	case WM_NCMOUSEMOVE:
		HandleNCMouseMove(msg);
		break;
	case WM_NCMOUSELEAVE:
		HandleNCMouseLeave(msg);
		break;
	}
	return FALSE;
}

static HHOOK g_hGetMessageHook = nullptr;
static BOOL g_bHasLButtonUpDuringSizeMove = FALSE;

LRESULT CALLBACK GetMsgHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code == HC_ACTION)
	{
		auto pMsg = (MSG*)lParam;
		// Whiling dragging the window around, users can press ESC to abort.
		// It seems that there is no way of telling if that happens (since
		// there is no WM_KEYDOWN messages), WM_LBUTTONUP is posted to the
		// message queue instead of directly sent to the window (PreWndMsg
		// won't be able to catch it. So we have to rely on this.
		if (pMsg->message == WM_LBUTTONUP)
		{
			g_bHasLButtonUpDuringSizeMove = TRUE;
		}
	}
	return CallNextHookEx(g_hGetMessageHook, code, wParam, lParam);
}

void CSnapWindowManager::HandleEnterSizeMove(SnapWndMsg& msg)
{
	if (m_div.valid)
	{
		m_bEnterSizeMove = EnterDividing(msg);
		m_bDividing = m_bEnterSizeMove;
	}
	else
	{
		m_bEnterSizeMove = EnterSnapping(msg);
	}
	if (!m_bEnterSizeMove)
		return;
	ASSERT(!g_hGetMessageHook);
	g_hGetMessageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgHookProc, (HINSTANCE)nullptr, GetCurrentThreadId());
	m_snapTarget = SnapTargetTypeUnknown;
	g_bHasLButtonUpDuringSizeMove = FALSE;
	msg.pHelper->GetWnd()->GetWindowRect(&m_rcCurSnapWndStart);
	GetCursorPos(&m_ptStart);
}

void CSnapWindowManager::HandleExitSizeMove(SnapWndMsg& msg)
{
	ASSERT(m_bEnterSizeMove);
	BOOL bAborted = !g_bHasLButtonUpDuringSizeMove;
	if (m_bIsMoving)
	{
		ASSERT(msg.pHelper == m_pCurSnapWnd);
		StopMoving(bAborted);
		m_bIsMoving = FALSE;
	}
	else if (m_bDividing)
	{
		StopDividing(bAborted);
		m_bDividing = FALSE;
	}
	m_snapTarget = SnapTargetType::None;
	m_bEnterSizeMove = FALSE;
	m_pCurSnapWnd = nullptr;

	ASSERT(g_hGetMessageHook);
	UnhookWindowsHookEx(g_hGetMessageHook);
	g_hGetMessageHook = nullptr;
}

void CSnapWindowManager::HandleMoving(SnapWndMsg& msg)
{
	ASSERT(m_bEnterSizeMove);
	if (m_snapTarget == SnapTargetType::None)
		return;
	CPoint ptCurrent;
	GetCursorPos(&ptCurrent);
	if (!m_bIsMoving)
	{
		if (ptCurrent == m_ptStart)
			return;
		if (m_snapTarget == SnapTargetTypeUnknown)
		{
			m_snapTarget = InitMovingSnap(msg);
			if (m_snapTarget == SnapTargetType::None)
			{
				m_bEnterSizeMove = FALSE;
				return;
			}
		}
		m_bIsMoving = TRUE;
		StartMoving(msg.pHelper);
	}
	OnMoving(ptCurrent);
}

void CSnapWindowManager::HandleSizing(SnapWndMsg& msg)
{
	ASSERT(m_bEnterSizeMove);
	if (m_bDividing && m_wndDividePreview)
	{
		// TODO
		auto& rect = *(RECT*)msg.lp;
		LONG* pPosV = nullptr;
		switch (m_nNCHittestRes)
		{
		case HTLEFT:
			pPosV = &rect.left;
			break;
		case HTTOP:
			pPosV = &rect.top;
			break;
		case HTRIGHT:
			pPosV = &rect.right;
			break;
		case HTBOTTOM:
			pPosV = &rect.bottom;
			break;
		default:
			ASSERT(0);
			return;
		}
		auto& pos = *pPosV;
		if (pos < m_minmaxDiv.nMin)
			pos = m_minmaxDiv.nMin;
		else if (pos > m_minmaxDiv.nMax)
			pos = m_minmaxDiv.nMax;

		size_t rcPosOff1 = 0, rcPosOff2 = 0;
		if (m_div.vertical)
		{
			rcPosOff1 = offsetof(RECT, left);
			rcPosOff2 = offsetof(RECT, right);
		}
		else
		{
			rcPosOff1 = offsetof(RECT, top);
			rcPosOff2 = offsetof(RECT, bottom);
		}

		for (auto& we : m_div.wnds)
		{
			auto& wi = m_vDivideChildRects[we.rectIdx];
			auto& wipos = *(LONG*)( (BYTE*)&wi.rect + (we.bFirst ? rcPosOff1 : rcPosOff2) );
			wipos = pos;
		}

		m_wndDividePreview->UpdateDivideWindows();
	}
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
	if (!m_wndSnapPreview)
	{
		m_wndSnapPreview.reset(new CSnapPreviewWnd());
		m_wndSnapPreview->Create(m_pWndOwner);
	}
	auto pSnapWnd = m_wndSnapPreview.get();
	if (pSnapWnd)
	{
		pSnapWnd->EnableAnimation(IsAnimationEnabled());
		m_pCurSnapWnd = pWndHelper;
		PreSnapInitialize();

		pSnapWnd->StartSnapping();
	}
}

void CSnapWindowManager::StopMoving(bool bAbort)
{
	if (!m_wndSnapPreview)
		return;
	EnableSnapSwitchCheck(false);

	m_wndSnapPreview->StopSnapping();

	if (!bAbort && m_curGrid.type != SnapGridType::None)
	{
		auto targetType = (SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask);
		if (CanDoSnapping(targetType))
			OnSnapToCurGrid();
	}

	m_curGrid = { SnapGridType::None };
	m_vChildRects.clear();
}

void CSnapWindowManager::OnSnapToCurGrid()
{
	SnapWindowGridPos grids;
	GetSnapWindowGridPosResult(grids);
	bool bSnapNow = true;
#if 1
	if (IsAnimationEnabled() && ShouldDoAnimationToSnapCurGrid())
	{
		auto pWnd = GetSnapAssistWnd();
		if (pWnd)
		{
			bSnapNow = false;
			std::swap(pWnd->m_data.initSnapGridWnds.wndPos, grids);
		}
	}
#endif
	if (bSnapNow)
	{
		SnapWindowsToGridResult(grids);
	}
#if 1
	SnapLayoutWindows layout;
	if (IsSnapAssistEnabled())
	{
		if ((SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask) == SnapTargetType::Owner)
		{
			if (!m_vChildRects.empty())
			{
				GetOwnerLayoutForSnapAssist(layout);
				ASSERT(layout.wnds.size() == layout.layout.cells.size());
				if (!layout.wnds.empty())
				{
					FindMatchedWindowsInSnapLayout(layout);
				}
			}
		}
	}
	if (!bSnapNow || !layout.wnds.empty())
		ShowSnapAssist(std::move(layout));
#endif
}

bool CSnapWindowManager::ShouldDoAnimationToSnapCurGrid() const
{
	return true;
}

void CSnapWindowManager::SnapWindowsToGridResult(const SnapWindowGridPos& grids)
{
	for (auto& wnd : grids.wnds)
	{
		CRect rect = wnd.rect;
		m_pWndOwner->ScreenToClient(&rect);
		::SetWindowPos(wnd.hWnd, nullptr, rect.left, rect.top, rect.Width(), rect.Height(), wnd.flags);
	}
}

void CSnapWindowManager::GetSnapWindowGridPosResult(SnapWindowGridPos& grids) const
{
	CWnd* pWnd = m_pCurSnapWnd->GetWnd();
	ASSERT(pWnd->GetParent() == m_pWndOwner);

	WindowPos wnd = {0};
	wnd.hWnd = pWnd->GetSafeHwnd();
	wnd.rect = m_curGrid.rect;
	grids.wnds.emplace_back(wnd);

	GetAdditionalSnapWindowGridPosResult(grids);
}

void CSnapWindowManager::GetAdditionalSnapWindowGridPosResult(SnapWindowGridPos& grids) const
{
	if ((SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask) == SnapTargetType::Child)
	{
		ASSERT(!m_vChildRects.empty());
		WindowPos wnd = { 0 };
		wnd.hWnd = m_curGrid.childInfo.hWndChild;
		wnd.flags = SWP_NOZORDER | SWP_NOACTIVATE;
		if (CheckSnapSwapWndsSwitch())
		{
			wnd.rect = m_rcCurSnapWndStart;
		}
		else
		{
			wnd.rect = m_curGrid.childInfo.rect;
			switch ((SnapGridType)((DWORD)m_curGrid.type & SnapGridSideMask))
			{
			case SnapGridType::Left:
				wnd.rect.left = m_curGrid.rect.right;
				break;
			case SnapGridType::Top:
				wnd.rect.top = m_curGrid.rect.bottom;
				break;
			case SnapGridType::Right:
				wnd.rect.right = m_curGrid.rect.left;
				break;
			case SnapGridType::Bottom:
				wnd.rect.bottom = m_curGrid.rect.top;
				break;
			}
		}
		grids.wnds.emplace_back(wnd);
	}
}

bool CSnapWindowManager::GetOwnerLayoutForSnapAssist(SnapLayoutWindows& layout) const
{
	CSize szGrid = m_rcOwner.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;

	SnapCellInfo cell;

	CWnd* pWnd = m_pCurSnapWnd->GetWnd();
	HWND hWnd = pWnd->GetSafeHwnd();

	bool b4Cells = false, bSecondCell = false;

	switch (((DWORD)m_curGrid.type & SnapGridSideMask))
	{
	case SnapGridType::Right:
		bSecondCell = true;
		// fall through
	case SnapGridType::Left:
		// Left
		cell.rect = m_rcOwner;
		cell.rect.right = m_rcOwner.left + szGrid.cx;
		layout.layout.cells.push_back(cell);
		// Right
		cell.rect = m_rcOwner;
		cell.rect.left += szGrid.cx;
		layout.layout.cells.push_back(cell);
		break;
	case SnapGridType::Bottom:
		bSecondCell = true;
		// fall through
	case SnapGridType::Top:
		// Top
		cell.rect = m_rcOwner;
		cell.rect.bottom = m_rcOwner.top + szGrid.cy;
		layout.layout.cells.push_back(cell);
		// Bottom
		cell.rect = m_rcOwner;
		cell.rect.top += szGrid.cy;
		layout.layout.cells.push_back(cell);
		break;
	default:
		b4Cells = true;
		break;
	}
	if (b4Cells)
	{
		// Top left, 0
		cell.rect = m_rcOwner;
		cell.rect.right = m_rcOwner.left + szGrid.cx;
		cell.rect.bottom = cell.rect.top + szGrid.cy;
		layout.layout.cells.push_back(cell);
		// Top right, 1
		cell.rect = m_rcOwner;
		cell.rect.left += szGrid.cx;
		cell.rect.bottom = cell.rect.top + szGrid.cy;
		layout.layout.cells.push_back(cell);
		// Bottom left, 2
		cell.rect = m_rcOwner;
		cell.rect.right = m_rcOwner.left + szGrid.cx;
		cell.rect.top += szGrid.cy;
		layout.layout.cells.push_back(cell);
		// Bottom right, 3
		cell.rect = m_rcOwner;
		cell.rect.left += szGrid.cx;
		cell.rect.top += szGrid.cy;
		layout.layout.cells.push_back(cell);

		int nIndex = 0;
		switch (((DWORD)m_curGrid.type & SnapGridSideMask))
		{
		default:
			ASSERT(0);
			// fall through
		case (DWORD)SnapGridType::Left | (DWORD)SnapGridType::Top:
			break;
		case (DWORD)SnapGridType::Right | (DWORD)SnapGridType::Top:
			nIndex = 1;
			break;
		case (DWORD)SnapGridType::Left | (DWORD)SnapGridType::Bottom:
			nIndex = 2;
			break;
		case (DWORD)SnapGridType::Right | (DWORD)SnapGridType::Bottom:
			nIndex = 3;
			break;
		}

		layout.wnds.resize(4);
		layout.wnds[nIndex] = hWnd;
	}
	else
	{
		layout.wnds.resize(2);
		layout.wnds[bSecondCell] = hWnd;
	}

	return true;
}

void CSnapWindowManager::FindMatchedWindowsInSnapLayout(SnapLayoutWindows& layout) const
{
	ASSERT(!layout.wnds.empty());
	for (size_t ii = 0; ii < layout.wnds.size(); ++ii)
	{
		if (layout.wnds[ii])
			continue;
		auto& cell = layout.layout.cells[ii];
		for (auto& wi : m_vChildRects)
		{
			if (EqualRect(&wi.rect, &cell.rect))
			{
				layout.wnds[ii] = wi.hWndChild;
				break;
			}
		}
	}
}

CSnapAssistWnd* CSnapWindowManager::GetSnapAssistWnd()
{
	if (!m_wndSnapAssist)
	{
		m_wndSnapAssist = std::make_unique<CSnapAssistWnd>(this);
		if (!m_wndSnapAssist->Create(m_pWndOwner))
		{
			m_wndSnapAssist.reset();
			return nullptr;
		}
	}
	auto pWnd = m_wndSnapAssist.get();
	if (pWnd)
	{
		pWnd->EnableAnimation(IsAnimationEnabled());
	}
	return pWnd;
}

bool CSnapWindowManager::ShowSnapAssist(SnapLayoutWindows&& layout)
{
	auto pWnd = GetSnapAssistWnd();
	if (!pWnd)
	{
		ASSERT(0);
		return false;
	}

	if (!layout.wnds.empty())
	{
		for (HWND hWnd = ::GetWindow(m_pWndOwner->GetSafeHwnd(), GW_CHILD); hWnd; hWnd = GetNextWindow(hWnd, GW_HWNDNEXT))
		{
			if (!IsWindowVisible(hWnd))
				continue;
			bool bAlreadyInLayout = false;
			for (auto& wnd : layout.wnds)
			{
				if (hWnd == wnd)
				{
					bAlreadyInLayout = true;
					break;
				}
			}
			if (!bAlreadyInLayout)
			{
				pWnd->m_data.candidateWnds.push_back({ hWnd });
			}
		}
	}

	pWnd->m_data.snapLayoutWnds = std::move(layout);
	pWnd->Show();
	return true;
}

void CSnapWindowManager::OnSnapAssistEvent(SnapAssistEvent event)
{
	if (!m_wndSnapAssist)
	{
		ASSERT(0);
		return;
	}
	auto& data = m_wndSnapAssist->m_data;
	bool bFinish = false;
	switch (event)
	{
	case SnapAssistEvent::FinishAnimation:
		bFinish = data.candidateWnds.empty();
		break;
	case SnapAssistEvent::Exit:
		bFinish = true;
		break;
	}
	if (!data.initSnapGridWnds.wndPos.wnds.empty())
	{
		SnapWindowsToGridResult(data.initSnapGridWnds.wndPos);
		data.initSnapGridWnds.RemoveAll();
	}
	if (bFinish)
	{
		m_wndSnapAssist->DestroyWindow();
		m_wndSnapAssist.reset();
	}
}

BOOL CSnapWindowManager::EnterDividing(const SnapWndMsg& msg)
{
	if (GetKeyState(VK_LBUTTON) > 0)
		return FALSE;
	if (!CalcMinMaxDividingPos(msg, m_minmaxDiv))
		return FALSE;
	if (!m_wndDividePreview)
	{
		m_wndDividePreview.reset(new CDividePreviewWnd(this));
		if (!m_wndDividePreview->Create(m_pWndOwner))
			return FALSE;
	}
	m_pCurSnapWnd = msg.pHelper;
	ASSERT(m_nActiveDivideWndIdx >= 0);
	//UINT nFlags = SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOOWNERZORDER;
	//for (int ii = 0; ii < (int)m_vChildRects.size(); ++ii)
	//{
	//	if (ii == m_nCurSnapWndIdx)
	//		continue;
	//	auto& wnd = m_vChildRects[ii];
	//	SetWindowPos(wnd.hWndChild, nullptr, 0, 0, 0, 0, nFlags);
	//}
	m_wndDividePreview->EnableAnimation(IsAnimationEnabled());
	m_wndDividePreview->Show();
	return TRUE;
}

void CSnapWindowManager::StopDividing(bool bAbort)
{
	ASSERT(m_nActiveDivideWndIdx >= 0);
	UINT nFlags = /*SWP_SHOWWINDOW|*/SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_NOACTIVATE;
	//if (bAbort)
	//{
	//	nFlags |= SWP_NOMOVE | SWP_NOSIZE;
	//}
	if (!bAbort)
	{
		for (int ii = 0; ii < (int)m_vDivideChildRects.size(); ++ii)
		{
			if (ii == m_nActiveDivideWndIdx)
				continue;
			auto& wnd = m_vDivideChildRects[ii];
			CRect rc = wnd.rect;
			m_pWndOwner->ScreenToClient(&rc);
			SetWindowPos(wnd.hWndChild, nullptr, rc.left, rc.top, rc.Width(), rc.Height(), nFlags);
		}
	}

	m_wndDividePreview->Hide();
}

BOOL CSnapWindowManager::CalcMinMaxDividingPos(const SnapWndMsg& msg, MinMaxDivideInfo& minmax) const
{
	minmax.nMin = LONG_MIN;
	minmax.nMax = LONG_MAX;

	size_t rcPosOff1 = 0, rcPosOff2 = 0;
	size_t ptPosOff = 0;
	if (m_div.vertical)
	{
		rcPosOff1 = offsetof(RECT, left);
		rcPosOff2 = offsetof(RECT, right);
		ptPosOff = offsetof(POINT, x);
	}
	else
	{
		rcPosOff1 = offsetof(RECT, top);
		rcPosOff2 = offsetof(RECT, bottom);
		ptPosOff = offsetof(POINT, y);
	}

	MINMAXINFO minmaxDefault = { 0 };
	minmaxDefault.ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
	minmaxDefault.ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
	minmaxDefault.ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
	minmaxDefault.ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);

	for (auto& we : m_div.wnds)
	{
		MINMAXINFO mmi = minmaxDefault;
		auto& wi = m_vDivideChildRects[we.rectIdx];
		SendMessage(wi.hWndChild, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);

		RECT rect;
		GetWindowRect(wi.hWndChild, &rect);

		LONG nMin = 0, nMax = 0;
		if (we.bFirst)
		{
			auto rcMax = *(LONG*)((BYTE*)&rect + rcPosOff2);
			nMin = rcMax - *(LONG*)((BYTE*)&mmi.ptMaxTrackSize + ptPosOff);
			nMax = rcMax - *(LONG*)((BYTE*)&mmi.ptMinTrackSize + ptPosOff);
		}
		else
		{
			auto rcMin = *(LONG*)((BYTE*)&rect + rcPosOff1);
			nMin = rcMin + *(LONG*)((BYTE*)&mmi.ptMinTrackSize + ptPosOff);
			nMax = rcMin + *(LONG*)((BYTE*)&mmi.ptMaxTrackSize + ptPosOff);
		}
		minmax.nMin = std::max(minmax.nMin, nMin);
		minmax.nMax = std::min(minmax.nMax, nMax);
	}

	return TRUE;
}

void CSnapWindowManager::OnMoving(CPoint pt)
{
	if (!m_wndSnapPreview)
		return;
	auto grid = GetSnapGridInfo(pt);
	if (m_curGrid.type != grid.type || !m_curGrid.rect.EqualRect(&grid.rect))
	{
		m_curGrid = grid;
		if ((SnapTargetType)((DWORD)m_curGrid.type & (DWORD)SnapTargetMask) == SnapTargetType::None)
		{
			m_wndSnapPreview->Hide();
		}
		else
		{
			m_wndSnapPreview->ShowAt(m_pCurSnapWnd->GetWnd(), grid.rect);
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
#ifdef _DEBUG
	CPerformanceCheck _check;
#endif // _DEBUG

	SnapWindowsHelper helper;
	helper.m_pManager = this;
	helper.m_pCurWnd = msg.pHelper->GetWnd();

	helper.InitChildWndInfosForSnap();
	SnapTargetType targetType = (SnapTargetType)((DWORD)SnapTargetType::Owner|(DWORD)SnapTargetType::Slot);
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
	if ((BYTE)m_snapTarget & (BYTE)SnapTargetType::Slot)
	{
		auto grid = GetSnapEmptySlotGridInfo(pt);
		if (grid.type != SnapGridType::None)
			return grid;
	}
	return { SnapGridType::None };
}

BOOL CSnapWindowManager::CanDoSnapping(SnapTargetType target) const
{
	bool bSwitch = CheckSnapOnOffSwitch();
	bool bReverse = target == SnapTargetType::Slot;
	return !(bSwitch ^ bReverse);
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
		grid.rect.left += szGrid.cx;
	}

	const LONG nLeftRightTestArea = m_rcOwner.Width() / 3;

	if (pt.y < grid.rect.top + s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Top);
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else if (pt.y > grid.rect.bottom - s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Bottom);
		grid.rect.top += szGrid.cy;
	}
	if (grid.type != SnapGridType::None)
	{
		if ( !((DWORD)grid.type & ((DWORD)SnapGridType::Left | (DWORD)SnapGridType::Right)) )
		{
			if (pt.x < grid.rect.left + nLeftRightTestArea)
			{
				grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Left);
				grid.rect.right = grid.rect.left + szGrid.cx;
			}
			else if (pt.x > grid.rect.right - nLeftRightTestArea)
			{
				grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Right);
				grid.rect.left += szGrid.cx;
			}
		}
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapTargetType::Owner);
	}
	return grid;
}

auto CSnapWindowManager::GetSnapChildGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_rcOwner };
	if (m_vChildRects.empty())
		return grid;
	if (!CanDoSnapping(SnapTargetType::Child))
		return grid;
	int nCount = (int)m_vChildRects.size();
	for (int ii = 0; ii < nCount; ++ii)
	{
		auto& wnd = m_vChildRects[ii];
		if (PtInRect(&wnd.rect, pt))
		{
			if (wnd.bAdjacentToOwner || wnd.bAdjacentToSibling)
			{
				bool bOverlapped = false;
				for (int jj = ii-1; jj >= 0; --jj)
				{
					auto& wnd2 = m_vChildRects[jj];
					RECT temp;
					if (IntersectRect(&temp, &wnd.rect, &wnd2.rect))
					{
						bOverlapped = true;
						break;
					}
				}
				if (!bOverlapped)
				{
					auto gridTemp = GetSnapChildGridInfoEx(pt, wnd);
					if (gridTemp.type != SnapGridType::None)
						return gridTemp;
				}
			}
			break;
		}
	}
	return grid;
}

auto CSnapWindowManager::GetSnapChildGridInfoEx(CPoint pt, const ChildWndInfo& childInfo) const -> SnapGridInfo
{
	SnapGridInfo grid = { (SnapGridType)SnapTargetType::None, m_rcOwner };
	grid.rect = childInfo.rect;
	grid.childInfo = childInfo;
	if (CheckSnapSwapWndsSwitch())
	{
		grid.type = SnapGridType::Child;
	}
	else
	{
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
	}
	return grid;
}

auto CSnapWindowManager::GetSnapEmptySlotGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { (SnapGridType)SnapTargetType::None, m_rcOwner };
	if (m_vChildRects.empty())
		return grid;
	if (!CanDoSnapping(SnapTargetType::Slot))
		return grid;

	if ( pt.x <= m_rcOwner.left || pt.y <= m_rcOwner.top
		|| pt.x >= m_rcOwner.right || pt.y >= m_rcOwner.bottom)
		return grid;

	size_t nSize = m_vChildRects.size() * 2 + 2;
	std::vector<LONG> vx; vx.reserve(nSize);
	std::vector<LONG> vy; vy.reserve(nSize);

	vx.push_back(m_rcOwner.left);
	vx.push_back(m_rcOwner.right);
	vy.push_back(m_rcOwner.top);
	vy.push_back(m_rcOwner.bottom);

	for (auto& wnd : m_vChildRects)
	{
		if (PtInRect(&wnd.rect, pt))
			return grid;
		vx.push_back(wnd.rect.left);
		vx.push_back(wnd.rect.right);
		vy.push_back(wnd.rect.top);
		vy.push_back(wnd.rect.bottom);
	}

	std::sort(vx.begin(), vx.end());
	std::sort(vy.begin(), vy.end());

	auto end = std::remove_if(vx.begin(), vx.end(), [lastPos=LONG_MIN](LONG pos) mutable {
		if (lastPos == pos)
			return true;
		lastPos = pos;
		return false;
	});
	vx.erase(end, vx.end());
	if (vx.size() < 2)
		return grid;

	end = std::remove_if(vy.begin(), vy.end(), [lastPos=LONG_MIN](LONG pos) mutable {
		if (lastPos == pos)
			return true;
		lastPos = pos;
		return false;
	});
	vy.erase(end, vy.end());
	if (vy.size() < 2)
		return grid;

	std::vector <std::vector<bool>> vWndGrids;
	vWndGrids.resize(vy.size() - 1);
	for (auto& gridRow : vWndGrids)
		gridRow.resize(vx.size() - 1);

	for (auto& wnd : m_vChildRects)
	{
		auto& rect = wnd.rect;
		// must exist, no checking
		auto itx1 = std::lower_bound(vx.begin(), vx.end(), rect.left);
		auto itx2 = std::lower_bound(itx1, vx.end(), rect.right);
		auto ity1 = std::lower_bound(vy.begin(), vy.end(), rect.top);
		auto ity2 = std::lower_bound(ity1, vy.end(), rect.bottom);
		auto nx1 = std::distance(vx.begin(), itx1);
		auto nx2 = std::distance(vx.begin(), itx2);
		auto ny1 = std::distance(vy.begin(), ity1);
		auto ny2 = std::distance(vy.begin(), ity2);
		for (auto yy = ny1; yy < ny2; ++yy)
		{
			auto& gridRow = vWndGrids[yy];
			for (auto xx = nx1; xx < nx2; ++xx)
			{
				gridRow[xx] = true;
			}
		}
	}

	// must exist, no checking
	auto itx = std::lower_bound(vx.begin(), vx.end(), pt.x);
	auto ity = std::lower_bound(vy.begin(), vy.end(), pt.y);
	auto nx = (int)std::distance(vx.begin(), itx) - 1;
	auto ny = (int)std::distance(vy.begin(), ity) - 1;

	auto nYMin = -1;
	for (auto yy = ny-1; yy >= 0; --yy)
	{
		auto& gridRow = vWndGrids[yy];
		if (gridRow[nx])
		{
			nYMin = yy+1;
			break;
		}
	}
	if (nYMin < 0)
		nYMin = 0;
	int nXMin = 0, nXMax = (int)vx.size()-1;
	for (auto yy = nYMin; yy <= ny; ++yy)
	{
		auto& gridRow = vWndGrids[yy];
		for (auto xl = nx-1; xl >= nXMin; --xl)
		{
			if (gridRow[xl])
			{
				if (xl+1 > nXMin)
					nXMin = xl+1;
				break;
			}
		}
		for (auto xr = nx+1; xr < nXMax; ++xr)
		{
			if (gridRow[xr])
			{
				if (xr < nXMax)
					nXMax = xr;
				break;
			}
		}
	}

	grid.rect.left		= vx[nXMin];
	grid.rect.top		= vy[nYMin];
	grid.rect.right		= vx[nXMax];
	grid.rect.bottom	= vy[ny+1];

	const LONG nMinLimit = std::max(m_curSnapWndMinMax.ptMinTrackSize.x, m_curSnapWndMinMax.ptMinTrackSize.y);
	if (grid.rect.Width() < nMinLimit
		|| grid.rect.Height() < nMinLimit)
		return grid;

	grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapTargetType::Slot);
	return grid;
}

bool CSnapWindowManager::ShouldIncludeWindow(CWnd* pChildWnd) const
{
	return pChildWnd->IsWindowVisible() && !pChildWnd->IsIconic();
}

void CSnapWindowManager::OnSnapOnOffSwitch()
{
	if (m_bIsMoving)
	{
		CPoint ptCurrent;
		GetCursorPos(&ptCurrent);
		OnMoving(ptCurrent);
	}
}

enum
{
	SnapSwitchCheckInterval			= 100,
	SnapOnOffSwitchVirtualkey		= VK_SHIFT,
	SnapSwapWndsSwitchVirtualkey	= VK_MENU,
};

void CSnapWindowManager::EnableSnapSwitchCheck(bool bEnable)
{
	if (bEnable)
	{
		if (!m_nTimerIDSnapSwitch)
		{
			m_nTimerIDSnapSwitch = SetTimer(0, SnapSwitchCheckInterval);
			m_bOnOffSwitchPressed = CheckSnapOnOffSwitch();
			m_bSwapWndsSwitchPressed = CheckSnapSwapWndsSwitch();
		}
	}
	else if (m_nTimerIDSnapSwitch)
	{
		KillTimer(m_nTimerIDSnapSwitch);
		m_nTimerIDSnapSwitch = 0;
	}
}

bool CSnapWindowManager::CheckSnapOnOffSwitch() const
{
	bool bPressed = GetAsyncKeyState(SnapOnOffSwitchVirtualkey) < 0;
	return bPressed;
}

bool CSnapWindowManager::CheckSnapSwapWndsSwitch() const
{
	bool bPressed = GetAsyncKeyState(SnapSwapWndsSwitchVirtualkey) < 0;
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
		bool bSnapOnOffSwitch = CheckSnapOnOffSwitch();
		bool bSwapWndsSwitch = CheckSnapSwapWndsSwitch();
		bool bDoSwitching = (bSnapOnOffSwitch ^ m_bOnOffSwitchPressed) || (bSwapWndsSwitch ^ m_bSwapWndsSwitchPressed);
		if (bDoSwitching)
		{
			OnSnapOnOffSwitch();
			m_bOnOffSwitchPressed = bSnapOnOffSwitch;
			m_bSwapWndsSwitchPressed = bSwapWndsSwitch;
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
	bool bShowDivider = false;
	if (!CheckSnapOnOffSwitch())
	{
		switch (msg.wp)
		{
		case HTLEFT:
		case HTTOP:
		case HTRIGHT:
		case HTBOTTOM:
			bShowDivider = true;
			break;
		}
	}
	if (bShowDivider)
	{
		CheckShowGhostDivider(msg);
	}
	else
	{
		HideGhostDividers();
	}
}

void CSnapWindowManager::HandleNCMouseLeave(SnapWndMsg& msg)
{
	HideGhostDividers();
}

#ifdef _DEBUG
//#define DEBUG_DIVIDER_SHOW_HIDE
#endif // _DEBUG

void CSnapWindowManager::CheckShowGhostDivider(SnapWndMsg& msg)
{
#ifdef _DEBUG
	CPerformanceCheck _check;
#endif // _DEBUG
	DivideWindowsHelper helper;
	helper.m_pManager = this;
	helper.m_pCurWnd = msg.pHelper->GetWnd();
	helper.m_nNCHittestRes = (int)msg.wp;
	helper.InitChildWndInfosForDivider();

	if (helper.m_nStickedChildCount < 2)
		return;

	StickedWndDiv div;
	helper.CheckStickedWindows(div);

	if (div.wnds.size() < 2)
		return;
#ifdef DEBUG_DIVIDER_SHOW_HIDE
	TRACE("++ Show old %d,%d v=%d\r\n", m_div.pos.x, m_div.pos.y, m_div.vertical);
#endif // DEBUG_DIVIDER_SHOW_HIDE

	m_vDivideChildRects.swap(helper.m_vChildRects);
	std::swap(m_div, div);
	m_div.valid = true;

	m_nActiveDivideWndIdx = helper.m_nCurWndIndex;
	m_nNCHittestRes = helper.m_nNCHittestRes;

	CGhostDividerWnd* pWnd = nullptr;

	for (auto& wi : m_vGhostDividerWnds)
	{
		if (!pWnd && helper.MatchDividerWindow(m_div, wi))
		{
			pWnd = wi.wnd;
		}
		else
		{
			wi.wnd->Hide();
		}
	}
#ifdef DEBUG_DIVIDER_SHOW_HIDE
	TRACE("++ Show new %d,%d v=%d\r\n", m_div.pos.x, m_div.pos.y, m_div.vertical);
#endif // DEBUG_DIVIDER_SHOW_HIDE

	if (!pWnd)
	{
		DividerWndInfo wndInfo = { 0 };
		wndInfo.pos = m_div.pos;
		wndInfo.wnd = new CGhostDividerWnd(this, helper.m_bVertical);
		m_vGhostDividerWnds.emplace_back(wndInfo);
		pWnd = wndInfo.wnd;
		pWnd->Create(m_pWndOwner, m_div.pos, m_div.length);
	}
	pWnd->EnableAnimation(IsAnimationEnabled());
	pWnd->Show();
}

static bool operator<(const POINT& l, const POINT& r)
{
	return (l.x < r.x || (l.x == r.x && l.y < r.y));
}

void CSnapWindowManager::HideGhostDividers()
{
#ifdef DEBUG_DIVIDER_SHOW_HIDE
	TRACE("-- Hide     %d,%d v=%d\r\n", m_div.pos.x, m_div.pos.y, m_div.vertical);
#endif // DEBUG_DIVIDER_SHOW_HIDE
	m_div.valid = false;
	DivideWindowsHelper helper;
	for (auto& wi : m_vGhostDividerWnds)
	{
		wi.wnd->Hide();
	}
}

void CSnapWindowManager::OnGhostDividerWndHidden(CGhostDividerWnd* pWnd)
{
	auto it = std::find_if(m_vGhostDividerWnds.begin(), m_vGhostDividerWnds.end(), [pWnd](auto& wi) {
		return wi.wnd == pWnd;
		});
	if (it != m_vGhostDividerWnds.end())
	{
		it->wnd->DestroyWindow();
		m_vGhostDividerWnds.erase(it);
	}
	else
	{
		ASSERT(0);
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

}
