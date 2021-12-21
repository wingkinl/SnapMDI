#pragma once

#include <chrono>
#include <memory>

#include "LayeredAnimationWndRenderImp.h"

class CLayeredAnimationWnd : public CWnd
{
public:
	CLayeredAnimationWnd();

	enum class RenderTech
	{
		// Direct Composition, fall back to layered if not supported
		DirectComposition,
		// Alpha blended layered window
		AlphaBlendedLayer,
		// Invert the color of the area
		InvertColor,
		Normal,
	};

	inline RenderTech GetRenderTech() const { return m_tech; }
	void SetPreferedRenderTech(RenderTech tech) { m_tech = tech; }

	inline bool IsAnimationEnabled() const { return m_bEnableAnimation; }
	void EnableAnimation(bool val) { m_bEnableAnimation = val; }

	const CRect& GetOwnerRect() const { return m_rcOwner; }
protected:
	void RepositionWindow(const CRect& rect);

	void GetWindowInOwnerRect(CRect& rect, CWnd* pWnd = nullptr) const;
	void StopAnimation();

	void FinishAnimationCleanup();

	void ScheduleAnimation();

	bool ShouldDoAnimation() const;

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

	virtual void OnAnimationTimer(double timeDiff) {}

	static LONG CalcSmoothPos(double pos, LONG from, LONG to);

	static CRect AnimateRect(double pos, const RECT& from, const RECT& to);
	// Attributes
protected:
	CWnd*		m_pWndOwner = nullptr;
	CWnd*		m_pActiveSnapWnd = nullptr;
	CRect		m_rcOwner;
	RenderTech	m_tech = RenderTech::DirectComposition;
	bool		m_bEnableAnimation = true;

	enum class AnimateStage
	{
		Ready,
		Showing,
		Hiding,
	};
	AnimateStage	m_aniStage = AnimateStage::Ready;

	std::shared_ptr<CLayeredAnimationWndRenderImp> m_renderImp;

	enum {TimerIDAnimation = 100};

	UINT_PTR	m_nTimerIDAni = 0;
	std::chrono::time_point<std::chrono::steady_clock>	m_AniStartTime;

	// Implementation
public:
	virtual ~CLayeredAnimationWnd();

	// Generated message map functions
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};
