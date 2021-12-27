#pragma once

#include <memory>
#include <vector>
#include <map>

namespace SnapChildWindows {

class CSnapPreviewWnd;
class CGhostDividerWnd;
class CDividePreviewWnd;
class CSnapAssistWnd;

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

struct SnapCellInfo 
{
	RECT	rect;
};

struct SnapLayout 
{
	std::vector<SnapCellInfo>	cells;
};

struct SnapLayoutWindows 
{
	SnapLayout			layout;
	std::vector<HWND>	wnds;
};

struct WindowPos
{
	HWND	hWnd;
	RECT	rect;	// in screen coordinates
	UINT	flags;
};

struct SnapWindowGridPos
{
	std::vector<WindowPos>	wnds;
};

#ifndef SNAP_WINDOW_EXT_CLASS
	#define SNAP_WINDOW_EXT_CLASS
#endif // !SNAP_WINDOW_EXT_CLASS


class SNAP_WINDOW_EXT_CLASS CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	void InitSnap(CWnd* pWndOwner);

	virtual void OnSnapSwitch(bool bPressed);

	inline bool IsEnabled() const { return m_bEnable; }
	void Enable(bool val) { m_bEnable = val; }

	inline bool IsAnimationEnabled() const { return m_bEnableAnimation; }
	void EnableAnimation(bool val) { m_bEnableAnimation = val; }

	inline bool IsSnapAssistEnabled() const { return m_bEnableSnapAssist; }
	void EnableSnapAssist(bool val) { m_bEnableSnapAssist = val; }
protected:
	SnapWndMsg::HandleResult PreWndMsg(SnapWndMsg& msg);

	void HandleEnterSizeMove(SnapWndMsg& msg);
	
	void HandleExitSizeMove(SnapWndMsg& msg);

	void HandleMoving(SnapWndMsg& msg);

	void HandleSizing(SnapWndMsg& msg);

	void StartMoving(CSnapWindowHelper* pWndHelper);

	void StopMoving(bool bAbort = false);

	void OnMoving(CPoint pt);

	virtual BOOL EnterSnapping(const SnapWndMsg& msg) const;

	enum class SnapTargetType : BYTE
	{
		None	= 0x00,
		Owner	= 0x01,
		Child	= 0x02,
		Slot	= 0x04,
		Custom	= 0x08,
	};

	enum
	{
		// separate the mask here instead of putting it above so that
		// VS can show the combination in debug
		SnapTargetMask = 0x0f,
	};

	virtual SnapTargetType InitMovingSnap(const SnapWndMsg& msg);

	virtual void OnSnapToCurGrid();

	virtual bool ShouldDoAnimationToSnapCurGrid() const;

	virtual void GetSnapWindowGridPosResult(SnapWindowGridPos& grids) const;

	void SnapWindowsToGridResult(const SnapWindowGridPos& grids);

	virtual bool GetOwnerLayoutForSnapAssist(SnapLayoutWindows& layout) const;

	void FindMatchedWindowsInSnapLayout(SnapLayoutWindows& layout) const;

	bool ShowSnapAssist(SnapLayoutWindows&& layout);

	enum class SnapAssistEvent
	{
		Exit,
		FinishAnimation,
	};

	void OnSnapAssistEvent(SnapAssistEvent event);

	virtual BOOL EnterDividing(const SnapWndMsg& msg);

	virtual void StopDividing(bool bAbort);

	struct MinMaxDivideInfo 
	{
		LONG nMin;
		LONG nMax;
	};

	virtual BOOL CalcMinMaxDividingPos(const SnapWndMsg& msg, MinMaxDivideInfo& minmax) const;

	struct ChildWndInfo
	{
		HWND	hWndChild;
		RECT	rect;
		bool	bAdjacentToOwner;
		bool	bAdjacentToSibling;
	};
	friend struct AdjacentWindowsHelper;

	enum class SnapGridType : DWORD
	{
		None			= (DWORD)SnapTargetType::None,
		Owner			= (DWORD)SnapTargetType::Owner,
		Child			= (DWORD)SnapTargetType::Child,
		Slot			= (DWORD)SnapTargetType::Slot,
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
		ChildWndInfo	childInfo;
	};
	virtual SnapGridInfo GetSnapGridInfo(CPoint pt) const;

	virtual BOOL CanDoSnapping(SnapTargetType target) const;

