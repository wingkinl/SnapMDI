#pragma once

class CLayeredAnimationWnd;

class CLayeredAnimationWndRenderImp
{
public:
	CLayeredAnimationWndRenderImp() = default;
	virtual ~CLayeredAnimationWndRenderImp() = default;

	void AttachToLayeredAnimationWnd(CLayeredAnimationWnd* pWnd) { m_pWnd = pWnd; }

	virtual BOOL Create(CWnd* pWndOwner, LPCRECT pRect = nullptr);

	virtual BOOL CanSupportAnimation() const { return FALSE; }

	virtual void StartRendering();

	virtual void StopRendering(bool bAbort);

	virtual void OnAnimationUpdate() {}

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

	virtual void HandlePaint() {}

	enum class RectType
	{
		Canvas,
		CurTarget,
	};
	virtual BOOL GetRect(CRect& rect, RectType type) const;
protected:
	CLayeredAnimationWnd* m_pWnd = nullptr;
};
