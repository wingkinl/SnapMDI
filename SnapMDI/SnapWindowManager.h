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
	LRESULT result;

	enum class HandleResult
	{
		Continue,
		Handled,
		NeedPostWndMsg,
	};
};

class CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	void InitSnap(CWnd* pWndOwner);
protected:
	SnapWndMsg::HandleResult PreWndMsg(SnapWndMsg& msg);

	LRESULT PostWndMsg(SnapWndMsg& msg);

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

	enum
	{
		// separate the mask here instead of putting it above so that
		// VS can show the combination in debug
		SnapTargetMask = 0x0f,
	};

	virtual SnapTargetType InitMovingSnap(const SnapWndMsg& msg);

	virtual BOOL OnSnapTo();

	virtual void OnAfterSnap();

	virtual void OnAfterSnapToOwner();

	virtual void OnAfterSnapToChild();

	virtual void OnAfterSnapToCustom();

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
		None			= (DWORD)SnapTargetType::None,
		Owner			= (DWORD)SnapTargetType::Owner,
		Child			= (DWORD)SnapTargetType::Child,
		Custom			= (DWORD)SnapTargetType::Custom,
		Left			= 0x00000100,
		Right			= 0x00000200,
		Top				= 0x00000400,
		Bottom			= 0x00000800,
	};

	enum
	{
		// separate the mask here instead of putting it above so that
		// VS can show the combination in debug
		SnapGridSideMask = 0x00000f00,
	};

	struct SnapGridInfo
	{
		SnapGridType	type;
		CRect			rect;
		const ChildWndInfo*	childInfo;
	};
	virtual SnapGridInfo GetSnapGridInfo(CPoint pt) const;

	virtual BOOL IsSnappingApplicable(SnapTargetType target) const;

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
	MINMAXINFO			m_curSnapWndMinMax = { 0 };
	POINT				m_ptStart = { 0 };
	BOOL				m_bEnterSizeMove = FALSE;
	BOOL				m_bCheckMoveAbortion = FALSE;
	BOOL				m_bIsMoving = FALSE;
	BOOL				m_bAborted = FALSE;
	SnapTargetType		m_snapTarget = SnapTargetType::None;
	SnapGridInfo		m_curGrid = {SnapGridType::None};

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

	inline SnapWndMsg::HandleResult PreWndMsg(SnapWndMsg& msg)
	{
		return m_pManager->PreWndMsg(msg);
	}
	inline LRESULT PostWndMsg(SnapWndMsg& msg)
	{
		return m_pManager->PostWndMsg(msg);
	}
protected:
	CSnapWindowManager* m_pManager = nullptr;
	CWnd* m_pWnd = nullptr;
};