#pragma once

#include <memory>

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

enum class SnapGridType : DWORD;

class CSnapWindowManager
{
public:
	CSnapWindowManager();
	virtual ~CSnapWindowManager();
public:
	void InitSnap(CWnd* pWndOwner);
protected:
	BOOL OnWndMsg(SnapWndMsg& msg);

	CSnapPreviewWnd* GetSnapPreview();

	void Initialize();

	void StartMoving(CSnapWindowHelper* pWndHelper);

	void StopMoving();

	void OnMoving(CPoint pt);

	struct SnapGridInfo
	{
		SnapGridType	type;
		CRect			rect;
	};
	SnapGridInfo GetSnapGridInfo(CPoint pt) const;
protected:
	friend class CSnapWindowHelper;

	CWnd* m_pWndOwner = nullptr;
	std::unique_ptr<CSnapPreviewWnd>	m_wndSnapPreview;

	CSnapWindowHelper*	m_pCurSnapWnd = nullptr;
	POINT				m_ptStart = { 0 };
	BOOL				m_bEnterSizeMove = FALSE;
	BOOL				m_bIsMoving = FALSE;
	SnapGridType		m_nCurGridType;
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

	inline BOOL OnWndMsg(SnapWndMsg& msg)
	{
		return m_pManager->OnWndMsg(msg);
	}
protected:
	CSnapWindowManager* m_pManager = nullptr;
	CWnd* m_pWnd = nullptr;
};