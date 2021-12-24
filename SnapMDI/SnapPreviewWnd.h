#pragma once

#include "LayeredAnimationWnd.h"

namespace SnapChildWindows {

using CSnapPreviewWndBase = CLayeredAnimationWnd;

class CSnapPreviewWnd : public CSnapPreviewWndBase
{
public:
	CSnapPreviewWnd();

	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping();

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	const CRect& GetCurRect() const override { return m_rectCur; }
private:
	void OnAnimationTo(const CRect& rect, bool bFinish);

	void OnAnimationTimer(double timeDiff) override;
private:
	CWnd*	m_pActiveSnapWnd = nullptr;
	CRect	m_rectCur;
	CRect	m_rectFrom;
	CRect	m_rectTo;
};

}