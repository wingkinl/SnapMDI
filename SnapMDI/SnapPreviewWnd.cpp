#include "pch.h"
#include "framework.h"
#include "SnapPreviewWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include "SnapRenderImp.h"

class CSnapPreviewRenderImpDirectComposition : public CSnapRenderImpBaseDirectComposition
{
public:
	void OnAnimationUpdate() override
	{
		GetRect(m_rect, RectType::CurTarget);
		m_pWnd->ScreenToClient(&m_rect);

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

		PaintSnapRect(dc, rect);

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
		if (CLayeredAnimationWndRenderImpDirectComposition::IsApplicable())
		{
			m_renderImp = std::make_shared<CSnapPreviewRenderImpDirectComposition>();
			break;
		}
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
	m_rectTo = rect;
	GetWindowInOwnerRect(m_rectTo);
	ASSERT(!m_pActiveSnapWnd || m_pActiveSnapWnd == pActiveSnapWnd);
	m_pActiveSnapWnd = pActiveSnapWnd;
	if (ShouldDoAnimation())
	{
		if (IsWindowVisible())
		{
			m_rectFrom = m_rectCur;
		}
		else
		{
			GetWindowInOwnerRect(m_rectFrom, pActiveSnapWnd);
			m_rectCur = m_rectFrom;
		}
		m_aniStage = AnimateStage::Showing;
		ScheduleAnimation();
	}
	else
	{
		if (m_renderImp)
		{
			m_rectCur = rect;
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
		m_rectFrom = m_rectCur;
		ScheduleAnimation();
	}
	else
	{
		// Must have no ongoing animation
		ASSERT(m_nTimerIDAni == 0);
		ShowWindow(SW_HIDE);
	}
}

void CSnapPreviewWnd::OnAnimationTo(const CRect& rect, bool bFinish)
{
	m_rectCur = rect;
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
		GetWindowInOwnerRect(m_rectTo, m_pActiveSnapWnd);
	}

	CRect rect;
	bool bFinish = timeDiff >= AnimationDuration;
	if (bFinish)
	{
		rect = m_rectTo;
	}
	else
	{
		auto dPos = timeDiff / AnimationDuration;
		rect = AnimateRect(dPos, m_rectFrom, m_rectTo);
	}
	OnAnimationTo(rect, bFinish);
}

