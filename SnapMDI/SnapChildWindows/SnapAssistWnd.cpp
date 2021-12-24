#include SW_PCH_FNAME
#include "SnapAssistWnd.h"
#include "SnapRenderImp.h"
#include "SnapWindowManager.h"

namespace SnapChildWindows {

class CSnapAssistRenderImpDirectComposition : public CSnapRenderImpBaseDirectComposition
{
public:
	BOOL HandlePaint() override
	{
		try
		{
			CreateDeviceResources();

			ComPtr<ID2D1DeviceContext> dc;
			POINT offset = {};
			HR(m_surface->BeginDraw(nullptr, // Entire surface
				__uuidof(dc),
				reinterpret_cast<void**>(dc.GetAddressOf()),
				&offset));

			dc->SetDpi(m_dpi.x, m_dpi.y);

			dc->SetTransform(Matrix3x2F::Translation(DPtoLP(offset.x, m_dpi.x),
				DPtoLP(offset.y, m_dpi.y)));

			dc->Clear();

			{
				//struct PaintParams
				//{
				//	CSnapAssistRenderImpDirectComposition* pThis;
				//	ID2D1DeviceContext* pDC;
				//};

				//auto pPreviweWnd = (CSnapAssistWnd*)m_pWnd;
				//auto locPaintRect = [](RECT rect, LPARAM lParam) {
				//	auto params = *(PaintParams*)lParam;
				//	params.pThis->m_pWnd->ScreenToClient(&rect);
				//	params.pThis->PaintSnapRect(params.pDC, rect);
				//};
				//PaintParams params = { this, dc.Get() };
				//pPreviweWnd->EnumDivideWindows(locPaintRect, (LPARAM)&params);
			}

			HR(m_surface->EndDraw());
			HR(m_device->Commit());
			m_pWnd->ValidateRect(nullptr);
		}
		catch (ComException const&)
		{
			ReleaseDeviceResources();
		}
		return TRUE;
	}
};

class CSnapAssistRenderImpAlpha : public CSnapRenderImpBaseAlpha
{
public:
	void OnAnimationUpdate() override
	{
		if (!m_bmp.GetSafeHandle())
			return;
		CRect rectClient;
		m_pWnd->GetClientRect(rectClient);

		CPoint point(0, 0);
		CSize size(rectClient.Size());
		ASSERT(size == m_szBmp);

		CClientDC clientDC(m_pWnd);
		CDC dc;
		dc.CreateCompatibleDC(&clientDC);

		CBitmap* pBitmapOld = (CBitmap*)dc.SelectObject(&m_bmp);
		ZeroMemory(m_pBits, size.cx * size.cy * 4);

		{
			//Gdiplus::Graphics gg(dc.GetSafeHdc());

			//struct PaintParams 
			//{
			//	CSnapAssistRenderImpAlpha* pThis;
			//	Gdiplus::Graphics* pGraphics;
			//};

			//auto pPreviweWnd = (CSnapAssistWnd*)m_pWnd;
			//auto locPaintRect = [](RECT rect, LPARAM lParam) {
			//	auto params = *(PaintParams*)lParam;
			//	params.pThis->m_pWnd->ScreenToClient(&rect);
			//	params.pThis->PaintSnapRect(*params.pGraphics, rect);
			//};
			//PaintParams params = {this, &gg};
			//pPreviweWnd->EnumDivideWindows(locPaintRect, (LPARAM)&params);
		}

		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = LWA_COLORKEY;

		m_pWnd->UpdateLayeredWindow(NULL, 0, &size, &dc, &point, 0, &bf, ULW_ALPHA);

		dc.SelectObject(pBitmapOld);

		if (!m_pWnd->IsWindowVisible())
		{
			m_pWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
				SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		}
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSnapAssistWnd::CSnapAssistWnd(CSnapWindowManager* pManager)
{
	m_pManager = pManager;
}

CSnapAssistWnd::~CSnapAssistWnd()
{
	DestroyWindow();
}

BOOL CSnapAssistWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;
	switch (m_tech)
	{
	case RenderTech::DirectComposition:
		if (CLayeredAnimationWndRenderImpDirectComposition::IsApplicable())
		{
			m_renderImp = std::make_shared<CSnapAssistRenderImpDirectComposition>();
			break;
		}
		// fall through
	case RenderTech::AlphaBlendedLayer:
		if (CLayeredAnimationWndRenderImpAlpha::IsApplicable())
		{
			m_tech = RenderTech::AlphaBlendedLayer;
			m_renderImp = std::make_shared<CSnapAssistRenderImpAlpha>();
			break;
		}
		// fall through
	default:
		ASSERT(0);
		return FALSE;
	}
	ASSERT(m_renderImp);
	m_renderImp->AttachToLayeredAnimationWnd(this);

	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);

	if (!m_renderImp->Create(pWndOwner, &m_rcOwner))
		return FALSE;
	return TRUE;
}

void CSnapAssistWnd::Show()
{

}

void CSnapAssistWnd::Hide(bool bStopNow)
{

}

BOOL CSnapAssistWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TOOLWINDOW;
	return __super::PreCreateWindow(cs);
}


}