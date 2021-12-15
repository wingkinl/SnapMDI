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

void CSnapPreviewWnd::ShowAt(CWnd* pWnd, const CRect& rect)
{
	if (m_bEnableAnimation)
	{
		CRect rectFrom;
		LPCRECT pRectFrom = nullptr;
		if (false)
		{
			GetWindowInOwnerRect(pWnd, rectFrom);
			pRectFrom = &rectFrom;
		}
		m_bHiding = false;
	}
	else
	{
		RepositionWindow(rect);
	}
}

void CSnapPreviewWnd::Hide(CWnd* pWnd)
{
	if (m_bEnableAnimation)
	{
		CRect rect;
		GetWindowInOwnerRect(pWnd, rect);
		m_bHiding = true;
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

void CSnapPreviewWnd::RepositionWindow(const RECT& rect)
{
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);
	RedrawWindow();
}

void CSnapPreviewWnd::OnAnimationFinished()
{
	if (m_bHiding)
	{
		ShowWindow(SW_HIDE);
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

bool CSnapPreviewWnd::ShouldDoAnimation() const
{
	if (!m_bEnableAnimation)
		return false;
	return m_bLayered;
}


BEGIN_MESSAGE_MAP(CSnapPreviewWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
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

