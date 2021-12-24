#include SW_PCH_FNAME
#include "SnapRenderImp.h"
#include "LayeredAnimationWnd.h"

namespace SnapChildWindows {

static inline void GetDefaultVisualSettings(SnapVisualSettings& settings)
{
	settings.fill = { 0.26f, 0.56f, 0.87f, 0.58f };
	settings.edge = { 0.8f, 0.8f, 0.8f, 0.78f };
}

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

	SnapVisualSettings settings;
	GetVisualSettings(settings);

	m_brush->SetColor(settings.edge);
	//pDC->FillRectangle(rect, m_brush.Get());
	pDC->FillRoundedRectangle(rRect, m_brush.Get());

	m_brush->SetColor(settings.fill);

	rRect.rect.left += gap;
	rRect.rect.top += gap;
	rRect.rect.right -= gap;
	rRect.rect.bottom -= gap;
	pDC->FillRoundedRectangle(rRect, m_brush.Get());
}

void CSnapRenderImpBaseDirectComposition::GetVisualSettings(SnapVisualSettings& settings) const
{
	GetDefaultVisualSettings(settings);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CSnapRenderImpBaseAlpha::PaintSnapRect(Gdiplus::Graphics& gg, CRect rect)
{
	COLORREF crfFill = RGB(66, 143, 222);

	SnapVisualSettings settings;
	GetVisualSettings(settings);

	Gdiplus::Color colorEdge((BYTE)(settings.edge.a * 255), 
		(BYTE)(settings.edge.r * 255), 
		(BYTE)(settings.edge.g * 255), 
		(BYTE)(settings.edge.b * 255));

	Gdiplus::SolidBrush brush(colorEdge);

	Gdiplus::GraphicsPath path;
	int diameter = GetSystemMetrics(SM_CXVSCROLL);
	UpdateRoundedRectPath(path, rect, diameter);

	gg.FillPath(&brush, &path);

	path.Reset();

	Gdiplus::Color colorFill((BYTE)(settings.fill.a * 255),
		(BYTE)(settings.fill.r * 255),
		(BYTE)(settings.fill.g * 255),
		(BYTE)(settings.fill.b * 255));
	brush.SetColor(colorFill);
	rect.DeflateRect(diameter / 2, diameter / 2);
	UpdateRoundedRectPath(path, rect, diameter);
	gg.FillPath(&brush, &path);

}

void CSnapRenderImpBaseAlpha::GetVisualSettings(SnapVisualSettings& settings) const
{
	GetDefaultVisualSettings(settings);
}

}