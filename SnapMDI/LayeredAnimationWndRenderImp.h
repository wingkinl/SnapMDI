#pragma once

class CLayeredAnimationWnd;

class CLayeredAnimationWndRenderImp
{
public:
	CLayeredAnimationWndRenderImp() = default;
	virtual ~CLayeredAnimationWndRenderImp() = default;

	void AttachToALAWnd(CLayeredAnimationWnd* pWnd) { m_pALAWnd = pWnd; }

	virtual BOOL Create(CWnd* pWndOwner) = 0;

	virtual BOOL CanSupportAnimation() const { return FALSE; }

	virtual void StartRendering();

	virtual void StopRendering(bool bAbort);

	virtual void OnAnimationUpdate() {}

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return FALSE; }

	virtual BOOL GetCanvas(CRect& rect) const;
protected:
	CLayeredAnimationWnd* m_pALAWnd = nullptr;
};
