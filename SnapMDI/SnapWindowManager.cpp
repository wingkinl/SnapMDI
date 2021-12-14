#include "pch.h"
#include "framework.h"
#include "SnapWindowManager.h"
#include "SnapPreviewWnd.h"

static int s_nHotRgnSize = -1;

enum class SnapGridType : DWORD
{
	None		= 0x00000000,
	Left		= 0x00000001,
	Right		= 0x00000002,
	Top			= 0x00000004,
	Bottom		= 0x00000008,
	TopLeft		= Top|Left,
	TopRight	= Top|Right,
	BottomLeft	= Bottom|Left,
	BottomRight	= Bottom|Right,
};


CSnapWindowManager::CSnapWindowManager()
{
	m_nCurGridType = SnapGridType::None;
}

CSnapWindowManager::~CSnapWindowManager()
{

}

void CSnapWindowManager::InitSnap(CWnd* pWndOwner)
{
	ASSERT(!m_pWndOwner);
	m_pWndOwner = pWndOwner;
}

BOOL CSnapWindowManager::OnWndMsg(SnapWndMsg& msg)
{
	switch (msg.message)
	{
	case WM_ENTERSIZEMOVE:
		// Do not handle it for system menu
		m_bEnterSizeMove = GetKeyState(VK_LBUTTON) < 0 && !msg.pHelper->GetWnd()->IsIconic();
		if (m_bEnterSizeMove)
		{
			GetCursorPos(&m_ptStart);
		}
		break;
	case WM_EXITSIZEMOVE:
		if (m_bEnterSizeMove)
		{
			if (m_bIsMoving)
			{
				ASSERT(msg.pHelper == m_pCurSnapWnd);
				StopMoving();
			}
			m_bEnterSizeMove = FALSE;
			m_bIsMoving = FALSE;
		}
		break;
	case WM_MOVING:
		if (m_bEnterSizeMove)
		{
			CPoint ptCurrent;
			GetCursorPos(&ptCurrent);
			if (!m_bIsMoving)
			{
				if (ptCurrent == m_ptStart)
					return FALSE;
				m_bIsMoving = TRUE;
				StartMoving(msg.pHelper);
			}
			OnMoving(ptCurrent);
		}
		break;
	}
	return FALSE;
}

CSnapPreviewWnd* CSnapWindowManager::GetSnapPreview()
{
	if (!m_wndSnapPreview)
	{
		m_wndSnapPreview.reset(new CSnapPreviewWnd);
		m_wndSnapPreview->Create(m_pWndOwner);
	}
	return m_wndSnapPreview.get();
}

void CSnapWindowManager::Initialize()
{
	if (s_nHotRgnSize < 0)
	{
		s_nHotRgnSize = GetSystemMetrics(SM_CYCAPTION);
	}
}

void CSnapWindowManager::StartMoving(CSnapWindowHelper* pWndHelper)
{
	auto pSnapWnd = GetSnapPreview();
	if (pSnapWnd)
	{
		m_pCurSnapWnd = pWndHelper;
		Initialize();
		pSnapWnd->StartSnapping();
	}
	else
	{
		m_pCurSnapWnd = nullptr;
	}
}

void CSnapWindowManager::StopMoving()
{
	if (!m_wndSnapPreview)
		return;
	m_wndSnapPreview->StopSnapping();
	if (m_nCurGridType != SnapGridType::None)
	{
		CRect rect;
		m_wndSnapPreview->GetSnapRect(rect);
		UINT nFlags = SWP_FRAMECHANGED;
		CWnd* pWnd = m_pCurSnapWnd->GetWnd();
		ASSERT(pWnd->GetParent() == m_pWndOwner);
		m_pWndOwner->ScreenToClient(rect);
		pWnd->SetWindowPos(nullptr, rect.left, rect.top, rect.Width(), rect.Height(), nFlags);
	}

	m_pCurSnapWnd = nullptr;
	m_nCurGridType = SnapGridType::None;
}

void CSnapWindowManager::OnMoving(CPoint pt)
{
	if (!m_wndSnapPreview)
		return;
	auto grid = GetSnapGridInfo(pt);
	if (m_nCurGridType != grid.type)
	{
		m_nCurGridType = grid.type;
		switch (m_nCurGridType)
		{
		case SnapGridType::None:
			m_wndSnapPreview->Hide();
			break;
		default:
			m_wndSnapPreview->ShowAt(grid.rect);
			break;
		}
	}
}

auto CSnapWindowManager::GetSnapGridInfo(CPoint pt) const -> SnapGridInfo
{
	SnapGridInfo grid = { SnapGridType::None, m_wndSnapPreview->m_rcOwner };
	CSize szGrid = grid.rect.Size();
	szGrid.cx /= 2;
	szGrid.cy /= 2;
	if (pt.x < grid.rect.left + s_nHotRgnSize)
	{
		grid.type = SnapGridType::Left;
		grid.rect.right = grid.rect.left + szGrid.cx;
	}
	else if (pt.x > grid.rect.right - s_nHotRgnSize)
	{
		grid.type = SnapGridType::Right;
		grid.rect.left = grid.rect.right - szGrid.cx;
	}
	if (pt.y < grid.rect.top + s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Top);
		grid.rect.bottom = grid.rect.top + szGrid.cy;
	}
	else if (pt.y > grid.rect.bottom - s_nHotRgnSize)
	{
		grid.type = (SnapGridType)((DWORD)grid.type | (DWORD)SnapGridType::Bottom);
		grid.rect.top = grid.rect.bottom - szGrid.cy;
	}
	return grid;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSnapWindowHelper::CSnapWindowHelper()
{
}

CSnapWindowHelper::~CSnapWindowHelper()
{

}

void CSnapWindowHelper::InitSnap(CSnapWindowManager* pManager, CWnd* pWnd)
{
	ASSERT(!m_pManager && !m_pWnd);
	m_pManager = pManager;
	m_pWnd = pWnd;
}

