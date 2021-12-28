#pragma once

#include "LayeredAnimationWndRenderImpEx.h"

namespace SnapChildWindows {

struct SnapVisualSettings 
{
	D2D1_COLOR_F	fill;
	D2D1_COLOR_F	edge;
};

class CSnapRenderImpBaseDirectComposition : public CLayeredAnimationWndRenderImpDirectComposition
{
public:
	void CreateDeviceResourcesEx(ID2D1DeviceContext* pDC) override;

	void OnAnimationUpdate() override;

	void PaintSnapRect(ID2D1DeviceContext* pDC, const CRect& rect, const SnapVisualSettings* pSettings = nullptr);
protected:
	virtual void GetVisualSettings(SnapVisualSettings& settings) const;
protected:
	ComPtr<ID2D1SolidColorBrush>	m_brush;
};

class CSnapRenderImpBaseAlpha : public CLayeredAnimationWndRenderImpAlpha
{
public:
	void PaintSnapRect(Gdiplus::Graphics& gg, CRect rect, const SnapVisualSettings* pSettings = nullptr);
protected:
	virtual void GetVisualSettings(SnapVisualSettings& settings) const;
};


}