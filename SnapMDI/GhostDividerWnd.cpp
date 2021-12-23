#include "pch.h"
#include "framework.h"
#include "GhostDividerWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include "SnapWindowManager.h"
#include <algorithm>

#undef min
#undef max

class CGhostDividerRenderImpAlpha : public CLayeredAnimationWndRenderImpAlpha
{
public:
	BOOL HandlePaint() override;

	void OnAnimationUpdate() override;

	bool NeedGDIPlus() const override { return false; }
};

BOOL CGhostDividerRenderImpAlpha::HandlePaint()
{
	CPaintDC dc(m_pWnd);

	CRect rect;
	m_pWnd->GetClientRect(rect);

	COLORREF crfFill = RGB(45, 100, 63);

	CBrush brFill(crfFill);

	auto oldBrush = dc.SelectObject(brFill.GetSafeHandle());
	auto oldPen = dc.SelectObject(GetStockObject(NULL_PEN));
	dc.Rectangle(rect);

	auto pDividerWnd = (CGhostDividerWnd*)m_pWnd;
	bool bVertical = pDividerWnd->IsVertical();

	auto drag = GetSystemMetrics(bVertical ? SM_CXDLGFRAME : SM_CYDLGFRAME);
	auto dragcx = drag;
	auto dragcy = GetSystemMetrics(bVertical ? SM_CXVSCROLL : SM_CYHSCROLL) * 2;
	if (!bVertical)
		std::swap(dragcx, dragcy);

	rect.left += (rect.Width() - dragcx) / 2;
	rect.top += (rect.Height() - dragcy) / 2;
	rect.right = rect.left + dragcx;
	rect.bottom = rect.top + dragcy;
	if (bVertical)
		++rect.right;
	else
		++rect.bottom;
	CBrush brDrag(RGB(255, 255, 255));
	dc.SelectObject(&brDrag);
	dc.RoundRect(rect, CPoint(drag, drag));

	dc.SelectObject(oldBrush);
	dc.SelectObject(oldPen);
	return TRUE;
}

void CGhostDividerRenderImpAlpha::OnAnimationUpdate()
{

}

CGhostDividerWnd::CGhostDividerWnd(CSnapWindowManager* pManager, bool bVertical)
{
	m_pManager = pManager;
	m_bVertical = bVertical;
}

CGhostDividerWnd::~CGhostDividerWnd()
{
	DestroyWindow();
}

BEGIN_MESSAGE_MAP(CGhostDividerWnd, CGhostDividerWndBase)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

void CGhostDividerWnd::Create(CWnd* pWndOwner, const POINT& pos, LONG length)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;
	m_tech = RenderTech::AlphaBlendedLayer;
	m_renderImp = std::make_shared<CGhostDividerRenderImpAlpha>();
	ASSERT(m_renderImp);
	m_renderImp->AttachToLayeredAnimationWnd(this);

	int cx = GetSystemMetrics(m_bVertical ? SM_CXVSCROLL : SM_CYHSCROLL);
	CSize szInflate(cx / 2+1, 0);
	CSize size(0, length);
	if (!m_bVertical)
	{
		std::swap(szInflate.cx, szInflate.cy);
		std::swap(size.cx, size.cy);
	}
	CRect rect(pos, size);
	rect.InflateRect(szInflate);

	m_renderImp->Create(pWndOwner, &rect);

	SetLayeredWindowAttributes(0, ShouldDoAnimation() ? 0 : 255, LWA_ALPHA);
}

BOOL CGhostDividerWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.dwExStyle |= WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
	return __super::PreCreateWindow(cs);
}

void CGhostDividerWnd::Show()
{
	UINT nFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER;
	SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0, nFlags);
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

void CGhostDividerWnd::OnShowWindow(BOOL bShow, UINT nStatus)
{
	__super::OnShowWindow(bShow, nStatus);
	// nStatus: It is 0 if the message is sent because of a ShowWindow member function call;
	if (!bShow && !nStatus)
	{
		m_pManager->OnGhostDividerWndHidden(this);
	}
}
