#include "pch.h"
#include "framework.h"
#include "SnapRenderImp.h"
#include "LayeredAnimationWnd.h"

void CSnapRenderImpBaseDirectComposition::CreateDeviceResourcesEx(ID2D1DeviceContext* pDC)
{
	D2D_COLOR_F const color = { 0.26f, 0.56f, 0.87f, 0.5f };
	HR(pDC->CreateSolidColorBrush(color,
		m_brush.ReleaseAndGetAddressOf()));
}

void CSnapRenderImpBaseDirectComposition::OnAnimationUpdate()
{
	m_pWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
	m_pWnd->RedrawWindow();
}

void CSnapRenderImpBaseDirectComposition::PaintSnapRect(ID2D1DeviceContext* pDC, const CRect& rect)
{
	D2D_RECT_F rectd2;
	rectd2.left = DPtoLP(rect.left, m_dpi.x);
	rectd2.top = DPtoLP(rect.top, m_dpi.y);
	rectd2.right = DPtoLP(rect.right, m_dpi.y);
	rectd2.bottom = DPtoLP(rect.bottom, m_dpi.y);
	//dc->FillRectangle(rect, m_brush.Get());
	D2D1_ROUNDED_RECT rRect;
	rRect.rect = rectd2;
	int width = GetSystemMetrics(SM_CXVSCROLL) / 2;
	auto gap = DPtoLP(width, m_dpi.x);

	rRect.radiusX = 10;
	rRect.radiusY = 10;

	m_brush->SetColor({ 0.8f, 0.8f, 0.8f, 0.78f });
	//pDC->FillRectangle(rect, m_brush.Get());
	pDC->FillRoundedRectangle(rRect, m_brush.Get());

	m_brush->SetColor({ 0.26f, 0.56f, 0.87f, 0.58f });

	rRect.rect.left += gap;
	rRect.rect.top += gap;
	rRect.rect.right -= gap;
	rRect.rect.bottom -= gap;
	pDC->FillRoundedRectangle(rRect, m_brush.Get());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CSnapRenderImpBaseAlpha::PaintSnapRect(Gdiplus::Graphics& gg, CRect rect)
{
	COLORREF crfFill = RGB(66, 143, 222);

	Gdiplus::Color color(150, GetRValue(crfFill), GetGValue(crfFill), GetBValue(crfFill));
	Gdiplus::SolidBrush brush(Gdiplus::Color(200, 0xcc, 0xcc, 0xcc));

	Gdiplus::GraphicsPath path;
	int diameter = GetSystemMetrics(SM_CXVSCROLL);
	UpdateRoundedRectPath(path, rect, diameter);

	gg.FillPath(&brush, &path);

	path.Reset();

	brush.SetColor(color);
	rect.DeflateRect(diameter / 2, diameter / 2);
	UpdateRoundedRectPath(path, rect, diameter);
	gg.FillPath(&brush, &path);

}
