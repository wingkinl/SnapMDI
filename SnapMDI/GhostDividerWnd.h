#pragma once

#include "LayeredAnimationWnd.h"

using CGhostDividerWndBase = CLayeredAnimationWnd;

class CGhostDividerWnd : public CGhostDividerWndBase
{
public:
	CGhostDividerWnd(BOOL bVertical);

	// in screen coordinates
	void Create(CWnd* pWndOwner, const POINT& pos, LONG length);

	void Show();
	void Hide();
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
private:
	void OnAnimationTimer(double timeDiff) override;
private:
	BOOL	m_bVertical = TRUE;
	BYTE	m_byAlpha = 0;
};

