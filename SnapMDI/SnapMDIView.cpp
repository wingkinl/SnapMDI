
// SnapMDIView.cpp : implementation of the CSnapMDIView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "SnapMDI.h"
#endif

#include "SnapMDIDoc.h"
#include "SnapMDIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSnapMDIView

IMPLEMENT_DYNCREATE(CSnapMDIView, CView)

BEGIN_MESSAGE_MAP(CSnapMDIView, CView)
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_TEST_PROP, OnUpdateTestProp)
	ON_COMMAND(ID_TEST_PROP, OnTestProp)
END_MESSAGE_MAP()

// CSnapMDIView construction/destruction

CSnapMDIView::CSnapMDIView() noexcept
{
}

CSnapMDIView::~CSnapMDIView()
{
}

BOOL CSnapMDIView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

// CSnapMDIView drawing

void CSnapMDIView::OnDraw(CDC* pDC)
{
	CSnapMDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rect;
	GetClientRect(rect);
	auto crf = pDoc->m_crfBackground;
	pDC->FillSolidRect(rect, crf);

	if (pDoc->m_bTestProp)
	{
		pDC->Ellipse(rect);
	}
	else
	{
		auto oldROP2 = pDC->SetROP2(R2_NOTXORPEN);
		pDC->MoveTo(rect.TopLeft());
		pDC->LineTo(rect.BottomRight());
		pDC->MoveTo(rect.right, rect.top);
		pDC->LineTo(rect.left, rect.bottom);
		pDC->SetROP2(oldROP2);
	}
}

BOOL CSnapMDIView::OnEraseBkgnd(CDC*)
{
	return TRUE;
}

void CSnapMDIView::OnUpdateTestProp(CCmdUI* pCmdUI)
{
	CSnapMDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pCmdUI->SetCheck(pDoc && pDoc->m_bTestProp);
}

void CSnapMDIView::OnTestProp()
{
	CSnapMDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;
	pDoc->m_bTestProp = !pDoc->m_bTestProp;
	Invalidate();
}

// CSnapMDIView diagnostics

#ifdef _DEBUG
void CSnapMDIView::AssertValid() const
{
	CView::AssertValid();
}

void CSnapMDIView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CSnapMDIDoc* CSnapMDIView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSnapMDIDoc)));
	return (CSnapMDIDoc*)m_pDocument;
}
#endif //_DEBUG


// CSnapMDIView message handlers
