#pragma once

#include "LayeredAnimationWnd.h"

namespace SnapChildWindows {

class CSnapWindowManager;

using CSnapAssistWndBase = CLayeredAnimationWnd;

class CSnapAssistWnd final : public CSnapAssistWndBase
{
public:
	CSnapAssistWnd(CSnapWindowManager* pManager);

	BOOL Create(CWnd* pWndOwner);
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;
private:
	CSnapWindowManager* m_pManager;
};

}