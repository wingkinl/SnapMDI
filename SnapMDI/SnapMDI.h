
// SnapMDI.h : main header file for the SnapMDI application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "SnapWindowManager.h"


// CSnapMDIApp:
// See SnapMDI.cpp for the implementation of this class
//

class CSnapMDIApp : public CWinAppEx
{
public:
	CSnapMDIApp() noexcept;


	CSnapWindowManager m_snapWndManager;

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CSnapMDIApp theApp;