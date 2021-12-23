#include "pch.h"
#include "framework.h"
#include "DividePreviewWnd.h"
#include "LayeredAnimationWndRenderImpEx.h"

#undef min
#undef max

CDividePreviewWnd::CDividePreviewWnd()
{

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
			//m_renderImp = std::make_shared<CSnapPreviewRenderImpDirectComposition>();
			break;
		}
		// fall through
	case RenderTech::AlphaBlendedLayer:
		if (CLayeredAnimationWndRenderImpAlpha::IsApplicable())
		{
			m_tech = RenderTech::AlphaBlendedLayer;
			//m_renderImp = std::make_shared<CSnapPreviewRenderImpAlpha>();
			break;
		}
		// fall through
	default:
		ASSERT(0);
		return FALSE;
	}
	ASSERT(m_renderImp);
	m_renderImp->AttachToLayeredAnimationWnd(this);
	m_renderImp->Create(pWndOwner);
}

void CDividePreviewWnd::Show()
{

}

void CDividePreviewWnd::Hide()
{

}
