#pragma once

#include "LayeredAnimationWndRenderImpEx.h"

class CSnapRenderImpBaseDirectComposition : public CLayeredAnimationWndRenderImpDirectComposition
{
public:
	void CreateDeviceResourcesEx(ID2D1DeviceContext* pDC) override;

	void OnAnimationUpdate() override;

	void PaintSnapRect(ID2D1DeviceContext* pDC, const CRect& rect);
protected:
	ComPtr<ID2D1SolidColorBrush>	m_brush;
};

class CSnapRenderImpBaseAlpha : public CLayeredAnimationWndRenderImpAlpha
{
public:
	void PaintSnapRect(Gdiplus::Graphics& gg, CRect rect);
};
