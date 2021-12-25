
// SnapMDIView.h : interface of the CSnapMDIView class
//

#pragma once


class CSnapMDIView : public CView
{
protected: // create from serialization only
	CSnapMDIView() noexcept;
	DECLARE_DYNCREATE(CSnapMDIView)

// Attributes
public:
	CSnapMDIDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	COLORREF m_crfBackground;
// Implementation
public:
	virtual ~CSnapMDIView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	afx_msg BOOL OnEraseBkgnd(CDC*);
// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SnapMDIView.cpp
inline CSnapMDIDoc* CSnapMDIView::GetDocument() const
   { return reinterpret_cast<CSnapMDIDoc*>(m_pDocument); }
#endif

