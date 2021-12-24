#pragma once

#include "LayeredAnimationWnd.h"
#include "SnapWindowManager.h"

namespace SnapChildWindows {

using CSnapAssistWndBase = CLayeredAnimationWnd;

typedef void (*EnumLayoutCellProc)(RECT rect, LPARAM lParam);

struct SnapGridWndAniInfo
{
	RECT	rectOld;
	HBITMAP hBmpWnd = nullptr;

	~SnapGridWndAniInfo()
	{
		if (hBmpWnd)
			DeleteObject(hBmpWnd);
	}
};

struct SnapGridWndAniData
{
	SnapWindowGridPos				wndPos;
	std::vector<SnapGridWndAniInfo>	snapGridWndAni;
};

class CSnapAssistWnd final : public CSnapAssistWndBase
{
public:
	CSnapAssistWnd(CSnapWindowManager* pManager);
	~CSnapAssistWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();

	void EnumLayoutCells(EnumLayoutCellProc pProc, LPARAM lParam) const;

	const SnapGridWndAniData& GetSnapGridWndAniData() const { return m_snapGridWndAni; }

	inline float GetTransitionFactor() const { return m_factor; }
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;

	void PrepareMoreInfoForAnimation();
private:
	friend class CSnapWindowManager;
	CSnapWindowManager* m_pManager;

	SnapGridWndAniData	m_snapGridWndAni;

	SnapLayoutWindows	m_snapLayoutWnds;

	float	m_factor = 0.f;
	bool	m_bShowLayoutCell = false;
};

}