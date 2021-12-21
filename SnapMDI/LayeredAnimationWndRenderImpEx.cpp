#include "pch.h"
#include "framework.h"
#include "LayeredAnimationWndRenderImpEx.h"
#include "LayeredAnimationWnd.h"

void CLayeredAnimationWndRenderImp::StartRendering()
{
	CRect rect;
	if (GetCanvas(rect))
	{
		m_pALAWnd->SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(),
			SWP_NOACTIVATE | SWP_NOREDRAW);
	}
}

void CLayeredAnimationWndRenderImp::StopRendering(bool bAbort)
{
	//
}

BOOL CLayeredAnimationWndRenderImp::GetCanvas(CRect& rect) const
{
	if (m_pALAWnd)
	{
		rect = m_pALAWnd->GetOwnerRect();
		return TRUE;
	}
	ASSERT(0);
	return FALSE;
}

BOOL CLayeredAnimationWndRenderImpInvert::Create(CWnd* pWndOwner)
{
	CRect rect;
	rect.SetRectEmpty();

	DWORD dwStyle = WS_POPUP;
	DWORD dwExStyle = 0;
	UINT nClassStyle = CS_HREDRAW | CS_VREDRAW;
	return m_pALAWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);
}

BOOL CLayeredAnimationWndRenderImpInvert::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_PAINT:
		HandlePaint();
		break;
	}
	return FALSE;
}

CLayeredAnimationWndRenderImpAlpha::~CLayeredAnimationWndRenderImpAlpha()
{
	if (m_bGDIPlusStarted)
	{
		GdiplusShutdown(m_gdiplusToken);
	}
}

BOOL CLayeredAnimationWndRenderImpAlpha::Create(CWnd* pWndOwner)
{
	CRect rect;
	rect.SetRectEmpty();

	DWORD dwStyle = WS_POPUP;
	DWORD dwExStyle = WS_EX_LAYERED;
	UINT nClassStyle = 0;
	BOOL bOK = m_pALAWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);

	if (bOK)
	{
		if (NeedGDIPlus())
		{
			GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
			m_bGDIPlusStarted = true;
		}
	}
	return bOK;
}

BOOL CLayeredAnimationWndRenderImpAlpha::CanSupportAnimation() const
{
	return TRUE;
}

void CLayeredAnimationWndRenderImpAlpha::StartRendering()
{
	__super::StartRendering();

	CRect rectOwner;
	GetCanvas(rectOwner);

	CSize size(rectOwner.Size());
	if (m_szBmp != size)
	{
		m_bmp.DeleteObject();

		m_pBits = NULL;
		HBITMAP hBitmap = CDrawingManager::CreateBitmap_32(size, (void**)&m_pBits);
		if (!hBitmap)
		{
			return;
		}
		m_bmp.Attach(hBitmap);
		m_szBmp = size;
	}
}

BOOL CLayeredAnimationWndRenderImpAlpha::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_PAINT:
		HandlePaint();
		break;
	}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

PFN_D3D11_CREATE_DEVICE	CLayeredAnimationWndRenderImpDirectComposition::procD3D11CreateDevice = nullptr;
ProcD2D1CreateDevice			CLayeredAnimationWndRenderImpDirectComposition::procD2D1CreateDevice = nullptr;
ProcDCompositionCreateDevice2	CLayeredAnimationWndRenderImpDirectComposition::procDCompositionCreateDevice2 = nullptr;
ProcGetDpiForMonitor	CLayeredAnimationWndRenderImpDirectComposition::procGetDpiForMonitor = nullptr;
BOOL	CLayeredAnimationWndRenderImpDirectComposition::s_bApplicableCheck = (BOOL)-1;

BOOL CLayeredAnimationWndRenderImpDirectComposition::IsApplicable()
{
	if (s_bApplicableCheck != (BOOL)-1)
		return s_bApplicableCheck;
	auto hD3D11 = LoadLibraryW(L"D3D11.dll");
	if (hD3D11)
	{
		procD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(hD3D11, "D3D11CreateDevice");
	}
	auto hD2d1 = LoadLibraryW(L"D2d1.dll");
	if (hD2d1)
	{
		procD2D1CreateDevice = (ProcD2D1CreateDevice)GetProcAddress(hD2d1, "D2D1CreateDevice");
	}
	auto hDcomp = LoadLibraryW(L"Dcomp.dll");
	if (hDcomp)
	{
		procDCompositionCreateDevice2 = (ProcDCompositionCreateDevice2)GetProcAddress(hDcomp, "DCompositionCreateDevice2");
	}
	auto hShcore = LoadLibraryW(L"Shcore.dll");
	if (hShcore)
	{
		procGetDpiForMonitor = (ProcGetDpiForMonitor)GetProcAddress(hShcore, "GetDpiForMonitor");
	}
	BOOL bApplicable = procD3D11CreateDevice && procD2D1CreateDevice
		&& procDCompositionCreateDevice2;
	if (!bApplicable)
	{
		if (hD3D11)
			FreeLibrary(hD3D11);
		if (hD2d1)
			FreeLibrary(hD2d1);
		if (hDcomp)
			FreeLibrary(hDcomp);
		if (hShcore)
			FreeLibrary(hShcore);
	}
	return s_bApplicableCheck = bApplicable;
}

