#pragma once

#include <chrono>

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();
	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping();

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	void EnableAnimation(bool val);
	bool IsAnimationEnabled() const;

	void GetSnapRect(CRect& rect) const;

	CRect	m_rcOwner;
private:
	void RepositionWindow(const CRect& rect);

	void OnAnimationTo(const CRect& rect, bool bFinish);

	void GetWindowInOwnerRect(CWnd* pWnd, CRect& rect) const;

	void StopAnimation();

	void ScheduleAnimation();
	// Attributes
private:
	CWnd*	m_pWndOwner = nullptr;
	CWnd*	m_pActiveSnapWnd = nullptr;
	bool	m_bEnableAnimation = true;
	bool	m_bLayered = false;
	bool	m_bHiding = false;

	UINT_PTR	m_nTimerID = 0;
	std::chrono::time_point<std::chrono::steady_clock>	m_AniStartTime;
	CRect		m_rectCur;
	CRect		m_rectFrom;
	CRect		m_rectTo;

	bool ShouldDoAnimation() const;

	// Implementation
public:
	virtual ~CSnapPreviewWnd();

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

