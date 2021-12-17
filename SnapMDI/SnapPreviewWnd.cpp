#include "pch.h"
#include "framework.h"
#include "SnapPreviewWnd.h"
#include <algorithm>
#include <VersionHelpers.h>

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

#undef min
#undef max

class CSnapAnimationImp
{
public:
	CSnapAnimationImp() = default;
	virtual ~CSnapAnimationImp() = default;

	virtual BOOL Create(CWnd* pWndOwner) = 0;

	virtual BOOL CanSupportAnimation() const { return FALSE; }

	void OnAnimationTo(CRect rect)
	{
		if (IsAnimateByMovingWnd())
			m_pPreviewWnd->RepositionWindow(rect);
		else
			OnAnimationToUpdate(rect);
	}

	virtual void StartSnapping(const CRect& rectOwner) {}

	virtual void StopSnapping(bool bAbort)
	{
		//
	}

	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) = 0;
protected:
	virtual void OnAnimationToUpdate(CRect rect) {}

	inline bool IsAnimateByMovingWnd() const
	{
		if (m_pPreviewWnd->GetConfigFlags() & CSnapPreviewWnd::AnimateByMovingWnd)
			return true;
		return false;
	}
protected:
	friend class CSnapPreviewWnd;
	CSnapPreviewWnd* m_pPreviewWnd = nullptr;
};

class CSnapAnimationImpInvert : public CSnapAnimationImp
{
public:
	BOOL Create(CWnd* pWndOwner) override
	{
		CRect rect;
		rect.SetRectEmpty();

		DWORD dwStyle = WS_POPUP;
		DWORD dwExStyle = 0;
		UINT nClassStyle = CS_HREDRAW | CS_VREDRAW;
		return m_pPreviewWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);
	}

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
	{
		switch (message)
		{
		case WM_PAINT:
			HandlePaint();
			break;
		}
		return FALSE;
	}
private:
	void HandlePaint()
	{
		CPaintDC dc(m_pPreviewWnd);

		CRect rect;
		m_pPreviewWnd->GetClientRect(rect);

		COLORREF colorFill = RGB(47, 103, 190);

		CBrush brFill(CDrawingManager::PixelAlpha(RGB(255 - GetRValue(colorFill), 255 - GetGValue(colorFill), 255 - GetBValue(colorFill)), 50));

		CBrush* pBrushOld = dc.SelectObject(&brFill);
		dc.PatBlt(0, 0, rect.Width(), rect.Height(), PATINVERT);
		dc.SelectObject(pBrushOld);
	}
};

class CSnapAnimationImpAlpha : public CSnapAnimationImp
{
public:
	~CSnapAnimationImpAlpha()
	{
		if (m_bGDIPlusStarted)
		{
			GdiplusShutdown(m_gdiplusToken);
		}
	}

	BOOL Create(CWnd* pWndOwner) override
	{
		CRect rect;
		rect.SetRectEmpty();

		DWORD dwStyle = WS_POPUP;
		DWORD dwExStyle = WS_EX_LAYERED;
		UINT nClassStyle = 0;
		BOOL bOK = m_pPreviewWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);

