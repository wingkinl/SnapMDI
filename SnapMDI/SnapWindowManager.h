#pragma once
#include "SnapPreviewWnd.h"

class CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	void Create(CWnd* pWndOwner);
protected:
	CSnapPreviewWnd	m_wndSnapPreview;
};