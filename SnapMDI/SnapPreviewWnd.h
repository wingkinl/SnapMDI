#pragma once

#include "AlphaLayeredAnimationWnd.h"

using CSnapPreviewWndBase = CAlphaLayeredAnimationWnd;

class CSnapPreviewWnd : public CSnapPreviewWndBase
{
public:
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
	CRect		m_rectCur;
	CRect		m_rectFrom;
	CRect		m_rectTo;
};
