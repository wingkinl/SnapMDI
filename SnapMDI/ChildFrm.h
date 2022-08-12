
// ChildFrm.h : interface of the CChildFrame class
//

#pragma once

#include "SnapChildWindows/SnapWindowManager.h"

#include "FloatMDIChild/FloatMDIChildHelper.h"
using namespace FloatMDIChild;


using CChildFrameBase = CMDIChildWndEx;

class CChildFrame : public CChildFrameBase
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame() noexcept;

// Attributes
protected:
	CSplitterWnd m_wndSplitter;
public:

// Operations
public:

// Overrides
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CSnapWindowHelper		m_snapHelper;
	CFloatMDIChildHelper	m_floatHelper;

	LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;

// Generated message map functions
protected:

	DECLARE_MESSAGE_MAP()
};
