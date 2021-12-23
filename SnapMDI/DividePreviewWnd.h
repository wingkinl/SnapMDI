#pragma once

#include "LayeredAnimationWnd.h"

using CDividePreviewWndBase = CLayeredAnimationWnd;

struct DivideWndInfo 
{

};

class CDividePreviewWnd : public CDividePreviewWndBase
{
public:
	CDividePreviewWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();
	void Hide();

	void UpdateDivideWindows(DivideWndInfo& wnds);
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;
private:
	BYTE	m_byAlpha = 0;
};

