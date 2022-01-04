#include SW_PCH_FNAME
#include "SnapPreviewWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include "SnapRenderImp.h"

namespace SnapChildWindows {

constexpr D2D1_COLOR_F SecondaryFillColor = { 0.87f, 0.26f, 0.56f, 0.58f };

class CSnapPreviewRenderImpDirectComposition : public CSnapRenderImpBaseDirectComposition
{
public:
	void OnAnimationUpdate() override
	{
		GetRect(m_rect, RectType::CurTarget);
		m_pWnd->ScreenToClient(&m_rect);

		auto pWnd = (CSnapPreviewWnd*)m_pWnd;
		if (pWnd->IsShowSecondary())
		{
			m_rectSecond = pWnd->GetSecondaryRect();
			m_pWnd->ScreenToClient(&m_rectSecond);
		}
		else
		{
			m_rectSecond.SetRectEmpty();
		}

		__super::OnAnimationUpdate();
	}

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

			PaintSnapRect(dc.Get(), m_rect);

			if (!m_rectSecond.IsRectEmpty())
			{
				SnapVisualSettings settings;
				GetVisualSettings(settings);
				settings.fill = SecondaryFillColor;
				PaintSnapRect(dc.Get(), m_rectSecond, &settings);
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
	CRect	m_rect;
	CRect	m_rectSecond;
};

class CSnapPreviewRenderImpAlpha : public CSnapRenderImpBaseAlpha
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

		CRect rect;
		GetRect(rect, RectType::CurTarget);
		m_pWnd->ScreenToClient(rect);
		ZeroMemory(m_pBits, size.cx * size.cy * 4);

		{
			Gdiplus::Graphics gg(dc.GetSafeHdc());
			PaintSnapRect(gg, rect);

			auto pWnd = (CSnapPreviewWnd*)m_pWnd;
			if (pWnd->IsShowSecondary())
			{
				CRect rectSecond;
				rectSecond = pWnd->GetSecondaryRect();
				m_pWnd->ScreenToClient(&rectSecond);

				SnapVisualSettings settings;
				GetVisualSettings(settings);
				settings.fill = SecondaryFillColor;
				PaintSnapRect(gg, rectSecond, &settings);
			}
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
		CPaintDC dc(m_pWnd);

		CRect rect;
		m_pWnd->GetClientRect(rect);

		COLORREF crfFill = RGB(66, 143, 222);

		CBrush brFill(crfFill);
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
		return TRUE;
	}
};

CSnapPreviewWnd::CSnapPreviewWnd()
{
	
}

void CSnapPreviewWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;
	switch (m_tech)
	{
	case RenderTech::DirectComposition:
		//if (CLayeredAnimationWndRenderImpDirectComposition::IsApplicable())
		//{
		//	m_renderImp = std::make_shared<CSnapPreviewRenderImpDirectComposition>();
		//	break;
		//}
		// fall through
	case RenderTech::AlphaBlendedLayer:
		if (CLayeredAnimationWndRenderImpAlpha::IsApplicable())
		{
			m_tech = RenderTech::AlphaBlendedLayer;
			m_renderImp = std::make_shared<CSnapPreviewRenderImpAlpha>();
			break;
		}
	case RenderTech::InvertColor:
		// fall through
	default:
		m_tech = RenderTech::InvertColor;
		m_renderImp = std::make_shared<CLayeredAnimationWndRenderImpInvert>();
		break;
	}
	ASSERT(m_renderImp);
	m_renderImp->AttachToLayeredAnimationWnd(this);
	m_renderImp->Create(pWndOwner);
}

void CSnapPreviewWnd::StartSnapping()
{
	ASSERT(m_pWndOwner && !IsWindowVisible());
	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);
	if (m_renderImp)
	{
		m_renderImp->StartRendering();
	}
}

void CSnapPreviewWnd::StopSnapping()
{
	if (m_renderImp)
	{
		m_renderImp->StopRendering();
		m_aniStage = AnimateStage::Hiding;
		FinishAnimationCleanup();
		m_pActiveSnapWnd = nullptr;
	}
}

void CSnapPreviewWnd::ShowAt(CWnd* pActiveSnapWnd, const CRect& rect)
{
	m_rcTo = rect;
	GetWindowInOwnerRect(m_rcTo);
	ASSERT(!m_pActiveSnapWnd || m_pActiveSnapWnd == pActiveSnapWnd);
	m_pActiveSnapWnd = pActiveSnapWnd;
	if (ShouldDoAnimation())
	{
		if (IsWindowVisible())
		{
			m_rcFrom = m_rcCur;
		}
		else
		{
			GetWindowInOwnerRect(m_rcFrom, pActiveSnapWnd);
			m_rcCur = m_rcFrom;
		}
		m_aniStage = AnimateStage::Showing;
		ScheduleAnimation();
	}
	else
	{
		if (m_renderImp)
		{
			m_rcCur = rect;
			m_rcSecondaryCur = m_rcSecondaryTo;
			m_renderImp->OnAnimationUpdate();
		}
	}
}

void CSnapPreviewWnd::Hide()
{
	if (!IsWindowVisible())
	{
		if (!m_nTimerIDAni)
			return;
	}
	if (ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Hiding;
		m_rcFrom = m_rcCur;

		if (m_bShowSecondary)
		{
			std::swap(m_rcSecondaryFrom, m_rcSecondaryTo);
		}

		ScheduleAnimation();
	}
	else
	{
		// Must have no ongoing animation
		ASSERT(m_nTimerIDAni == 0);
		ShowWindow(SW_HIDE);
	}
}

void CSnapPreviewWnd::SetSecondaryRects(const RECT& rcFrom, const RECT& rcTo)
{
	m_rcSecondaryCur = rcFrom;
	m_rcSecondaryFrom = rcFrom;
	m_rcSecondaryTo = rcTo;
}

void CSnapPreviewWnd::UpdateAnimation(bool bFinish)
{
	if (m_renderImp)
	{
		m_renderImp->OnAnimationUpdate();
	}
	if (bFinish)
	{
		if (m_aniStage == AnimateStage::Hiding)
			m_pActiveSnapWnd = nullptr;
		FinishAnimationCleanup();
	}
}

constexpr double AnimationDuration = 0.15;

void CSnapPreviewWnd::OnAnimationTimer(double timeDiff)
{
	if (m_aniStage == AnimateStage::Hiding)
	{
		GetWindowInOwnerRect(m_rcTo, m_pActiveSnapWnd);
	}

	bool bFinish = timeDiff >= AnimationDuration;
	if (bFinish)
	{
		m_rcCur = m_rcTo;
		m_rcSecondaryCur = m_rcSecondaryTo;
	}
	else
	{
		auto dPos = timeDiff / AnimationDuration;
		m_rcCur = AnimateRect(dPos, m_rcFrom, m_rcTo);
		if (m_bShowSecondary)
			m_rcSecondaryCur = AnimateRect(dPos, m_rcSecondaryFrom, m_rcSecondaryTo);
	}

	UpdateAnimation(bFinish);
}


}