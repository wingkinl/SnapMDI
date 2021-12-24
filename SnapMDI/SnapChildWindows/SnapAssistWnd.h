#pragma once

#include "LayeredAnimationWnd.h"
#include "SnapWindowManager.h"

namespace SnapChildWindows {

using CSnapAssistWndBase = CLayeredAnimationWnd;

typedef void (*EnumLayoutCellWindowsProc)(RECT rect, LPARAM lParam);

class CSnapAssistWnd final : public CSnapAssistWndBase
{
public:
	CSnapAssistWnd(CSnapWindowManager* pManager);
	~CSnapAssistWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();

	void EnumLayoutCellWindows(EnumLayoutCellWindowsProc pProc, LPARAM lParam) const;

	inline float GetTransitionFactor() const { return m_factor; }
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;
private:
	friend class CSnapWindowManager;
	CSnapWindowManager* m_pManager;

	SnapWindowGridPos	m_snapGridsAni;

	SnapLayoutWindows	m_snapLayoutWnds;

	float	m_factor = 0.f;
	bool	m_bShowLayoutCell = false;
};

}