#pragma once

class CAlphaLayeredAnimationWnd;

class CALAWndRenderImp
{
public:
	CALAWndRenderImp() = default;
	virtual ~CALAWndRenderImp() = default;

	void AttachToALAWnd(CAlphaLayeredAnimationWnd* pWnd) { m_pALAWnd = pWnd; }

	virtual BOOL Create(CWnd* pWndOwner) = 0;

	virtual BOOL CanSupportAnimation() const { return FALSE; }

	virtual void StartRendering();

	virtual void StopRendering(bool bAbort);

	virtual void OnAnimationUpdate() {}

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) { return FALSE; }

	virtual BOOL GetCanvas(CRect& rect) const;
protected:
	CAlphaLayeredAnimationWnd* m_pALAWnd = nullptr;
};
