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
				auto locPaintRect = [](RECT rect, LPARAM lParam) {
					auto params = *(PaintParams*)lParam;
					params.pThis->m_pWnd->ScreenToClient(&rect);
					params.pThis->PaintSnapRect(params.pDC, rect);
				};
				PaintParams params = { this, dc.Get() };
				pWnd->EnumLayoutCellWindows(locPaintRect, (LPARAM)&params);
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

			auto pWnd = (CSnapAssistWnd*)m_pWnd;
			auto locPaintRect = [](RECT rect, LPARAM lParam) {
				auto params = *(PaintParams*)lParam;
				params.pThis->m_pWnd->ScreenToClient(&rect);
				params.pThis->PaintSnapRect(*params.pGraphics, rect);
			};
			PaintParams params = {this, &gg};
			pWnd->EnumLayoutCellWindows(locPaintRect, (LPARAM)&params);
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

	UINT nFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
	SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, nFlags);

	if (!m_snapGridsAni.wnds.empty() && ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Showing;
		m_bShowLayoutCell = false;
		ScheduleAnimation();
	}
	else
	{
		m_bShowLayoutCell = true;
		m_renderImp->OnAnimationUpdate();
	}
}

void CSnapAssistWnd::EnumLayoutCellWindows(EnumLayoutCellWindowsProc pProc, LPARAM lParam) const
{
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

constexpr double AnimationDuration = 0.4;

void CSnapAssistWnd::OnAnimationTimer(double timeDiff)
{
	float factor = 0.f;
	float from = m_factor;
	bool bFinish = timeDiff >= AnimationDuration;
	if (bFinish)
	{
		factor = 1.0f;
	}
	else
	{
		auto dPos = timeDiff / AnimationDuration;
		factor = (float)CalcSmoothPosF(dPos, from, 1.0f);
		factor = std::max(factor, 0.0f);
		factor = std::min(factor, 1.0f);
	}
	m_factor = factor;

	if (m_renderImp)
	{
		m_renderImp->OnAnimationUpdate();
	}
	if (bFinish)
	{
		m_bShowLayoutCell = true;
		FinishAnimationCleanup();
	}
}

}