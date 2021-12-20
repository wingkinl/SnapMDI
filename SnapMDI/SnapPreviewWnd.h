#pragma once

#include "AlphaLayeredAnimationWnd.h"

class CSnapWindowManager;

using CSnapPreviewWndBase = CAlphaLayeredAnimationWnd;

class CSnapPreviewWnd : public CSnapPreviewWndBase
{
public:
	CSnapPreviewWnd(CSnapWindowManager* pManager);

	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping(bool bAbort);

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	const CRect& GetCurRect() const { return m_rectCur; }

	void EnableSnapSwitchCheck(bool bEnable);

	bool CheckSnapSwitch() const;
private:
	void OnAnimationTo(const CRect& rect, bool bFinish);

	void OnAnimationTimer(double timeDiff) override;

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
private:
	CSnapWindowManager* m_pSnapManager = nullptr;

	bool		m_bStartedSnapping = false;
	CRect		m_rectCur;
	CRect		m_rectFrom;
	CRect		m_rectTo;

	UINT_PTR	m_nTimerIDSnapSwitch = 0;
	bool		m_bSwitchKeyPressed = false;
};