		if (bOK)
		{
			if (IsAnimateByMovingWnd())
				m_pPreviewWnd->SetLayeredWindowAttributes(0, 128, LWA_ALPHA);
			else
			{
				GdiplusStartupInput gdiplusStartupInput;
				GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
				m_bGDIPlusStarted = true;
			}
		}
		return bOK;
	}

	BOOL CanSupportAnimation() const override { return TRUE; }

	void StartSnapping(const CRect& rectOwner) override
	{
		if (IsAnimateByMovingWnd())
		{
			__super::StartSnapping(rectOwner);
		}
		else
		{
			m_pPreviewWnd->SetWindowPos(&CWnd::wndTop, rectOwner.left, rectOwner.top, rectOwner.Width(), rectOwner.Height(),
				SWP_NOACTIVATE | SWP_NOREDRAW);

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
	}

	void OnAnimationToUpdate(CRect rect) override
	{
		if (!m_bmp.GetSafeHandle())
			return;
		CRect rectClient;
		m_pPreviewWnd->GetClientRect(rectClient);

		CPoint point(0, 0);
		CSize size(rectClient.Size());
		ASSERT(size == m_szBmp);

		CClientDC clientDC(m_pPreviewWnd);
		CDC dc;
		dc.CreateCompatibleDC(&clientDC);

		CBitmap* pBitmapOld = (CBitmap*)dc.SelectObject(&m_bmp);

		m_pPreviewWnd->ScreenToClient(rect);
		ZeroMemory(m_pBits, size.cx * size.cy * 4);
		{
			COLORREF crfFill = RGB(66, 143, 222);

			Gdiplus::Graphics gg(dc.GetSafeHdc());
			Gdiplus::Color color(128, GetRValue(crfFill), GetGValue(crfFill), GetBValue(crfFill));
			Gdiplus::SolidBrush brush(Gdiplus::Color(128, 0xcc, 0xcc, 0xcc));
			Gdiplus::Rect rc(rect.left, rect.top, rect.Width(), rect.Height());

			Gdiplus::GraphicsPath path;

			int diameter = GetSystemMetrics(SM_CXVSCROLL);
			Gdiplus::Rect rcArc(rc.X, rc.Y, diameter, diameter);

			// top left arc  
			path.AddArc(rcArc, 180, 90);

			// top right arc  
			rcArc.X = rect.right - diameter;
			path.AddArc(rcArc, 270, 90);

			// bottom right arc  
			rcArc.Y = rect.bottom - diameter;
			path.AddArc(rcArc, 0, 90);

			// bottom left arc 
			rcArc.X = rect.left;
			path.AddArc(rcArc, 90, 90);

			path.CloseFigure();

			gg.FillPath(&brush, &path);

			brush.SetColor(color);
			rc.Inflate(-diameter/2, -diameter/2);
			gg.FillRectangle(&brush, rc);
		}

		BLENDFUNCTION bf;
		bf.BlendOp = AC_SRC_OVER;
		bf.BlendFlags = 0;
		bf.SourceConstantAlpha = 255;
		bf.AlphaFormat = LWA_COLORKEY;

		m_pPreviewWnd->UpdateLayeredWindow(NULL, 0, &size, &dc, &point, 0, &bf, ULW_ALPHA);

		dc.SelectObject(pBitmapOld);

		if (!m_pPreviewWnd->IsWindowVisible())
		{
			m_pPreviewWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
				SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		}
	}

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
	{
		switch (message)
		{
		case WM_PAINT:
			HandlePaint();
			break;
		}
		return FALSE;
	}
private:
	void HandlePaint()
	{
		CPaintDC dc(m_pPreviewWnd);

		CRect rect;
		m_pPreviewWnd->GetClientRect(rect);

		COLORREF crfFill = RGB(66, 143, 222);

		CBrush brFill(crfFill);
		//dc.FillRect(rect, &brFill);
		int width = GetSystemMetrics(SM_CXVSCROLL);
		CSize sz(width, width);
		dc.DPtoLP(&sz);
		CPen pen(PS_SOLID, sz.cx, RGB(0xcc, 0xcc, 0xcc));
		auto oldBrush = dc.SelectObject(brFill.GetSafeHandle());
		auto oldPen = dc.SelectObject(pen.GetSafeHandle());
		dc.Rectangle(rect);

		dc.SelectObject(oldBrush);
		dc.SelectObject(oldPen);
	}

	CBitmap	m_bmp;
	LPBYTE	m_pBits = nullptr;
	CSize	m_szBmp;
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

class CSnapAnimationImpDirectComposition : public CSnapAnimationImp
{
	static PFN_D3D11_CREATE_DEVICE	procD3D11CreateDevice;
	static ProcD2D1CreateDevice		procD2D1CreateDevice;
	static ProcDCompositionCreateDevice2 procDCompositionCreateDevice2;
	static ProcGetDpiForMonitor procGetDpiForMonitor;
	static BOOL s_bApplicableCheck;
public:
	static BOOL IsApplicable()
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

	BOOL Create(CWnd* pWndOwner) override
	{
		CRect rect;
		rect.SetRectEmpty();

		DWORD dwStyle = WS_POPUP;
		DWORD dwExStyle = WS_EX_NOREDIRECTIONBITMAP;
		UINT nClassStyle = 0;
		BOOL bOK = m_pPreviewWnd->CreateEx(dwExStyle, AfxRegisterWndClass(nClassStyle), _T(""), dwStyle, rect, pWndOwner, NULL);

		return bOK;
	}

	struct ComException
	{
		HRESULT result;

		ComException(HRESULT const value) :
			result(value)
		{}
	};

	static void HR(HRESULT const result)
	{
		if (S_OK != result)
		{
			throw ComException(result);
		}
	}

	template <typename T>
	static float DPtoLP(T const pixel, float const dpi)
	{
		return pixel * 96.0f / dpi;
	}

	bool IsDeviceCreated() const
	{
		return m_device3d;
	}

	void ReleaseDeviceResources()
	{
		m_device3d.Reset();
	}

	void CreateDeviceResources()
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

		auto hWnd = m_pPreviewWnd->GetSafeHwnd();
		HR(m_device->CreateTargetForHwnd(hWnd,
			true, // Top most
			m_target.ReleaseAndGetAddressOf()));
		HR(m_device->CreateVisual(m_visual.ReleaseAndGetAddressOf()));
		RECT rect = {};
		m_pPreviewWnd->GetClientRect(&rect);

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
		D2D_COLOR_F const color = { 0.26f, 0.56f, 0.87f, 0.5f };
		HR(dc->CreateSolidColorBrush(color,
			m_brush.ReleaseAndGetAddressOf()));
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

	BOOL CanSupportAnimation() const override { return TRUE; }

	void StartSnapping(const CRect& rectOwner) override
	{
		if (IsAnimateByMovingWnd())
		{
			__super::StartSnapping(rectOwner);
		}
		else
		{
			m_pPreviewWnd->SetWindowPos(&CWnd::wndTop, rectOwner.left, rectOwner.top, rectOwner.Width(), rectOwner.Height(),
				SWP_NOACTIVATE | SWP_NOREDRAW);
		}
	}

	void OnAnimationToUpdate(CRect rect) override
	{
		m_rect = rect;
		m_pPreviewWnd->ScreenToClient(&m_rect);

		m_pPreviewWnd->SetWindowPos(&CWnd::wndTop, 0, 0, 0, 0,
			SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE);
		m_pPreviewWnd->RedrawWindow();
	}

	BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override
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

	void HandlePaint()
	{
		try
		{
			CreateDeviceResources();

			ComPtr<ID2D1DeviceContext> dc;
			POINT offset = {};
			HR(m_surface->BeginDraw(nullptr, // Entire surface
				__uuidof(dc),
				reinterpret_cast<void**>(dc.GetAddressOf()),
				&offset));

			dc->SetDpi(m_dpi.x, m_dpi.y);

			dc->SetTransform(Matrix3x2F::Translation(DPtoLP(offset.x, m_dpi.x),
				DPtoLP(offset.y, m_dpi.y)));

			dc->Clear();
			D2D_RECT_F rect;
			if (IsAnimateByMovingWnd())
			{
				rect = D2D_RECT_F{0, 0, m_size.width, m_size.height };
			}
			else
			{
				rect.left = DPtoLP(m_rect.left, m_dpi.x);
				rect.top = DPtoLP(m_rect.top, m_dpi.y);
				rect.right = DPtoLP(m_rect.right, m_dpi.y);
				rect.bottom = DPtoLP(m_rect.bottom, m_dpi.y);
			}
			//dc->FillRectangle(rect, m_brush.Get());
			D2D1_ROUNDED_RECT rRect;
			rRect.rect = rect;
			int width = GetSystemMetrics(SM_CXVSCROLL) / 2;
			auto gap = DPtoLP(width, m_dpi.x);
			
			rRect.radiusX = 10;
			rRect.radiusY = 10;
			
			m_brush->SetColor({0.8f, 0.8f, 0.8f, 0.5f});
			//dc->FillRectangle(rect, m_brush.Get());
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			m_brush->SetColor({ 0.26f, 0.56f, 0.87f, 0.5f });

			rRect.rect.left += gap;
			rRect.rect.top += gap;
			rRect.rect.right -= gap;
			rRect.rect.bottom -= gap;
			dc->FillRoundedRectangle(rRect, m_brush.Get());

			HR(m_surface->EndDraw());
			HR(m_device->Commit());
			m_pPreviewWnd->ValidateRect(nullptr);
		}
		catch (ComException const&)
		{
			ReleaseDeviceResources();
		}
	}

	void HandleSize(WPARAM wparam, LPARAM lparam)
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

	void HandleDPIChanged(WPARAM wParam, LPARAM lParam)
	{
		m_dpi.x = LOWORD(wParam);
		m_dpi.y = HIWORD(wParam);
		RECT const& rect = *reinterpret_cast<RECT const*>(lParam);
		VERIFY(m_pPreviewWnd->SetWindowPos(
			&CWnd::wndTop,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			SWP_NOACTIVATE));
	}
private:
	// Device independent resources
	ComPtr<ID2D1SolidColorBrush>	m_brush;
	D2D_SIZE_F						m_size;
	D2D_POINT_2F					m_dpi;
	
	RECT							m_rect;

	// Device resources
	ComPtr<ID3D11Device>			m_device3d;
	// The DirectComposition device
	ComPtr<IDCompositionDesktopDevice> m_device;
	ComPtr<IDCompositionTarget>  m_target;
	ComPtr<IDCompositionVisual2> m_visual;
	ComPtr<IDCompositionSurface> m_surface;
};

PFN_D3D11_CREATE_DEVICE	CSnapAnimationImpDirectComposition::procD3D11CreateDevice = nullptr;
ProcD2D1CreateDevice			CSnapAnimationImpDirectComposition::procD2D1CreateDevice = nullptr;
ProcDCompositionCreateDevice2	CSnapAnimationImpDirectComposition::procDCompositionCreateDevice2 = nullptr;
ProcGetDpiForMonitor	CSnapAnimationImpDirectComposition::procGetDpiForMonitor = nullptr;
BOOL	CSnapAnimationImpDirectComposition::s_bApplicableCheck = (BOOL)-1;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CSnapPreviewWnd::CSnapPreviewWnd(DWORD nConfigFlags)
{
	m_nConfigs = nConfigFlags;
}

CSnapPreviewWnd::~CSnapPreviewWnd()
{
}

void CSnapPreviewWnd::Create(CWnd* pWndOwner)
{
	ASSERT_VALID(pWndOwner);
	m_pWndOwner = pWndOwner;

	switch (GetAnimationTech())
	{
	case AnimationTechDC:
		if (CSnapAnimationImpDirectComposition::IsApplicable())
		{
			m_animation = std::make_unique<CSnapAnimationImpDirectComposition>();
			break;
		}
		// fall through
	case AnimationTechAlpha:
		if (GetGlobalData()->m_nBitsPerPixel > 8)
		{
			m_nFlags &= ~AnimationTechMask;
			m_nFlags |= AnimationTechAlpha;
			m_animation = std::make_unique<CSnapAnimationImpAlpha>();
			break;
		}
	case AnimationTechInvert:
	default:
		// fall through
		m_nFlags &= ~AnimationTechMask;
		m_nFlags |= AnimationTechInvert;
		m_animation = std::make_unique<CSnapAnimationImpInvert>();
		break;
	}
	ASSERT(m_animation);
	m_animation->m_pPreviewWnd = this;
	m_animation->Create(pWndOwner);
}

void CSnapPreviewWnd::StartSnapping()
{
	ASSERT(m_pWndOwner && !IsWindowVisible());
	m_pWndOwner->GetWindowRect(&m_rcOwner);
	if (m_animation)
	{
		m_animation->StartSnapping(m_rcOwner);
	}
}

void CSnapPreviewWnd::StopSnapping(bool bAbort)
{
	if (m_animation)
	{
		m_animation->StopSnapping(bAbort);
		StopAnimation();
		ShowWindow(SW_HIDE);
	}
}

void CSnapPreviewWnd::ShowAt(CWnd* pActiveSnapWnd, const CRect& rect)
{
	if (ShouldDoAnimation())
	{
		ASSERT(!m_pActiveSnapWnd || m_pActiveSnapWnd == pActiveSnapWnd);
		m_pActiveSnapWnd = pActiveSnapWnd;
		if (IsWindowVisible())
		{
			m_rectFrom = m_rectCur;
		}
		else
		{
			GetWindowInOwnerRect(pActiveSnapWnd, m_rectFrom);
			m_rectCur = m_rectFrom;
		}
		m_aniStage = AnimateStage::Showing;
		m_rectTo = rect;
		ScheduleAnimation();
	}
	else
	{
		m_rectTo = rect;
		RepositionWindow(rect);
	}
}

void CSnapPreviewWnd::Hide()
{
	if (ShouldDoAnimation())
	{
		m_aniStage = AnimateStage::Hiding;
		m_rectFrom = m_rectCur;
		ScheduleAnimation();
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
}

void CSnapPreviewWnd::GetSnapRect(CRect& rect) const
{
	rect = m_rectTo;
}

void CSnapPreviewWnd::RepositionWindow(const CRect& rect)
{
	SetWindowPos(&CWnd::wndTop, rect.left, rect.top, rect.Width(), rect.Height(),
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW);
	RedrawWindow();
}

void CSnapPreviewWnd::OnAnimationTo(const CRect& rect, bool bFinish)
{
	m_rectCur = rect;
	if (m_animation)
		m_animation->OnAnimationTo(rect);
	if (bFinish)
	{
		if (m_aniStage != AnimateStage::Showing)
		{
			ShowWindow(SW_HIDE);
			m_pActiveSnapWnd = nullptr;
			m_aniStage = AnimateStage::Ready;
		}
	}
}

void CSnapPreviewWnd::GetWindowInOwnerRect(CWnd* pWnd, CRect& rect) const
{
	pWnd->GetWindowRect(rect);
	CRect rcOwner;
	m_pWndOwner->GetWindowRect(rcOwner);
	rect.left = std::max(rect.left, rcOwner.left);
	rect.top = std::max(rect.top, rcOwner.top);
	rect.right = std::min(rect.right, rcOwner.right);
	rect.bottom = std::min(rect.bottom, rcOwner.bottom);
}

void CSnapPreviewWnd::StopAnimation()
{
	if (m_nAniTimerID)
	{
		KillTimer(m_nAniTimerID);
		m_nAniTimerID = 0;
	}
}

enum
{	AnimationInterval = 10
};

void CSnapPreviewWnd::ScheduleAnimation()
{
	m_AniStartTime = std::chrono::steady_clock::now();
	m_nAniTimerID = SetTimer(100, AnimationInterval, nullptr);
}

DWORD CSnapPreviewWnd::GetAnimationTech() const
{
	return (m_nConfigs & AnimationTechMask);
}

bool CSnapPreviewWnd::ShouldDoAnimation() const
{
	if (m_nConfigs & ConfigFlag::DisableAnimation)
		return false;
	if (!m_animation)
		return false;
	return m_animation->CanSupportAnimation();
}


BOOL CSnapPreviewWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if (m_animation)
	{
		if (m_animation->OnWndMsg(message, wParam, lParam, pResult))
			return TRUE;
	}
	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

BEGIN_MESSAGE_MAP(CSnapPreviewWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapPreviewWnd message handlers

BOOL CSnapPreviewWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

// How to programmatically move a window slowly, as if the user were doing it?
// see https://stackoverflow.com/questions/932706/how-to-programmatically-move-a-window-slowly-as-if-the-user-were-doing-it
inline static double SmoothMoveELX(double x)
{
	double dPI = atan(1) * 4;
	return (cos((1 - x) * dPI) + 1) / 2;
}

inline static LONG CalcSmoothPos(double pos, LONG from, LONG to)
{
	if (from == to || pos > 1.)
		return to;
	auto newPos = from * (1 - SmoothMoveELX(pos))
		+ to * SmoothMoveELX(pos);
	//auto newPos = from + (to - from) * pos;
	return (LONG)newPos;
}

void CSnapPreviewWnd::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != m_nAniTimerID)
		return;
	auto curTime = std::chrono::steady_clock::now();
	std::chrono::duration<double> timeDiff = curTime - m_AniStartTime;
	auto timeDiffCount = timeDiff.count();

	constexpr double duration = 0.15;

	if (m_aniStage == AnimateStage::Hiding)
	{
		GetWindowInOwnerRect(m_pActiveSnapWnd, m_rectTo);
	}

	CRect rect;
	bool bFinish = timeDiffCount >= duration;
	if (bFinish)
	{
		rect = m_rectTo;
	}
	else
	{
		auto dPos = (double)timeDiffCount / duration;
		rect.left = CalcSmoothPos(dPos, m_rectFrom.left, m_rectTo.left);
		rect.top = CalcSmoothPos(dPos, m_rectFrom.top, m_rectTo.top);
		rect.right = CalcSmoothPos(dPos, m_rectFrom.right, m_rectTo.right);
		rect.bottom = CalcSmoothPos(dPos, m_rectFrom.bottom, m_rectTo.bottom);
	}
	OnAnimationTo(rect, bFinish);
	if (bFinish)
		StopAnimation();
}

void CSnapPreviewWnd::OnDestroy()
{
	StopAnimation();
	__super::OnDestroy();
}

