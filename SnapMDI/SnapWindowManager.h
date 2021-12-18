#pragma once

#include <memory>
#include <vector>

class CSnapPreviewWnd;
class CSnapWindowHelper;

struct SnapWndMsg
{
	CSnapWindowHelper* pHelper;
	UINT message;
	WPARAM wp;
	LPARAM lp;
	LRESULT* pResult;
};

class CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	void InitSnap(CWnd* pWndOwner);
protected:
	BOOL OnWndMsg(const SnapWndMsg& msg);

	CSnapPreviewWnd* GetSnapPreview();

	void StartMoving(CSnapWindowHelper* pWndHelper);

	void StopMoving(bool bAbort = false);

	void OnMoving(CPoint pt);

	virtual BOOL EnterSnapping(const SnapWndMsg& msg) const;

	enum class SnapTargetType : BYTE
	{
		None	= 0x00,
		Owner	= 0x01,
		Child	= 0x02,
		Custom	= 0x04,
	};

	virtual SnapTargetType InitMovingSnap(const SnapWndMsg& msg);

	struct ChildWndInfo
	{
		HWND	hWndChild;
		RECT	rect;
		BYTE	states;
		HWND	hWndSiblingLeft;
		HWND	hWndSiblingTop;
		HWND	hWndSiblingRight;
		HWND	hWndSiblingBottom;

		enum class StateFlag : BYTE
		{
			BorderWithOwnerLeft		= 0x01,
			BorderWithOwnerTop		= 0x02,
			BorderWithOwnerRight	= 0x04,
			BorderWithOwnerBottom	= 0x08,
			BorderWithSibling		= 0x10,
		};
	};

	enum class SnapGridType : DWORD
	{
		SnapTargetMask	= 0x000000ff,
		Left			= 0x00000100,
		Right			= 0x00000200,
		Top				= 0x00000400,
		Bottom			= 0x00000800,
		TopLeft			= Top|Left,
		TopRight		= Top|Right,
		BottomLeft		= Bottom|Left,
		BottomRight		= Bottom|Right,
	};

	struct SnapGridInfo
	{
		DWORD	type;
		CRect	rect;
	};
	virtual SnapGridInfo GetSnapGridInfo(CPoint pt) const;

	virtual SnapGridInfo GetSnapOwnerGridInfo(CPoint pt) const;

	virtual SnapGridInfo GetSnapChildGridInfo(CPoint pt) const;

	virtual SnapGridInfo GetSnapChildGridInfoEx(CPoint pt, const ChildWndInfo& childInfo) const;
private:
	void PreSnapInitialize();
protected:
	friend class CSnapWindowHelper;

	CWnd*	m_pWndOwner = nullptr;
	CRect	m_rcOwner;
	std::unique_ptr<CSnapPreviewWnd>	m_wndSnapPreview;

	CSnapWindowHelper*	m_pCurSnapWnd = nullptr;
	POINT				m_ptStart = { 0 };
	BOOL				m_bEnterSizeMove = FALSE;
	BOOL				m_bIsMoving = FALSE;
	SnapTargetType		m_snapTarget = SnapTargetType::None;
	SnapGridInfo		m_curGrid = {0};

	std::vector<ChildWndInfo>	m_vChildRects;
};


class CSnapWindowHelper
{
public:
	CSnapWindowHelper();
	~CSnapWindowHelper();
public:
	void InitSnap(CSnapWindowManager* pManager, CWnd* pWnd);

	inline CSnapWindowManager* GetManager() const
	{
		return m_pManager;
	}
	inline CWnd* GetWnd() const
	{
		return m_pWnd;
	}

	inline BOOL OnWndMsg(const SnapWndMsg& msg)
	{
		return m_pManager->OnWndMsg(msg);
	}
protected:
	CSnapWindowManager* m_pManager = nullptr;
	CWnd* m_pWnd = nullptr;
};