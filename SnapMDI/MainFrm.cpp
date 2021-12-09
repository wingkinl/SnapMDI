
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.h"
#include "framework.h"
#include "SnapMDI.h"

#include "MainFrm.h"
#include "SnapPreviewWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CFloatFrameBase::AttachMDIChild(CMDIChildWnd* pWndChild, bool bMaximize)
{
#if 1
	CMDIChildWnd* pActiveChild = pWndChild;
	CDocument* pDocument;
	if (pActiveChild == NULL ||
		(pDocument = pActiveChild->GetActiveDocument()) == NULL)
	{
		TRACE(traceAppMsg, 0, "Warning: No active document for float command.\n");
		AfxMessageBox(AFX_IDP_COMMAND_FAILURE);
		return;     // command failed
	}

	CWinThread* pThread = AfxGetThread();
	ENSURE_VALID(pThread);
	CWnd* pMainWnd = pThread->m_pMainWnd;
	pThread->m_pMainWnd = this;

	// otherwise we have a new frame !
	CDocTemplate* pTemplate = pDocument->GetDocTemplate();
	ASSERT_VALID(pTemplate);
	CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDocument, pActiveChild);
	if (pFrame == NULL)
	{
		TRACE(traceAppMsg, 0, "Warning: failed to create new frame.\n");
		return;     // command failed
	}

	pTemplate->InitialUpdateFrame(pFrame, pDocument);

	pThread->m_pMainWnd = pMainWnd;

	pWndChild->MDIDestroy();

#else
	//pWndChild->ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0);
	//pWndChild->SetParent(CWnd::FromHandle(m_hWndMDIClient));
	ActivateFrame();
	auto pView = pWndChild->GetActiveView();

	CCreateContext context;
	context.m_pCurrentFrame = nullptr;
	context.m_pCurrentDoc = pView->GetDocument();
	context.m_pNewViewClass = RUNTIME_CLASS(CTestMDIView);

	auto pos = theApp.GetFirstDocTemplatePosition();
	context.m_pNewDocTemplate = theApp.GetNextDocTemplate(pos);

	auto pNewChildFrame = new CChildFrame;
	DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | WS_CHILD;
	LPCTSTR lpszClass = GetIconWndClass(dwDefaultStyle, IDR_TestMDITYPE);
	if (!pNewChildFrame->Create(lpszClass, _T("ttt"), dwDefaultStyle, rectDefault, this))
	{
		return;
	}

	//pNewChildFrame->ModifyStyle(WS_CAPTION | WS_THICKFRAME, 0);
	//pNewChildFrame->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);

	pView->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);

	auto pWnd = pNewChildFrame;
	//pView->SetParent(CWnd::FromHandle(m_hWndMDIClient));
	pView->SetParent(pNewChildFrame);
	pWndChild->MDIDestroy();

	if (bMaximize)
		pNewChildFrame->MDIMaximize();

	context.m_pNewDocTemplate->InitialUpdateFrame(pNewChildFrame, context.m_pCurrentDoc);

	//CRect rect;
	//GetClientRect(rect);
	//pWnd->SetWindowPos(nullptr, 0, 0, rect.Width(), rect.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
#endif
}

CFloatFrame::CFloatFrame(CMainFrame* pMain)
{
	m_pMainFrame = pMain;
}

CFloatFrame::~CFloatFrame()
{

}

BEGIN_MESSAGE_MAP(CFloatFrame, CFloatFrameBase)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL CFloatFrame::Create(CWnd* pWndParent)
{
	//UINT nClassStyle = CS_HREDRAW | CS_VREDRAW;
	//CString strClassName = ::AfxRegisterWndClass(nClassStyle,
	//	AfxGetApp()->LoadCursor(IDC_ARROW), NULL, nullptr);

	//DWORD dwStyle = WS_OVERLAPPEDWINDOW;
	//DWORD dwExStyle = 0;
	//CRect rect(100, 100, 1280, 720);
	//if (!CreateEx(dwExStyle, strClassName, _T("Test"), dwStyle,
	//	rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
	//	pWndParent->GetSafeHwnd(), nullptr))
	//{
	//	return FALSE;
	//}

	// attempt to create the window
	DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW;
	LPCTSTR lpszClass = GetIconWndClass(dwDefaultStyle, IDR_MAINFRAME);
	m_strTitle = _T("Demo");
	CString strTitle = m_strTitle;
	if (!CFloatFrameBase::Create(lpszClass, strTitle, dwDefaultStyle, rectDefault,
		pWndParent, 0, 0L, nullptr))
	{
		return FALSE;   // will self destruct on failure normally
	}

	auto pWndClient = CWnd::FromHandle(m_hWndMDIClient);
	pWndClient->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
	//SetMenu(nullptr);
	//SendMessageToDescendants(WM_INITIALUPDATE, 0, 0, TRUE, TRUE);

	ShowWindow(SW_SHOW);
	UpdateWindow();
	return TRUE;
}

BOOL CFloatFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	return CreateClient(lpcs, nullptr);
}

void CFloatFrame::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(rect);
	dc.FillSolidRect(&rect, RGB(171, 171, 171));
}

void CFloatFrame::OnSize(UINT nType, int cx, int cy)
{
	CFloatFrameBase::OnSize(nType, cx, cy);
// 	auto pMDIChild = MDIGetActive();
// 	if (pMDIChild->GetSafeHwnd())
// 	{
// 		pMDIChild->SetWindowPos(nullptr, 0, 0, cx, cy, SWP_NOMOVE|SWP_NOZORDER| SWP_NOACTIVATE);
// 	}
}

void CFloatFrame::OnClose()
{
	auto pMDIChild = MDIGetActive();
	if (pMDIChild)
	{
		m_pMainFrame->AttachMDIChild(pMDIChild, false);
	}

	CFloatFrameBase::OnClose();
}

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMainFrameBase)

BEGIN_MESSAGE_MAP(CMainFrame, CMainFrameBase)
	ON_WM_CREATE()
	ON_COMMAND(ID_TEST_FLOAT, OnTestFloat)
	ON_COMMAND(ID_TEST_SNAPPREVIEW, OnTestSnapPreview)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// CMainFrame construction/destruction

CMainFrame::CMainFrame() noexcept
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMainFrameBase::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);


	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMainFrameBase::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMainFrameBase::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMainFrameBase::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnTestFloat()
{
	auto pWnd = MDIGetActive();
	if (pWnd)
	{
		auto pFloatFrame = new CFloatFrame(this);
		pFloatFrame->Create(nullptr);
		pFloatFrame->AttachMDIChild(pWnd);
	}
}

void CMainFrame::OnTestSnapPreview()
{
	CRect rect;
	GetWindowRect(rect);
	rect.OffsetRect(10, 10);
	auto pSnapWnd = new CSnapPreviewWnd;
	pSnapWnd->Create(this);
	pSnapWnd->ShowAt(rect);
}
