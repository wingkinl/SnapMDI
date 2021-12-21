#pragma once

#include "LayeredAnimationWnd.h"

using CGhostDividerWndBase = CLayeredAnimationWnd;

class CGhostDividerWnd : public CGhostDividerWndBase
{
public:
	CGhostDividerWnd(BOOL bVertical);

	void Create(CWnd* pWndOwner);

	// in screen coordinates
	void Show(const POINT& pos, LONG length);
	void Hide();
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
private:
	void OnAnimationTimer(double timeDiff) override;
private:
	BOOL	m_bVertical = TRUE;
	BYTE	m_byAlpha = 0;
};

