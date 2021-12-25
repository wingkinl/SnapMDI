#pragma once

#include "LayeredAnimationWnd.h"
#include "SnapWindowManager.h"

namespace SnapChildWindows {

using CSnapAssistWndBase = CLayeredAnimationWnd;

struct BitmapWrapper 
{
	HBITMAP	hBmp;

	~BitmapWrapper()
	{
		if (hBmp)
			DeleteObject(hBmp);
	}
};

struct InitSnapWnd
{
	RECT			rectOld;
	BitmapWrapper	bmp;
};

struct SnapGridWndData
{
	SnapWindowGridPos			wndPos;
	std::vector<InitSnapWnd>	initSnapWnd;
};

struct SnapCandidateWnd 
{
	HWND			hWnd;
	BitmapWrapper	bmp;
};

struct SnapAssistData 
{
	// For the initial moving window animation only
	SnapGridWndData		snapGridWnd;

	SnapLayoutWindows	snapLayoutWnds;

	std::vector<SnapCandidateWnd>	candidateWnds;

	float				factor = 0.f;
	bool				bShowLayoutCell = false;
};

class CSnapAssistWnd final : public CSnapAssistWndBase
{
public:
	CSnapAssistWnd(CSnapWindowManager* pManager);
	~CSnapAssistWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();

	const SnapAssistData& GetData() const { return m_data; }
private:
	BOOL PreCreateWindow(CREATESTRUCT& cs) override;

	void OnAnimationTimer(double timeDiff) override;

	void PrepareSnapAssist(bool bShowAnimationForInitialWnds);

	void OnExitSnapAssist();
private:
	friend class CSnapWindowManager;
	CSnapWindowManager* m_pManager;

	SnapAssistData		m_data;
};

}