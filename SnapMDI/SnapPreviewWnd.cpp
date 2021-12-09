#include "pch.h"
#include "framework.h"
#include "SnapPreviewWnd.h"

CSnapPreviewWnd::CSnapPreviewWnd()
	: m_pWndOwner(NULL)
{
}

CSnapPreviewWnd::~CSnapPreviewWnd()
{
}

void CSnapPreviewWnd::Create(CWnd* pwndOwner)
{
	ASSERT_VALID(pwndOwner);

	m_pWndOwner = pwndOwner;

	CRect rect;
	rect.SetRectEmpty();

	DWORD dwExStyle = (GetGlobalData()->m_nBitsPerPixel > 8) ? WS_EX_LAYERED : 0;

	CreateEx(dwExStyle, AfxRegisterWndClass(0), _T(""), WS_POPUP, rect, pwndOwner, NULL);

	if (dwExStyle == WS_EX_LAYERED)
	{
		SetLayeredWindowAttributes(0, 100, LWA_ALPHA);
	}
}

void CSnapPreviewWnd::ShowAt(CRect rect)
{
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);

	RedrawWindow();
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

	if (GetGlobalData()->m_nBitsPerPixel > 8)
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
