#include "pch.h"
#include "framework.h"
#include "DividePreviewWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include "SnapRenderImp.h"
#include "SnapWindowManager.h"

#undef min
#undef max

class CDividePreviewRenderImpDirectComposition : public CSnapRenderImpBaseDirectComposition
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
					CDividePreviewRenderImpDirectComposition* pThis;
					ID2D1DeviceContext* pDC;
				};

				auto pPreviweWnd = (CDividePreviewWnd*)m_pWnd;
				auto locPaintRect = [](RECT rect, LPARAM lParam) {
					auto params = *(PaintParams*)lParam;
					params.pThis->m_pWnd->ScreenToClient(&rect);
					params.pThis->PaintSnapRect(params.pDC, rect);
				};
				PaintParams params = { this, dc.Get() };
				pPreviweWnd->EnumDivideWindows(locPaintRect, (LPARAM)&params);
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
private:
};

class CDividePreviewRenderImpAlpha : public CSnapRenderImpBaseAlpha
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
				CDividePreviewRenderImpAlpha* pThis;
				Gdiplus::Graphics* pGraphics;
			};

			auto pPreviweWnd = (CDividePreviewWnd*)m_pWnd;
			auto locPaintRect = [](RECT rect, LPARAM lParam) {
				auto params = *(PaintParams*)lParam;
				params.pThis->m_pWnd->ScreenToClient(&rect);
				params.pThis->PaintSnapRect(*params.pGraphics, rect);
			};
			PaintParams params = {this, &gg};
			pPreviweWnd->EnumDivideWindows(locPaintRect, (LPARAM)&params);
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

	BOOL HandlePaint() override
	{
		return FALSE;
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CDividePreviewWnd::CDividePreviewWnd(CSnapWindowManager* pManager)
{
	m_pManager = pManager;
}

BOOL CDividePreviewWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;
	switch (m_tech)
	{
	case RenderTech::DirectComposition:
		if (CLayeredAnimationWndRenderImpDirectComposition::IsApplicable())
		{
			m_renderImp = std::make_shared<CDividePreviewRenderImpDirectComposition>();
			break;
		}
		// fall through
	case RenderTech::AlphaBlendedLayer:
		if (CLayeredAnimationWndRenderImpAlpha::IsApplicable())
		{
			m_tech = RenderTech::AlphaBlendedLayer;
			m_renderImp = std::make_shared<CDividePreviewRenderImpAlpha>();
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

void CDividePreviewWnd::Show()
{
	ASSERT(m_pWndOwner);
	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);
	if (m_renderImp)
	{
		m_renderImp->StartRendering();
	}

	UINT nFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
	SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, nFlags);

	if (ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Showing;
		ScheduleAnimation();
	}
	else
	{
		if (m_renderImp)
		{
			m_renderImp->OnAnimationUpdate();
		}
		SetLayeredWindowAttributes(0, 255, LWA_ALPHA);
	}
}

void CDividePreviewWnd::Hide()
{
	if (!IsWindowVisible())
	{
		if (!m_nTimerIDAni)
			return;
	}
	if (ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Hiding;
		ScheduleAnimation();
	}
	else
	{
		// Must have no ongoing animation
		ASSERT(m_nTimerIDAni == 0);
		ShowWindow(SW_HIDE);
	}
}

void CDividePreviewWnd::UpdateDivideWindows()
{
	if (m_renderImp)
	{
		m_renderImp->OnAnimationUpdate();
	}
}

void CDividePreviewWnd::EnumDivideWindows(EnumDivideWindowsProc pProc, LPARAM lParam) const
{
	auto& vChildRects = m_pManager->m_vChildRects;
	for (int ii = 0; ii < (int)vChildRects.size(); ++ii)
	{
		if (ii != m_pManager->m_nCurSnapWndIdx)
			pProc(vChildRects[ii].rect, lParam);
	}
}

BOOL CDividePreviewWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
	return __super::PreCreateWindow(cs);
}

constexpr double AnimationDuration = 0.4;

void CDividePreviewWnd::OnAnimationTimer(double timeDiff)
{
	LONG alpha = 0;
	LONG alphaFrom = m_byAlpha, alphaTo = 255;
	if (m_aniStage == AnimateStage::Hiding)
	{
		alphaTo = 0;
	}
	bool bFinish = timeDiff >= AnimationDuration;
	if (bFinish)
	{
		alpha = alphaTo;
	}
	else
	{
		auto dPos = timeDiff / AnimationDuration;
		alpha = CalcSmoothPos(dPos, alphaFrom, alphaTo);
		alpha = std::max(alpha, 0L);
		alpha = std::min(alpha, 255L);
	}
	m_byAlpha = (BYTE)alpha;

	if (m_renderImp)
	{
		m_renderImp->OnAnimationUpdate();
	}
	SetLayeredWindowAttributes(0, m_byAlpha, LWA_ALPHA);
	if (bFinish)
	{
		FinishAnimationCleanup();
	}
}
