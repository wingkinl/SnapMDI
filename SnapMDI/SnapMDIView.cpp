
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
END_MESSAGE_MAP()

// CSnapMDIView construction/destruction

CSnapMDIView::CSnapMDIView() noexcept
{
	// TODO: add construction code here

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

void CSnapMDIView::OnDraw(CDC* /*pDC*/)
{
	CSnapMDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
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
