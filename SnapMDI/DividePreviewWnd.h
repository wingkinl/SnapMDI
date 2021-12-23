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
	void Hide();

	void UpdateDivideWindows();

	void EnumDivideWindows(EnumDivideWindowsProc pProc, LPARAM lParam) const;
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;
private:
	CSnapWindowManager* m_pManager;
	BYTE	m_byAlpha = 0;
};

