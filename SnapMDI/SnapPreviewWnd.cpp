#include "pch.h"
#include "framework.h"
#include "SnapPreviewWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"

class CSnapPreviewRenderImpDirectComposition : public CLayeredAnimationWndRenderImpDirectComposition
{
public:
	void CreateDeviceResourcesEx(ID2D1DeviceContext* pDC) override
	{
		D2D_COLOR_F const color = { 0.26f, 0.56f, 0.87f, 0.5f };
		HR(pDC->CreateSolidColorBrush(color,
			m_brush.ReleaseAndGetAddressOf()));
	}

	void OnAnimationUpdate() override
	{
		GetRect(m_rect, RectType::CurTarget);
		m_pWnd->ScreenToClient(&m_rect);

		m_pWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		m_pWnd->RedrawWindow();
	}

	void HandlePaint() override
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
			D2D_RECT_F rect;
			rect.left = DPtoLP(m_rect.left, m_dpi.x);
			rect.top = DPtoLP(m_rect.top, m_dpi.y);
			rect.right = DPtoLP(m_rect.right, m_dpi.y);
			rect.bottom = DPtoLP(m_rect.bottom, m_dpi.y);
			//dc->FillRectangle(rect, m_brush.Get());
			D2D1_ROUNDED_RECT rRect;
			rRect.rect = rect;
			int width = GetSystemMetrics(SM_CXVSCROLL) / 2;
			auto gap = DPtoLP(width, m_dpi.x);

			rRect.radiusX = 10;
			rRect.radiusY = 10;

			m_brush->SetColor({ 0.8f, 0.8f, 0.8f, 0.78f });
			//dc->FillRectangle(rect, m_brush.Get());
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			m_brush->SetColor({ 0.26f, 0.56f, 0.87f, 0.58f });

			rRect.rect.left += gap;
			rRect.rect.top += gap;
			rRect.rect.right -= gap;
			rRect.rect.bottom -= gap;
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			HR(m_surface->EndDraw());
			HR(m_device->Commit());
			m_pWnd->ValidateRect(nullptr);
		}
		catch (ComException const&)
		{
			ReleaseDeviceResources();
		}
	}
private:
	CRect	m_rect;
	ComPtr<ID2D1SolidColorBrush>	m_brush;
};

class CSnapPreviewRenderImpAlpha : public CLayeredAnimationWndRenderImpAlpha
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
			COLORREF crfFill = RGB(66, 143, 222);

			Gdiplus::Graphics gg(dc.GetSafeHdc());
			Gdiplus::Color color(150, GetRValue(crfFill), GetGValue(crfFill), GetBValue(crfFill));
			Gdiplus::SolidBrush brush(Gdiplus::Color(200, 0xcc, 0xcc, 0xcc));
			Gdiplus::Rect rc(rect.left, rect.top, rect.Width(), rect.Height());

			Gdiplus::GraphicsPath path;
			int diameter = GetSystemMetrics(SM_CXVSCROLL);
			UpdateRoundedRectPath(path, rect, diameter);

			gg.FillPath(&brush, &path);

			path.Reset();

			brush.SetColor(color);
			rect.DeflateRect(diameter/2, diameter/2);
			UpdateRoundedRectPath(path, rect, diameter);
			gg.FillPath(&brush, &path);
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

	void HandlePaint() override
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

void CSnapPreviewWnd::StopSnapping(bool bAbort)
{
	if (m_renderImp)
	{
		m_renderImp->StopRendering(bAbort);
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

