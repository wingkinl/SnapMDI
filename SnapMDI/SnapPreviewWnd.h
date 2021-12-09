#pragma once

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();
	void Create(CWnd* pwndOwner);

	void ShowAt(CRect rect);

	// Attributes
protected:
	CWnd* m_pWndOwner;

	// Implementation
public:
	virtual ~CSnapPreviewWnd();
	void PostNcDestroy() override { delete this; }

	// Generated message map functions
protected:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);

	DECLARE_MESSAGE_MAP()
};

