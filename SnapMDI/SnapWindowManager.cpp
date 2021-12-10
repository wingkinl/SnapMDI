#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"

CSnapWindowManager::CSnapWindowManager()
{

}

CSnapWindowManager::~CSnapWindowManager()
{

}

void CSnapWindowManager::Create(CWnd* pWndOwner)
{
	m_wndSnapPreview.Create(pWndOwner);
}
