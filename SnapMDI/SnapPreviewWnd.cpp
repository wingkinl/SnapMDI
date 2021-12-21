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
		m_rect = ((CSnapPreviewWnd*)m_pALAWnd)->GetCurRect();
		m_pALAWnd->ScreenToClient(&m_rect);

		m_pALAWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		m_pALAWnd->RedrawWindow();
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

			m_brush->SetColor({ 0.8f, 0.8f, 0.8f, 0.5f });
			//dc->FillRectangle(rect, m_brush.Get());
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			m_brush->SetColor({ 0.26f, 0.56f, 0.87f, 0.5f });

			rRect.rect.left += gap;
			rRect.rect.top += gap;
			rRect.rect.right -= gap;
			rRect.rect.bottom -= gap;
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			HR(m_surface->EndDraw());
			HR(m_device->Commit());
			m_pALAWnd->ValidateRect(nullptr);
		}
		catch (ComException const&)
		{
			ReleaseDeviceResources();
		}
	}
private:
	RECT	m_rect;
	ComPtr<ID2D1SolidColorBrush>	m_brush;
};

class CSnapPreviewRenderImpAlpha : public CLayeredAnimationWndRenderImpAlpha
{
public:
	static BOOL IsApplicable()
	{
		if (GetGlobalData()->m_nBitsPerPixel > 8)
			return TRUE;
		return FALSE;
	}

	void OnAnimationUpdate() override
	{
		if (!m_bmp.GetSafeHandle())
			return;
		CRect rectClient;
		m_pALAWnd->GetClientRect(rectClient);

		CPoint point(0, 0);
		CSize size(rectClient.Size());
		ASSERT(size == m_szBmp);

		CClientDC clientDC(m_pALAWnd);
		CDC dc;
		dc.CreateCompatibleDC(&clientDC);

		CBitmap* pBitmapOld = (CBitmap*)dc.SelectObject(&m_bmp);

		CRect rect = ((CSnapPreviewWnd*)m_pALAWnd)->GetCurRect();
		m_pALAWnd->ScreenToClient(rect);
		ZeroMemory(m_pBits, size.cx * size.cy * 4);
		{
			COLORREF crfFill = RGB(66, 143, 222);

			Gdiplus::Graphics gg(dc.GetSafeHdc());
			Gdiplus::Color color(128, GetRValue(crfFill), GetGValue(crfFill), GetBValue(crfFill));
			Gdiplus::SolidBrush brush(Gdiplus::Color(128, 0xcc, 0xcc, 0xcc));
			Gdiplus::Rect rc(rect.left, rect.top, rect.Width(), rect.Height());

			Gdiplus::GraphicsPath path;

			int diameter = GetSystemMetrics(SM_CXVSCROLL);
			Gdiplus::Rect rcArc(rc.X, rc.Y, diameter, diameter);

			// top left arc  
			path.AddArc(rcArc, 180, 90);

			// top right arc  
			rcArc.X = rect.right - diameter;
			path.AddArc(rcArc, 270, 90);

			// bottom right arc  
			rcArc.Y = rect.bottom - diameter;
			path.AddArc(rcArc, 0, 90);

			// bottom left arc 
			rcArc.X = rect.left;
			path.AddArc(rcArc, 90, 90);

			path.CloseFigure();

			gg.FillPath(&brush, &path);

			brush.SetColor(color);
			rc.Inflate(-diameter / 2, -diameter / 2);
			gg.FillRectangle(&brush, rc);
		}

		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = LWA_COLORKEY;

		m_pALAWnd->UpdateLayeredWindow(NULL, 0, &size, &dc, &point, 0, &bf, ULW_ALPHA);

		dc.SelectObject(pBitmapOld);

		if (!m_pALAWnd->IsWindowVisible())
		{
			m_pALAWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
				SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		}
	}

	void HandlePaint() override
	{
		CPaintDC dc(m_pALAWnd);

		CRect rect;
		m_pALAWnd->GetClientRect(rect);

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

class CSnapPreviewRenderImpInvert : public CLayeredAnimationWndRenderImpInvert
{
public:
	void HandlePaint() override
	{
		CPaintDC dc(m_pALAWnd);

		CRect rect;
		m_pALAWnd->GetClientRect(rect);

		COLORREF colorFill = RGB(47, 103, 190);

		CBrush brFill(CDrawingManager::PixelAlpha(RGB(255 - GetRValue(colorFill), 255 - GetGValue(colorFill), 255 - GetBValue(colorFill)), 50));

		CBrush* pBrushOld = dc.SelectObject(&brFill);
		dc.PatBlt(0, 0, rect.Width(), rect.Height(), PATINVERT);
		dc.SelectObject(pBrushOld);
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
		if (CSnapPreviewRenderImpAlpha::IsApplicable())
		{
			m_tech = RenderTech::AlphaBlendedLayer;
			m_renderImp = std::make_shared<CSnapPreviewRenderImpAlpha>();
			break;
		}
	case RenderTech::InvertColor:
	default:
		// fall through
		m_tech = RenderTech::InvertColor;
		m_renderImp = std::make_shared<CSnapPreviewRenderImpInvert>();
		break;
	}
	ASSERT(m_renderImp);
	m_renderImp->AttachToALAWnd(this);
	m_renderImp->Create(pWndOwner);
}

void CSnapPreviewWnd::StartSnapping()
{
	ASSERT(m_pWndOwner && !IsWindowVisible());
	m_bStartedSnapping = true;
	m_pWndOwner->GetClientRect(&m_rcOwner);
	m_pWndOwner->ClientToScreen(&m_rcOwner);
	if (m_renderImp)
	{
		m_renderImp->StartRendering();
	}
}

void CSnapPreviewWnd::StopSnapping(bool bAbort)
{
	m_bStartedSnapping = false;
	if (m_renderImp)
	{
		m_renderImp->StopRendering(bAbort);
		m_aniStage = AnimateStage::Hiding;
		FinishAnimationCleanup();
	}
}

void CSnapPreviewWnd::ShowAt(CWnd* pActiveSnapWnd, const CRect& rect)
{
	m_rectTo = rect;
	GetWindowInOwnerRect(m_rectTo);
	if (ShouldDoAnimation())
	{
		ASSERT(!m_pActiveSnapWnd || m_pActiveSnapWnd == pActiveSnapWnd);
		m_pActiveSnapWnd = pActiveSnapWnd;
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
		RepositionWindow(rect);
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

