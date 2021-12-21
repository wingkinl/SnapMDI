#pragma once

#include "LayeredAnimationWnd.h"

using CGhostDividerWndBase = CLayeredAnimationWnd;

class CGhostDividerWnd : public CGhostDividerWndBase
{
public:
	CGhostDividerWnd();

	void Create(CWnd* pWndOwner);

	// rect in screen coordinates
	void Show(const CRect& rect, bool bVertical);
	void Hide();
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
private:
	void OnAnimationTimer(double timeDiff) override;
private:
	BYTE	m_byAlpha = 0;
};

