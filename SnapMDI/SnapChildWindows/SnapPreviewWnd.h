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

	const CRect& GetCurRect() const override { return m_rcCur; }

	const CRect& GetSecondaryRect() const { return m_rcSecondaryCur; }

	inline void SetShowSecondary(bool val) { m_bShowSecondary = val; }
	inline bool IsShowSecondary()  const { return m_bShowSecondary; }

	void SetSecondaryRects(const RECT& rcFrom, const RECT& rcTo);
private:
	void UpdateAnimation(bool bFinish);

	void OnAnimationTimer(double timeDiff) override;
private:
	CWnd*	m_pActiveSnapWnd = nullptr;

	CRect	m_rcCur;
	CRect	m_rcFrom;
	CRect	m_rcTo;

	bool	m_bShowSecondary = false;
	CRect	m_rcSecondaryCur;
	CRect	m_rcSecondaryFrom;
	CRect	m_rcSecondaryTo;
};

}