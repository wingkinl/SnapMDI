#pragma once

#include "LayeredAnimationWnd.h"
#include "SnapWindowManager.h"

namespace SnapChildWindows {

using CSnapAssistWndBase = CLayeredAnimationWnd;

class CSnapAssistWnd final : public CSnapAssistWndBase
{
public:
	CSnapAssistWnd(CSnapWindowManager* pManager);
	~CSnapAssistWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();

	void Hide(bool bStopNow = false);
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;
private:
	friend class CSnapWindowManager;
	CSnapWindowManager* m_pManager;

	SnapLayoutWindows	m_snapLayoutWnds;
};

}