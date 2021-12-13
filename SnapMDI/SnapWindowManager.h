#pragma once
#include "SnapPreviewWnd.h"

class CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	BOOL OnWndMsg(CWnd* pWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	void Create(CWnd* pWndOwner);
protected:
	CSnapPreviewWnd	m_wndSnapPreview;
};

class CSnapWindowHelper
{
public:
	CSnapWindowHelper(CWnd* pWnd);
	~CSnapWindowHelper();
public:
	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
protected:
	CWnd*	m_pWnd;
	BOOL	m_bEnable = TRUE;
	POINT	m_ptStart = { 0 };
	BOOL	m_bIsMoving = FALSE;
};