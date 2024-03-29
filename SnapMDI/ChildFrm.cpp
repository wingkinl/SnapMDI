
// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "pch.h"
#include "framework.h"
#include "SnapMDI.h"

#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CChildFrameBase)

BEGIN_MESSAGE_MAP(CChildFrame, CChildFrameBase)
END_MESSAGE_MAP()

// CChildFrame construction/destruction

CChildFrame::CChildFrame() noexcept
{
	m_snapHelper.InitSnap(&theApp.m_snapWndManager, this);
	
	m_floatHelper.InitFloat(&theApp.m_floatManager, this);
}

CChildFrame::~CChildFrame()
{
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	if (!CChildFrameBase::PreCreateWindow(cs))
		return FALSE;
	return TRUE;
}

// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CChildFrameBase::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CChildFrameBase::Dump(dc);
}
#endif //_DEBUG

// CChildFrame message handlers


LRESULT CChildFrame::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	{
		SnapWndMsg msg = { &m_snapHelper, message, wParam, lParam };
		if (m_snapHelper.PreWndMsg(msg))
			return msg.result;
	}
	{
		FloatMDIChildMsg msg = {&m_floatHelper, message, wParam, lParam};
		if (m_floatHelper.PreWndMsg(msg))
			return msg.result;
	}
	return __super::WindowProc(message, wParam, lParam);
}

