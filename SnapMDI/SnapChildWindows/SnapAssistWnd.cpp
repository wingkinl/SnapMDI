#include SW_PCH_FNAME
#include "SnapAssistWnd.h"
#include "SnapRenderImp.h"
#include "SnapWindowManager.h"

#undef min
#undef max

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
				struct PaintParams
				{
					CSnapAssistRenderImpDirectComposition* pThis;
					ID2D1DeviceContext* pDC;
				};

				auto pWnd = (CSnapAssistWnd*)m_pWnd;
				auto locPaintCell = [](RECT rect, LPARAM lParam) {
					auto params = *(PaintParams*)lParam;
					params.pThis->m_pWnd->ScreenToClient(&rect);
					params.pThis->PaintSnapRect(params.pDC, rect);
				};
				PaintParams params = { this, dc.Get() };
				pWnd->EnumLayoutCells(locPaintCell, (LPARAM)&params);
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
			Gdiplus::Graphics gg(dc.GetSafeHdc());

			struct PaintParams 
			{
				CSnapAssistRenderImpAlpha* pThis;
				Gdiplus::Graphics* pGraphics;
			};
			PaintParams params = { this, &gg };

			auto pWnd = (CSnapAssistWnd*)m_pWnd;

			auto locPaintCell = [](RECT rect, LPARAM lParam) {
				auto params = *(PaintParams*)lParam;
				params.pThis->m_pWnd->ScreenToClient(&rect);
				params.pThis->PaintSnapRect(*params.pGraphics, rect);
			};
			pWnd->EnumLayoutCells(locPaintCell, (LPARAM)&params);

			PaintSnapGridWnds(gg);
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

	void PaintSnapGridWnds(Gdiplus::Graphics& gg)
	{
		auto pWnd = (CSnapAssistWnd*)m_pWnd;
		auto& data = pWnd->GetSnapGridWndAniData();
		if (data.snapGridWndAni.empty())
			return;
		auto factor = pWnd->GetTransitionFactor();
		ASSERT(data.snapGridWndAni.size() == data.wndPos.wnds.size());
		for (size_t ii = 0; ii < data.snapGridWndAni.size(); ++ii)
		{
			auto& info = data.snapGridWndAni[ii];
			auto& wnd = data.wndPos.wnds[ii];
			PaintSnapGridWnd(gg, info, wnd, factor);
		}
	}

	void PaintSnapGridWnd(Gdiplus::Graphics& gg, const SnapGridWndAniInfo& info, const WindowPos& wnd, float factor)
	{
		auto pWnd = (CSnapAssistWnd*)m_pWnd;
		CRect rect;
		rect.left = (LONG)(info.rectOld.left + (wnd.rect.left - info.rectOld.left) * factor);
		rect.top = (LONG)(info.rectOld.top + (wnd.rect.top - info.rectOld.top) * factor);
		rect.right = (LONG)(info.rectOld.right + (wnd.rect.right - info.rectOld.right) * factor);
		rect.bottom = (LONG)(info.rectOld.bottom + (wnd.rect.bottom - info.rectOld.bottom) * factor);
		pWnd->ScreenToClient(&rect);
		//PaintSnapRect(gg, rect);
		Gdiplus::Bitmap bmp(info.hBmpWnd, nullptr);
		Gdiplus::ColorMatrix colorMat = {
			1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, factor, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f, 1.0f
		};
		Gdiplus::ImageAttributes imgAttr;
		imgAttr.SetColorMatrix(&colorMat, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
		Gdiplus::Rect rc(rect.left, rect.top, rect.Width(), rect.Height());
		gg.DrawImage(&bmp, rc, 0, 0, bmp.GetWidth(), bmp.GetHeight(), Gdiplus::UnitPixel, &imgAttr);
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
// 		if (CLayeredAnimationWndRenderImpDirectComposition::IsApplicable())
// 		{
// 			m_renderImp = std::make_shared<CSnapAssistRenderImpDirectComposition>();
// 			break;
// 		}
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
	if (!m_renderImp)
	{
		ASSERT(0);
		return;
	}
	ASSERT(m_pWndOwner);
	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);

	m_renderImp->StartRendering();

	bool bShowAnimationForInitialMove = !m_snapGridWndAni.wndPos.wnds.empty() && ShouldDoAnimation();

	UINT nFlags = SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE;
	if (m_snapLayoutWnds.wnds.empty())
		nFlags |= SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER;

	SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, nFlags);

	if (bShowAnimationForInitialMove)
	{
		m_aniStage = AnimateStage::Showing;
		m_bShowLayoutCell = false;
		PrepareForInitialMovingAnimation();
		m_factor = 0.0f;
		ScheduleAnimation();
	}
	else
	{
		m_bShowLayoutCell = true;
		m_factor = 1.0f;
		m_renderImp->OnAnimationUpdate();
	}
}

