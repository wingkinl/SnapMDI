#pragma once

namespace FloatMDIChild {

#ifndef FLOAT_MDI_CHILD_EXT_CLASS
	#define FLOAT_MDI_CHILD_EXT_CLASS
#endif // !FLOAT_MDI_CHILD_EXT_CLASS

class CFloatMDIChildHelper;

struct FloatMDIChildMsg
{
	CFloatMDIChildHelper* pHelper;
	UINT message;
	WPARAM wp;
	LPARAM lp;
	LRESULT result;
};


class FLOAT_MDI_CHILD_EXT_CLASS CFloatMDIChildManager
{
public:
	CFloatMDIChildManager();
	~CFloatMDIChildManager();
public:
	void InitFloat(CMDIFrameWnd* pFrameWnd);

	inline bool IsEnabled() const { return m_bEnable; }
	void Enable(bool val) { m_bEnable = val; }
protected:
	BOOL PreWndMsg(FloatMDIChildMsg& msg);
protected:
	friend class CFloatMDIChildHelper;

	bool	m_bEnable = true;
};

class FLOAT_MDI_CHILD_EXT_CLASS CFloatMDIChildHelper
{
public:
	CFloatMDIChildHelper();
	~CFloatMDIChildHelper();
public:
	void InitFloat(CFloatMDIChildManager* pManager, CWnd* pWnd);

	inline CFloatMDIChildManager* GetManager() const
	{
		return m_pManager;
	}
	inline CWnd* GetWnd() const
	{
		return m_pWnd;
	}

	inline BOOL PreWndMsg(FloatMDIChildMsg& msg)
	{
		return m_pManager->PreWndMsg(msg);
	}
protected:
	CFloatMDIChildManager*	m_pManager = nullptr;
	CWnd*					m_pWnd = nullptr;
};

}