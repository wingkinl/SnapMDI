#include "pch.h"
#include "framework.h"
#include "SnapPreviewWnd.h"
#include <algorithm>

#undef min
#undef max

CSnapPreviewWnd::CSnapPreviewWnd()
{
}

CSnapPreviewWnd::~CSnapPreviewWnd()
{
}

void CSnapPreviewWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;

	CRect rect;
	rect.SetRectEmpty();

	m_bLayered = GetGlobalData()->m_nBitsPerPixel > 8;
	DWORD dwExStyle = m_bLayered ? WS_EX_LAYERED : 0;

	CreateEx(dwExStyle, AfxRegisterWndClass(0), _T(""), WS_POPUP, rect, pWndOwner, NULL);

	if (m_bLayered)
	{
		SetLayeredWindowAttributes(0, 100, LWA_ALPHA);
	}
}

void CSnapPreviewWnd::StartSnapping()
{
	ASSERT(m_pWndOwner && !IsWindowVisible());
	m_pWndOwner->GetWindowRect(&m_rcOwner);
}

void CSnapPreviewWnd::StopSnapping()
{
	ShowWindow(SW_HIDE);
}

void CSnapPreviewWnd::ShowAt(CWnd* pActiveSnapWnd, const CRect& rect)
{
	if (ShouldDoAnimation())
	{
		ASSERT(!m_pActiveSnapWnd || m_pActiveSnapWnd == pActiveSnapWnd);
		m_pActiveSnapWnd = pActiveSnapWnd;
		if (IsWindowVisible())
		{
			m_rectFrom = m_rectCur;
		}
		else
		{
			GetWindowInOwnerRect(pActiveSnapWnd, m_rectFrom);
			m_rectCur = m_rectFrom;
		}
		m_bHiding = false;
		m_rectTo = rect;
		ScheduleAnimation();
	}
	else
	{
		RepositionWindow(rect);
	}
}

void CSnapPreviewWnd::Hide()
{
	if (ShouldDoAnimation())
	{
		m_bHiding = true;
		m_rectFrom = m_rectCur;
		ScheduleAnimation();
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

void CSnapPreviewWnd::EnableAnimation(bool val)
{
	m_bEnableAnimation = val;
}

bool CSnapPreviewWnd::IsAnimationEnabled() const
{
	return m_bEnableAnimation;
}

void CSnapPreviewWnd::GetSnapRect(CRect& rect) const
{
	GetWindowRect(rect);
}

void CSnapPreviewWnd::RepositionWindow(const CRect& rect)
{
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(),
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);
	RedrawWindow();
}

void CSnapPreviewWnd::OnAnimationTo(const CRect& rect, bool bFinish)
{
	m_rectCur = rect;
	RepositionWindow(rect);
	if (bFinish)
	{
		if (m_bHiding)
		{
			ShowWindow(SW_HIDE);
			m_pActiveSnapWnd = nullptr;
		}
	}
}

void CSnapPreviewWnd::GetWindowInOwnerRect(CWnd* pWnd, CRect& rect) const
{
	pWnd->GetWindowRect(rect);
	CRect rcOwner;
	m_pWndOwner->GetWindowRect(rcOwner);
	rect.left = std::max(rect.left, rcOwner.left);
	rect.top = std::max(rect.top, rcOwner.top);
	rect.right = std::min(rect.right, rcOwner.right);
	rect.bottom = std::min(rect.bottom, rcOwner.bottom);
}

void CSnapPreviewWnd::StopAnimation()
{
	if (m_nTimerID)
	{
		KillTimer(m_nTimerID);
		m_nTimerID = 0;
	}
}

enum
{	AnimationInterval = 10
};

void CSnapPreviewWnd::ScheduleAnimation()
{
	m_AniStartTime = std::chrono::steady_clock::now();
	m_nTimerID = SetTimer(100, AnimationInterval, nullptr);
}

bool CSnapPreviewWnd::ShouldDoAnimation() const
{
	if (!m_bEnableAnimation)
		return false;
	return m_bLayered;
}


BEGIN_MESSAGE_MAP(CSnapPreviewWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapPreviewWnd message handlers

void CSnapPreviewWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);

	COLORREF colorFill = RGB(47, 103, 190);

	if (m_bLayered)
	{
		CBrush brFill(CDrawingManager::PixelAlpha(colorFill, 105));
		//dc.FillRect(rect, &brFill);
		int width = GetSystemMetrics(SM_CXVSCROLL);
		CSize sz(width, width);
		dc.DPtoLP(&sz);
		CPen pen(PS_SOLID, sz.cx, RGB(0xcc, 0xcc, 0xcc));
		auto oldBrush = dc.SelectObject(brFill.GetSafeHandle());
		auto oldPen = dc.SelectObject(pen.GetSafeHandle());
		dc.Rectangle(rect);

		dc.SelectObject(oldBrush);
		dc.SelectObject(oldPen);
	}
	else
	{
		CBrush brFill(CDrawingManager::PixelAlpha(RGB(255 - GetRValue(colorFill), 255 - GetGValue(colorFill), 255 - GetBValue(colorFill)), 50));

		CBrush* pBrushOld = dc.SelectObject(&brFill);
		dc.PatBlt(0, 0, rect.Width(), rect.Height(), PATINVERT);
		dc.SelectObject(pBrushOld);
	}
}

BOOL CSnapPreviewWnd::OnEraseBkgnd(CDC* /*pDC*/)
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

inline static LONG CalcSmoothPos(double pos, LONG from, LONG to)
{
	if (from == to || pos > 1.)
		return to;
	auto newPos = from * (1 - SmoothMoveELX(pos))
		+ to * SmoothMoveELX(pos);
	return (LONG)newPos;
}

void CSnapPreviewWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != m_nTimerID)
		return;
	auto curTime = std::chrono::steady_clock::now();
	std::chrono::duration<double> timeDiff = curTime - m_AniStartTime;
	auto timeDiffCount = timeDiff.count();

	constexpr double duration = 0.2;

	if (m_bHiding)
	{
		GetWindowInOwnerRect(m_pActiveSnapWnd, m_rectTo);
	}

	CRect rect;
	bool bFinish = timeDiffCount >= duration;
	if (bFinish)
	{
		rect = m_rectTo;
	}
	else
	{
		auto dPos = (double)timeDiffCount / duration;
		rect.left = CalcSmoothPos(dPos, m_rectFrom.left, m_rectTo.left);
		rect.top = CalcSmoothPos(dPos, m_rectFrom.top, m_rectTo.top);
		rect.right = CalcSmoothPos(dPos, m_rectFrom.right, m_rectTo.right);
		rect.bottom = CalcSmoothPos(dPos, m_rectFrom.bottom, m_rectTo.bottom);
	}
	OnAnimationTo(rect, bFinish);
	if (bFinish)
		StopAnimation();
}

void CSnapPreviewWnd::OnDestroy()
{
	StopAnimation();
	__super::OnDestroy();
}

