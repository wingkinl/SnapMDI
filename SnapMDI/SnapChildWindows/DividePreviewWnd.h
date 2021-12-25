#pragma once

#include "LayeredAnimationWnd.h"

namespace SnapChildWindows {

class CSnapWindowManager;

using CDividePreviewWndBase = CLayeredAnimationWnd;

typedef void (*EnumDivideWindowsProc)(RECT rect, LPARAM lParam);

class CDividePreviewWnd final : public CDividePreviewWndBase
{
public:
	CDividePreviewWnd(CSnapWindowManager* pManager);

	BOOL Create(CWnd* pWndOwner);

	void Show();
	void Hide(bool bStopNow = false);

	void UpdateDivideWindows();

	void EnumDivideWindows(EnumDivideWindowsProc pProc, LPARAM lParam) const;

	inline float GetAlphaFactor() const { return m_factor; }
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;

	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	
	DECLARE_MESSAGE_MAP()
private:
	CSnapWindowManager* m_pManager;
	float	m_factor = 0.f;
};

}