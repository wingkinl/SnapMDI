#pragma once

#include "LayeredAnimationWnd.h"

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

	inline float GetAlphaFactor() const { return m_alpha; }
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;
private:
	CSnapWindowManager* m_pManager;
	float	m_alpha = 0.f;
};

