#pragma once

#include "LayeredAnimationWnd.h"

class CSnapWindowManager;

using CGhostDividerWndBase = CLayeredAnimationWnd;

class CGhostDividerWnd final : public CGhostDividerWndBase
{
public:
	CGhostDividerWnd(CSnapWindowManager* pManager, bool bVertical);
	~CGhostDividerWnd();

	// in screen coordinates
	void Create(CWnd* pWndOwner, const POINT& pos, LONG length);

	bool IsVertical() const { return m_bVertical; }

	void Show();
	void Hide();
protected:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
private:
	void OnAnimationTimer(double timeDiff) override;
private:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	DECLARE_MESSAGE_MAP()
private:
	CSnapWindowManager* m_pManager;
	bool	m_bVertical = TRUE;
	BYTE	m_byAlpha = 0;
};

