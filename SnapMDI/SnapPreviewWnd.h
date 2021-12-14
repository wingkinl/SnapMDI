#pragma once

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();
	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping();

	// rect in screen coordinates
	void ShowAt(CRect rect);
	void Hide();

	void EnableAnimation(bool val);
	bool IsAnimationEnabled() const;

	void GetSnapRect(CRect& rect) const;

	CRect	m_rcOwner;
	// Attributes
protected:
	CWnd*	m_pWndOwner = nullptr;
	bool	m_bEnableAnimation = true;
	bool	m_bLayered = false;

	bool ShouldDoAnimation() const;

	// Implementation
public:
	virtual ~CSnapPreviewWnd();

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};

