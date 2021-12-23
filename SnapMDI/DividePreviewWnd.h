#pragma once

#include "LayeredAnimationWnd.h"

using CDividePreviewWndBase = CLayeredAnimationWnd;

class CDividePreviewWnd : public CDividePreviewWndBase
{
public:
	CDividePreviewWnd();

	BOOL Create(CWnd* pWndOwner);

	void Show();
	void Hide();
private:
};

