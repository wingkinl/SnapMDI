#pragma once

#include "LayeredAnimationWndRenderImp.h"

#include <gdiplus.h>
using namespace Gdiplus;

#include <wrl.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <d2d1_2helper.h>
#include <dcomp.h>
#include <ShellScalingApi.h>
#include <d2d1_1.h>

using namespace Microsoft::WRL;
using namespace D2D1;

class CLayeredAnimationWndRenderImpInvert : public CLayeredAnimationWndRenderImp
{
public:
	void HandlePaint() override;
};


class CLayeredAnimationWndRenderImpAlpha : public CLayeredAnimationWndRenderImp
{
public:
	~CLayeredAnimationWndRenderImpAlpha();

	static BOOL IsApplicable()
	{
		if (GetGlobalData()->m_nBitsPerPixel > 8)
			return TRUE;
		return FALSE;
	}

	BOOL Create(CWnd* pWndOwner) override;

	BOOL CanSupportAnimation() const override;

	void StartRendering() override;

	static void UpdateRoundedRectPath(Gdiplus::GraphicsPath& path, const CRect& rect, int diameter);
protected:
	virtual bool NeedGDIPlus() const { return true; }

	CBitmap	m_bmp;
	LPBYTE	m_pBits = nullptr;
	CSize	m_szBmp;
private:
	bool m_bGDIPlusStarted = false;
	ULONG_PTR m_gdiplusToken = 0;
};

using ProcDCompositionCreateDevice2 = decltype(DCompositionCreateDevice2)*;
// minimum: Windows 8.1
using ProcGetDpiForMonitor = decltype(GetDpiForMonitor)*;

typedef HRESULT(*ProcD2D1CreateDevice)(
	IDXGIDevice* dxgiDevice,
	const D2D1_CREATION_PROPERTIES* creationProperties,
	ID2D1Device** d2dDevice
	);

class CLayeredAnimationWndRenderImpDirectComposition : public CLayeredAnimationWndRenderImp
{
	static PFN_D3D11_CREATE_DEVICE	procD3D11CreateDevice;
	static ProcD2D1CreateDevice		procD2D1CreateDevice;
	static ProcDCompositionCreateDevice2 procDCompositionCreateDevice2;
	static ProcGetDpiForMonitor procGetDpiForMonitor;
	static BOOL s_bApplicableCheck;
public:
	static BOOL IsApplicable();

	BOOL Create(CWnd* pWndOwner) override;

	struct ComException
	{
		HRESULT result;

		ComException(HRESULT const value) :
			result(value)
		{}
	};

	static void HR(HRESULT const result);

	template <typename T>
	static float DPtoLP(T const pixel, float const dpi)
	{
		return pixel * 96.0f / dpi;
	}

	bool IsDeviceCreated() const;

	void ReleaseDeviceResources();

	void CreateDeviceResources();

	virtual void CreateDeviceResourcesEx(ID2D1DeviceContext* pDC);

	BOOL CanSupportAnimation() const override;

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;

	void HandleSize(WPARAM wparam, LPARAM lparam);

	void HandleDPIChanged(WPARAM wParam, LPARAM lParam);
protected:
	// Device independent resources
	D2D_SIZE_F						m_size = { 0 };
	D2D_POINT_2F					m_dpi = { 0 };

	// Device resources
	ComPtr<ID3D11Device>			m_device3d;
	// The DirectComposition device
	ComPtr<IDCompositionDesktopDevice> m_device;
	ComPtr<IDCompositionTarget>  m_target;
	ComPtr<IDCompositionVisual2> m_visual;
	ComPtr<IDCompositionSurface> m_surface;
};
