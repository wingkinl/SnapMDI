#include "pch.h"
#include "framework.h"
#include "AlphaLayeredAnimationWnd.h"
#include <algorithm>

#undef min
#undef max

CAlphaLayeredAnimationWnd::CAlphaLayeredAnimationWnd()
{
	
}

CAlphaLayeredAnimationWnd::~CAlphaLayeredAnimationWnd()
{
}

void CAlphaLayeredAnimationWnd::RepositionWindow(const CRect& rect)
{
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(),
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);
	RedrawWindow();
}

void CAlphaLayeredAnimationWnd::GetWindowInOwnerRect(CRect& rect, CWnd* pWnd) const
{
	if (pWnd)
		pWnd->GetWindowRect(rect);
	rect.left = std::max(rect.left, m_rcOwner.left);
	rect.top = std::max(rect.top, m_rcOwner.top);
	rect.right = std::min(rect.right, m_rcOwner.right);
	rect.bottom = std::min(rect.bottom, m_rcOwner.bottom);
}

void CAlphaLayeredAnimationWnd::StopAnimation()
{
	if (m_nAniTimerID)
	{
		KillTimer(m_nAniTimerID);
		m_nAniTimerID = 0;
	}
}

void CAlphaLayeredAnimationWnd::FinishAnimationCleanup()
{
	StopAnimation();
	if (m_aniStage != AnimateStage::Showing)
	{
		ShowWindow(SW_HIDE);
		m_pActiveSnapWnd = nullptr;
		m_aniStage = AnimateStage::Ready;
	}
}

enum
{	AnimationInterval = 10
};

void CAlphaLayeredAnimationWnd::ScheduleAnimation()
{
	m_AniStartTime = std::chrono::steady_clock::now();
	m_nAniTimerID = SetTimer(100, AnimationInterval, nullptr);
}

bool CAlphaLayeredAnimationWnd::ShouldDoAnimation() const
{
	if (!m_bEnableAnimation)
		return false;
	if (!m_renderImp)
		return false;
	return m_renderImp->CanSupportAnimation();
}

BOOL CAlphaLayeredAnimationWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (m_renderImp)
	{
		if (m_renderImp->OnWndMsg(message, wParam, lParam, pResult))
			return TRUE;
	}
	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CAlphaLayeredAnimationWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapWindowBase message handlers

BOOL CAlphaLayeredAnimationWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

// How to programmatically move a window slowly, as if the user were doing it?
// see https://stackoverflow.com/questions/932706/how-to-programmatically-move-a-window-slowly-as-if-the-user-were-doing-it
inline static double SmoothMoveELX(double x)
{
	double dPI = atan(1) * 4;
	return (cos((1 - x) * dPI) + 1) / 2;
}

LONG CAlphaLayeredAnimationWnd::CalcSmoothPos(double pos, LONG from, LONG to)
{
	if (from == to || pos > 1.)
		return to;
	auto newPos = from * (1 - SmoothMoveELX(pos))
		+ to * SmoothMoveELX(pos);
	//auto newPos = from + (to - from) * pos;
	return (LONG)newPos;
}

CRect CAlphaLayeredAnimationWnd::AnimateRect(double pos, const RECT& from, const RECT& to)
{
	CRect rect;
	rect.left = CalcSmoothPos(pos, from.left, to.left);
	rect.top = CalcSmoothPos(pos, from.top, to.top);
	rect.right = CalcSmoothPos(pos, from.right, to.right);
	rect.bottom = CalcSmoothPos(pos, from.bottom, to.bottom);
	return rect;
}

void CAlphaLayeredAnimationWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != m_nAniTimerID)
		return;
	auto curTime = std::chrono::steady_clock::now();
	std::chrono::duration<double> timeDiff = curTime - m_AniStartTime;
	auto timeDiffCount = timeDiff.count();

	OnAnimationTimer(timeDiffCount);
}

void CAlphaLayeredAnimationWnd::OnDestroy()
{
	StopAnimation();
	__super::OnDestroy();
}
