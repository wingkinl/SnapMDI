#pragma once

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();
	void Create(CWnd* pWndOwner);

	void ShowAt(CWnd* pWnd, CRect rect);
	void Hide(CWnd* pWnd);

	void EnableAnimation(bool val);
	bool IsAnimationEnabled() const;

	// Attributes
protected:
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

