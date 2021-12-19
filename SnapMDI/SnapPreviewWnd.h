#pragma once

#include <chrono>
#include <memory>

class CSnapAnimationImp;

class CSnapPreviewWnd : public CWnd
{
public:
	CSnapPreviewWnd();

	void Create(CWnd* pWndOwner);

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

	inline bool IsAnimateByMovingWnd() const { return m_bAnimateByMovingWnd; }
	void SetAnimateByMovingWnd(bool val) { m_bAnimateByMovingWnd = val; }

	void StartSnapping();
	void StopSnapping(bool bAbort);

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	void RepositionWindow(const CRect& rect);
private:
	void OnAnimationTo(const CRect& rect, bool bFinish);

	void GetWindowInOwnerRect(CRect& rect, CWnd* pWnd = nullptr) const;
	void StopAnimation();

	void FinishAnimationCleanup();

	void ScheduleAnimation();

	bool ShouldDoAnimation() const;

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	// Attributes
private:
	CWnd*	m_pWndOwner = nullptr;
	CWnd*	m_pActiveSnapWnd = nullptr;
	CRect	m_rcOwner;
	RenderTech	m_tech = RenderTech::DirectComposition;
	bool		m_bEnableAnimation = true;
	bool		m_bAnimateByMovingWnd = false;

	enum class AnimateStage
	{
		Ready,
		Showing,
		Hiding,
	};
	AnimateStage	m_aniStage = AnimateStage::Ready;

	std::unique_ptr<CSnapAnimationImp> m_animation;

	UINT_PTR	m_nAniTimerID = 0;
	std::chrono::time_point<std::chrono::steady_clock>	m_AniStartTime;
	CRect		m_rectCur;
	CRect		m_rectFrom;
	CRect		m_rectTo;

	// Implementation
public:
	virtual ~CSnapPreviewWnd();

	// Generated message map functions
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()
};

