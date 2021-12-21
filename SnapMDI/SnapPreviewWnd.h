#pragma once

#include "LayeredAnimationWnd.h"

using CSnapPreviewWndBase = CLayeredAnimationWnd;

class CSnapPreviewWnd : public CSnapPreviewWndBase
{
public:
	CSnapPreviewWnd();

	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping(bool bAbort);

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	const CRect& GetCurRect() const { return m_rectCur; }
private:
	void OnAnimationTo(const CRect& rect, bool bFinish);

	void OnAnimationTimer(double timeDiff) override;
private:
	bool		m_bStartedSnapping = false;
	CRect		m_rectCur;
	CRect		m_rectFrom;
	CRect		m_rectTo;
};