	virtual SnapGridInfo GetSnapOwnerGridInfo(CPoint pt) const;

	virtual SnapGridInfo GetSnapChildGridInfo(CPoint pt) const;

	virtual SnapGridInfo GetSnapChildGridInfoEx(CPoint pt, const ChildWndInfo& childInfo) const;

	virtual SnapGridInfo GetSnapEmptySlotGridInfo(CPoint pt) const;
private:
	void PreSnapInitialize();

	void EnableSnapSwitchCheck(bool bEnable);

	virtual bool CheckSnapSwitch() const;

	static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

	void OnTimer(UINT_PTR nIDEvent, DWORD dwTime);

	UINT_PTR SetTimer(UINT_PTR nID, UINT uElapse);
	void KillTimer(UINT_PTR nID);

	void HandleNCMouseMove(SnapWndMsg& msg);
	void HandleNCMouseLeave(SnapWndMsg& msg);

	void CheckShowGhostDivider(SnapWndMsg& msg);
	void HideGhostDividers();

	friend class CGhostDividerWnd;

	void OnGhostDividerWndHidden(CGhostDividerWnd* pWnd);

	CSnapAssistWnd* GetSnapAssistWnd();
protected:
	friend class CSnapWindowHelper;

	CWnd*	m_pWndOwner = nullptr;
	CRect	m_rcOwner;
	std::unique_ptr<CSnapPreviewWnd>	m_wndSnapPreview;

	struct DividerWndInfo 
	{
		POINT pos;
		CGhostDividerWnd*	wnd = nullptr;
	};
	std::vector<DividerWndInfo>	m_vGhostDividerWnds;

	friend class CDividePreviewWnd;
	std::unique_ptr<CDividePreviewWnd>	m_wndDividePreview;

	friend class CSnapAssistWnd;
	std::unique_ptr<CSnapAssistWnd>	m_wndSnapAssist;

	CSnapWindowHelper*	m_pCurSnapWnd = nullptr;
	MINMAXINFO			m_curSnapWndMinMax = { 0 };
	POINT				m_ptStart = { 0 };
	BOOL				m_bEnterSizeMove = FALSE;
	BOOL				m_bCheckMoveAbortion = FALSE;
	BOOL				m_bIsMoving = FALSE;
	BOOL				m_bAborted = FALSE;
	SnapTargetType		m_snapTarget = SnapTargetType::None;
	SnapGridInfo		m_curGrid = {SnapGridType::None};

	bool				m_bEnable = true;
	bool				m_bEnableAnimation = true;
	bool				m_bEnableSnapAssist = true;

	// For snapping, does not include the active window
	// Sorted by Z order, the first one is at the top of the Z order.
	std::vector<ChildWndInfo>	m_vChildRects;
	// For dividing, since we also use it for animation, and two animation (one for snapping,
	// the other for dividing) can happen at the same time, we need another one instead of
	// reusing the m_vChildRects above
	std::vector<ChildWndInfo>	m_vDivideChildRects;
	int							m_nActiveDivideWndIdx = -1;	// index to m_vDivideChildRects

	struct WndEdge
	{
		size_t	rectIdx;	// points to m_vDivideChildRects
		bool	bFirst;		// first edge or the second
	};

	struct StickedWndDiv
	{
		// The starting point
		// For vertical dividers, this is the top-most point
		// For horizontal dividers, this is the left-most point
		// This point is good enough to distinguish dividers.
		POINT	pos = {-1, -1};
		LONG	length = 0;
		bool	vertical = false;
		std::vector<WndEdge>	wnds;

		bool valid = false;
	};
	StickedWndDiv		m_div;
	MinMaxDivideInfo	m_minmaxDiv = { 0 };
	BOOL				m_bDividing = FALSE;

	friend struct DivideWindowsHelper;

	UINT_PTR	m_nTimerIDSnapSwitch = 0;
	bool		m_bSwitchPressed = false;

	int			m_nNCHittestRes = HTNOWHERE;

	typedef std::map<UINT_PTR, CSnapWindowManager*> TimerMap;
	static TimerMap	s_mapTimers;
};


class SNAP_WINDOW_EXT_CLASS CSnapWindowHelper
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
		//return m_pManager->PostWndMsg(msg);
		return 0;
	}
protected:
	CSnapWindowManager* m_pManager = nullptr;
	CWnd* m_pWnd = nullptr;
};

}