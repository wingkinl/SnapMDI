
// MainFrm.h : interface of the CMainFrame class
//

#pragma once

class CMainFrame;
class CSnapPreviewWnd;

class CFloatFrameBase : public CMDIFrameWnd
{
public:
	void AttachMDIChild(CMDIChildWnd* pWndChild, bool bMaximize = true);
};

class CFloatFrame : public CFloatFrameBase
{
public:
	CFloatFrame(CMainFrame* pMain);
	~CFloatFrame();
public:
	BOOL Create(CWnd* pWndParent);
protected:
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;
private:
	CMainFrame* m_pMainFrame;
protected:
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
protected:
	DECLARE_MESSAGE_MAP()
};

using CMainFrameBase = CMDIFrameWndEx;

class CMainFrame : public CMainFrameBase
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame() noexcept;

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CMFCMenuBar		m_wndMenuBar;
	CMFCStatusBar	m_wndStatusBar;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTestFloat();
	DECLARE_MESSAGE_MAP()

};


