#pragma once

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();
	void Create(CWnd* pWndOwner);

	void ShowAt(CRect rect);

	// Attributes
protected:

	// Implementation
public:
	virtual ~CSnapPreviewWnd();

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};