BOOL CLayeredAnimationWndRenderImpDirectComposition::Create(CWnd* pWndOwner)
{
	CRect rect;
	rect.SetRectEmpty();

	DWORD dwStyle = WS_POPUP;
	DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
	UINT nClassStyle = 0;
	BOOL bOK = m_pALAWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);

	return bOK;
}

void CLayeredAnimationWndRenderImpDirectComposition::HR(HRESULT const result)
{
	if (S_OK != result)
	{
		throw ComException(result);
	}
}

bool CLayeredAnimationWndRenderImpDirectComposition::IsDeviceCreated() const
{
	return m_device3d;
}

void CLayeredAnimationWndRenderImpDirectComposition::ReleaseDeviceResources()
{
	m_device3d.Reset();
}

void CLayeredAnimationWndRenderImpDirectComposition::CreateDeviceResources()
{
	if (IsDeviceCreated())
		return;

	HR(procD3D11CreateDevice(nullptr,	// Adapter
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,	// Module
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr, 0,	// Highest available feature level
		D3D11_SDK_VERSION,
		m_device3d.GetAddressOf(),
		nullptr,	// Actual feature level
		nullptr));	// Device context
	ComPtr<IDXGIDevice> devicex;
	HR(m_device3d.As(&devicex));
	ComPtr<ID2D1Device> device2d;
	HR(procD2D1CreateDevice(devicex.Get(),
		nullptr, // Default properties
		device2d.GetAddressOf()));

	HR(procDCompositionCreateDevice2(
		device2d.Get(),
		__uuidof(m_device),
		reinterpret_cast<void**>(m_device.ReleaseAndGetAddressOf())));

	auto hWnd = m_pALAWnd->GetSafeHwnd();
	HR(m_device->CreateTargetForHwnd(hWnd,
		true, // Top most
		m_target.ReleaseAndGetAddressOf()));
	HR(m_device->CreateVisual(m_visual.ReleaseAndGetAddressOf()));
	RECT rect = {};
	m_pALAWnd->GetClientRect(&rect);

	HR(m_device->CreateSurface(rect.right - rect.left,
		rect.bottom - rect.top,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_ALPHA_MODE_PREMULTIPLIED,
		m_surface.ReleaseAndGetAddressOf()));
	HR(m_visual->SetContent(m_surface.Get()));
	HR(m_target->SetRoot(m_visual.Get()));

	ComPtr<ID2D1DeviceContext> dc;
	HR(device2d->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
		dc.GetAddressOf()));

	CreateDeviceResourcesEx(dc.Get());

	HMONITOR const monitor = MonitorFromWindow(hWnd,
		MONITOR_DEFAULTTONEAREST);
	unsigned x = 0;
	unsigned y = 0;
	if (procGetDpiForMonitor)
	{
		HR(procGetDpiForMonitor(monitor,
			MDT_EFFECTIVE_DPI,
			&x,
			&y));
	}
	else
	{
		CClientDC dc(nullptr);
		x = dc.GetDeviceCaps(LOGPIXELSX);
		y = dc.GetDeviceCaps(LOGPIXELSY);
	}
	m_dpi.x = static_cast<float>(x);
	m_dpi.y = static_cast<float>(y);
	m_size.width = (rect.right - rect.left) * 96 / m_dpi.x;
	m_size.height = (rect.bottom - rect.top) * 96 / m_dpi.y;
}

void CLayeredAnimationWndRenderImpDirectComposition::CreateDeviceResourcesEx(ID2D1DeviceContext* pDC)
{

}

BOOL CLayeredAnimationWndRenderImpDirectComposition::CanSupportAnimation() const
{
	return TRUE;
}

BOOL CLayeredAnimationWndRenderImpDirectComposition::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_PAINT:
		HandlePaint();
		break;
	case WM_SIZE:
		HandleSize(wParam, lParam);
		break;
	case WM_DPICHANGED:
		HandleDPIChanged(wParam, lParam);
		break;
	}
	return FALSE;
}

void CLayeredAnimationWndRenderImpDirectComposition::HandlePaint()
{

}

void CLayeredAnimationWndRenderImpDirectComposition::HandleSize(WPARAM wparam, LPARAM lparam)
{
	try
	{
		if (!IsDeviceCreated())
			return;
		if (SIZE_MINIMIZED == wparam)
			return;
		unsigned const width = LOWORD(lparam);
		unsigned const height = HIWORD(lparam);
		m_size.width = DPtoLP(width, m_dpi.x);
		m_size.height = DPtoLP(height, m_dpi.y);

		HR(m_device->CreateSurface(width,
			height,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			DXGI_ALPHA_MODE_PREMULTIPLIED,
			m_surface.ReleaseAndGetAddressOf()));
		HR(m_visual->SetContent(m_surface.Get()));
	}
	catch (ComException const&)
	{
		ReleaseDeviceResources();
	}
}

void CLayeredAnimationWndRenderImpDirectComposition::HandleDPIChanged(WPARAM wParam, LPARAM lParam)
{
	m_dpi.x = LOWORD(wParam);
	m_dpi.y = HIWORD(wParam);
	RECT const& rect = *reinterpret_cast<RECT const*>(lParam);
	VERIFY(m_pALAWnd->SetWindowPos(
		&CWnd::wndTop,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		SWP_NOACTIVATE));
}