void CSnapAssistWnd::PrepareForInitialMovingAnimation()
{
	auto count = m_snapGridWndAni.wndPos.wnds.size();
	m_snapGridWndAni.snapGridWndAni.resize(count);

	CClientDC clientDC(this);
	CDC dc;
	dc.CreateCompatibleDC(&clientDC);

	for (size_t ii = 0; ii < count; ++ii)
	{
		auto& wnd = m_snapGridWndAni.wndPos.wnds[ii];
		auto& ani = m_snapGridWndAni.snapGridWndAni[ii];
		::GetWindowRect(wnd.hWnd, &ani.rectOld);
		
		auto& rcOld = (CRect&)ani.rectOld;
		CBitmap bmpWnd;
		bmpWnd.CreateCompatibleBitmap(&clientDC, rcOld.Width(), rcOld.Height());

		CBitmap* pBitmapOld = (CBitmap*)dc.SelectObject(&bmpWnd);

		::SendMessage(wnd.hWnd, WM_PRINT, (WPARAM)dc.GetSafeHdc(), 
			(LPARAM)(PRF_CLIENT | PRF_ERASEBKGND | PRF_CHILDREN | PRF_NONCLIENT));

		dc.SelectObject(pBitmapOld);

		ani.hBmpWnd = (HBITMAP)bmpWnd.Detach();
	}
}

void CSnapAssistWnd::EnumLayoutCells(EnumLayoutCellProc pProc, LPARAM lParam) const
{
	if (!m_bShowLayoutCell)
		return;
	for (size_t ii = 0; ii < m_snapLayoutWnds.layout.cells.size(); ++ii)
	{
		if (m_snapLayoutWnds.wnds[ii])
			continue;
		auto& cell = m_snapLayoutWnds.layout.cells[ii];
		pProc(cell.rect, lParam);
	}
}

BOOL CSnapAssistWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TOOLWINDOW;
	return __super::PreCreateWindow(cs);
}

constexpr double AnimationDuration = 0.12;

void CSnapAssistWnd::OnAnimationTimer(double timeDiff)
{
	constexpr float factor_from = 0.2f, factor_to = 0.8f;
	float factor = 0.f;
	float from = m_factor;
	bool bFinish = timeDiff >= AnimationDuration;
	if (bFinish)
	{
		// Now that we have finished the animation, we want full factor for the grids
		factor = 1.0f;
		m_bShowLayoutCell = true;
		m_snapGridWndAni.snapGridWndAni.clear();
	}
	else
	{
		auto dPos = timeDiff / AnimationDuration;
		//factor = (float)CalcSmoothPosF(dPos, from, factor_to);
		factor = (float)(factor_from + (factor_to - factor_from) * dPos);
		factor = std::max(factor, factor_from);
		factor = std::min(factor, factor_to);
	}
	m_factor = factor;

	m_renderImp->OnAnimationUpdate();
	if (bFinish)
	{
		FinishAnimationCleanup();
		// Make sure this is the last line, because the m_pManager would destroy this window!
		m_pManager->OnSnapAssistEvent(CSnapWindowManager::SnapAssistEvent::FinishAnimation);
	}
}

}