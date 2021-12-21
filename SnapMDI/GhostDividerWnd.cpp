#include "pch.h"
#include "framework.h"
#include "GhostDividerWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include <algorithm>

#undef min
#undef max

class CGhostDividerRenderImpAlpha : public CLayeredAnimationWndRenderImpAlpha
{
public:
	void HandlePaint() override;

	void OnAnimationUpdate() override;

	bool m_bVertical = false;
};

void CGhostDividerRenderImpAlpha::HandlePaint()
{
	CPaintDC dc(m_pWnd);

	CRect rect;
	m_pWnd->GetClientRect(rect);

	COLORREF crfFill = RGB(66, 143, 222);

	CBrush brFill(crfFill);

	auto oldBrush = dc.SelectObject(brFill.GetSafeHandle());
	auto oldPen = dc.SelectObject(GetStockObject(NULL_PEN));
	dc.Rectangle(rect);

	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);

}

void CGhostDividerRenderImpAlpha::OnAnimationUpdate()
{

}

CGhostDividerWnd::CGhostDividerWnd()
{

}

void CGhostDividerWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;
	m_tech = RenderTech::AlphaBlendedLayer;
	m_renderImp = std::make_shared<CGhostDividerRenderImpAlpha>();
	ASSERT(m_renderImp);
	m_renderImp->AttachToLayeredAnimationWnd(this);
	m_renderImp->Create(pWndOwner);
	SetLayeredWindowAttributes(0, 0, LWA_ALPHA);
}

BOOL CGhostDividerWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
	return __super::PreCreateWindow(cs);
}

void CGhostDividerWnd::Show(const CRect& rect, bool bVertical)
{
	UINT nFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW;
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(), nFlags);
	if (ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Showing;
		ScheduleAnimation();
	}
}

void CGhostDividerWnd::Hide()
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

constexpr double AnimationDuration = 0.4;

void CGhostDividerWnd::OnAnimationTimer(double timeDiff)
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
	SetLayeredWindowAttributes(0, m_byAlpha, LWA_ALPHA);
	if (bFinish)
	{
		FinishAnimationCleanup();
	}
}
