#pragma once

#include <chrono>
#include <memory>

class CSnapAnimationImp;

class CSnapPreviewWnd : public CWnd
{
public:
	enum ConfigFlag : DWORD
	{
		AnimationTechMask		= 0x00000003,
		// Direct Composition, fall back to layered if not supported
		AnimationTechDC			= 0x00000000,
		// Alpha blended layered window
		AnimationTechAlpha		= 0x00000001,
		// Invert the color of the area
		AnimationTechInvert		= 0x00000002,
		// Animation implemented by moving this window
		AnimateByMovingWnd		= 0x00000010,
		DisableAnimation		= 0x00000020,
	};

	CSnapPreviewWnd(DWORD nConfigFlags = 0);

	void Create(CWnd* pWndOwner);

	void StartSnapping();
	void StopSnapping(bool bAbort);

	// rect in screen coordinates
	void ShowAt(CWnd* pActiveSnapWnd, const CRect& rect);
	void Hide();

	inline DWORD GetConfigFlags() const { return m_nConfigs; }

	void RepositionWindow(const CRect& rect);
private:
	void OnAnimationTo(const CRect& rect, bool bFinish);

	void GetWindowInOwnerRect(CRect& rect, CWnd* pWnd = nullptr) const;
	void StopAnimation();

	void FinishAnimationCleanup();

	void ScheduleAnimation();

	DWORD GetAnimationTech() const;

	bool ShouldDoAnimation() const;

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
	// Attributes
private:
	CWnd*	m_pWndOwner = nullptr;
	CWnd*	m_pActiveSnapWnd = nullptr;
	CRect	m_rcOwner;
	DWORD	m_nConfigs = 0;

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

