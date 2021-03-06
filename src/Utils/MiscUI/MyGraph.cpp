// MyGraph.cpp

#include "stdafx.h"
#include "MyGraph.h"
#include "BufferDC.h"

#include <cmath>
#include <memory>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// This macro can be called at the beginning and ending of every
// method.  It is identical to saying "ASSERT_VALID(); ASSERT_KINDOF();"
// but is written like this so that VALIDATE can be a macro.  It is useful
// as an "early warning" that something has gone wrong with "this" object.
#ifndef VALIDATE
	#ifdef _DEBUG
		#define VALIDATE		::AfxAssertValidObject(this, __FILE__ , __LINE__ ); \
									  _ASSERTE(IsKindOf(GetRuntimeClass()));
	#else
		#define VALIDATE
	#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Constants.

#define TICK_PIXELS								  4			// Size of tick marks.
#define GAP_PIXELS								  6			// Better if an even value.
#define LEGEND_COLOR_BAR_WIDTH_PIXELS			 50			// Width of color bar.
#define LEGEND_COLOR_BAR_GAP_PIXELS				  1			// Space between color bars.
#define Y_AXIS_MAX_TICK_COUNT					  5			// How many ticks on y axis.
#define MIN_FONT_SIZE							 70			// The minimum font-size in pt*10.
#define LEGEND_VISIBILITY_THRESHOLD				300			// The width of the graph in pixels when the legend gets hidden.

#define INTERSERIES_PERCENT_USED				0.85		// How much of the graph is 
															// used for bars/pies (the 
															// rest is for inter-series
															// spacing).

#define TITLE_DIVISOR							 5			// Scale font to graph width.
#define LEGEND_DIVISOR							 8			// Scale font to graph height.
#define Y_AXIS_LABEL_DIVISOR					 6			// Scale font to graph height.

const double PI = 3.1415926535897932384626433832795;

/////////////////////////////////////////////////////////////////////////////
// MyGraphSeries

// Constructor.
MyGraphSeries::MyGraphSeries(const CString& sLabel /* = "" */ )
	: m_sLabel(sLabel)
{
}

// Destructor.
/* virtual */ MyGraphSeries::~MyGraphSeries()
{
	for (int nGroup = 0; nGroup < m_oaRegions.GetSize(); ++nGroup) {
		delete m_oaRegions.GetAt(nGroup);
	}
}

//
void MyGraphSeries::SetLabel(const CString& sLabel)
{
	VALIDATE;

	m_sLabel = sLabel;
}

//
void MyGraphSeries::SetData(int nGroup, int nValue)
{
	VALIDATE;
	_ASSERTE(0 <= nGroup);

	m_dwaValues.SetAtGrow(nGroup, nValue);
}

//
void MyGraphSeries::SetTipRegion(int nGroup, const CRect& rc)
{
	VALIDATE;
	
	CRgn* prgnNew = new CRgn;
	ASSERT_VALID(prgnNew);

	VERIFY(prgnNew->CreateRectRgnIndirect(rc));
	SetTipRegion(nGroup, prgnNew);
}

//
void MyGraphSeries::SetTipRegion(int nGroup, CRgn* prgn)
{
	VALIDATE;
	_ASSERTE(0 <= nGroup);
	ASSERT_VALID(prgn);

	// If there is an existing region, delete it.
	CRgn* prgnOld = NULL;

	if (nGroup < m_oaRegions.GetSize()) 
	{
		prgnOld = m_oaRegions.GetAt(nGroup);
		ASSERT_NULL_OR_POINTER(prgnOld, CRgn);
	}

	if (prgnOld) {
		delete prgnOld;
		prgnOld = NULL;
	}

	// Add the new region.
	m_oaRegions.SetAtGrow(nGroup, prgn);

	_ASSERTE(m_oaRegions.GetSize() <= m_dwaValues.GetSize());
}

//
CString MyGraphSeries::GetLabel() const
{
	VALIDATE;

	return m_sLabel;
}

//
int MyGraphSeries::GetData(int nGroup) const
{
	VALIDATE;
	_ASSERTE(0 <= nGroup);
	_ASSERTE(m_dwaValues.GetSize() > nGroup);

	return m_dwaValues[nGroup];
}

// Returns the largest data value in this series.
int MyGraphSeries::GetMaxDataValue(bool bStackedGraph) const
{
	VALIDATE;

	int nMax(0);

	for (int nGroup = 0; nGroup < m_dwaValues.GetSize(); ++nGroup) {
		if(!bStackedGraph){
			nMax = max(nMax, static_cast<int> (m_dwaValues.GetAt(nGroup)));
		}
		else{
			nMax += static_cast<int> (m_dwaValues.GetAt(nGroup));
		}
	}

	return nMax;
}

// Returns the number of data points that are not zero.
int MyGraphSeries::GetNonZeroElementCount() const
{
	VALIDATE;

	int nCount(0);

	for (int nGroup = 0; nGroup < m_dwaValues.GetSize(); ++nGroup) {
		
		if (m_dwaValues.GetAt(nGroup)) {
			++nCount;
		}
	}

	return nCount;
}

// Returns the sum of the data points for this series.
int MyGraphSeries::GetDataTotal() const
{
	VALIDATE;

	int nTotal(0);

	for (int nGroup = 0; nGroup < m_dwaValues.GetSize(); ++nGroup) {
		nTotal += m_dwaValues.GetAt(nGroup);
	}

	return nTotal;
}

// Returns which group (if any) the sent point lies within in this series.
INT_PTR MyGraphSeries::HitTest(const CPoint& pt, int searchStart = 0) const
{
	VALIDATE;

	for (int nGroup = searchStart; nGroup < m_oaRegions.GetSize(); ++nGroup) {
		CRgn* prgnData = m_oaRegions.GetAt(nGroup);
		ASSERT_NULL_OR_POINTER(prgnData, CRgn);

		if (prgnData  &&  prgnData->PtInRegion(pt)) {
			return nGroup;
		}
	}

	return -1;
}

// Get the series portion of the tip for this group in this series.
CString MyGraphSeries::GetTipText(int nGroup) const
{
	VALIDATE;
	_ASSERTE(0 <= nGroup);
	_ASSERTE(m_oaRegions.GetSize() <= m_dwaValues.GetSize());
	
	CString sTip;

	sTip.Format(_T("%d (%d%%)"), m_dwaValues.GetAt(nGroup), 
		GetDataTotal() ? (int) (100.0 * (double) m_dwaValues.GetAt(nGroup) / 
		(double) GetDataTotal()) : 0);

	return sTip;
}


/////////////////////////////////////////////////////////////////////////////
// MyGraph

// Constructor.
MyGraph::MyGraph(GraphType eGraphType /* = MyGraph::Pie */ , bool bStackedGraph /* = false */)
	: m_nXAxisWidth(0)
	, m_nYAxisHeight(0)
	, m_eGraphType(eGraphType)
	, m_bStackedGraph(bStackedGraph)
{
	m_ptOrigin.x = m_ptOrigin.y = 0;
	m_rcGraph.SetRectEmpty();
	m_rcLegend.SetRectEmpty();
	m_rcTitle.SetRectEmpty();
}

// Destructor.
/* virtual */ MyGraph::~MyGraph()
{
}

BEGIN_MESSAGE_MAP(MyGraph, CStatic)
	//{{AFX_MSG_MAP(MyGraph)
	ON_WM_PAINT()
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnNeedText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnNeedText)
END_MESSAGE_MAP()

// Called by the framework to allow other necessary sub classing to occur 
// before the window is sub classed.
void MyGraph::PreSubclassWindow() 
{
	VALIDATE;

	CStatic::PreSubclassWindow();

	VERIFY(EnableToolTips(true));
}


/////////////////////////////////////////////////////////////////////////////
// MyGraph message handlers

// Handle the tooltip messages.  Returns true to mean message was handled.
BOOL MyGraph::OnNeedText(UINT /*uiId*/, NMHDR* pNMHDR, LRESULT* pResult) 
{
	_ASSERTE(pNMHDR  &&  "Bad parameter passed");
	_ASSERTE(pResult  &&  "Bad parameter passed");

	bool bReturn(false);
	UINT_PTR uiID(pNMHDR->idFrom);

	// Notification in NT from automatically created tooltip.
	if (0U != uiID) {
		bReturn = true;

		// Need to handle both ANSI and UNICODE versions of the message.
		TOOLTIPTEXTA* pTTTA = reinterpret_cast<TOOLTIPTEXTA*> (pNMHDR);
		ASSERT_POINTER(pTTTA, TOOLTIPTEXTA);

		TOOLTIPTEXTW* pTTTW = reinterpret_cast<TOOLTIPTEXTW*> (pNMHDR);
		ASSERT_POINTER(pTTTW, TOOLTIPTEXTW);

		CString sTipText(GetTipText());

#ifndef _UNICODE	
		if (TTN_NEEDTEXTA == pNMHDR->code) {
			lstrcpyn(pTTTA->szText, sTipText, sizeof(pTTTA->szText));
		}
		else {
			_mbstowcsz(pTTTW->szText, sTipText, sizeof(pTTTA->szText));
		}
#else
		if (pNMHDR->code == TTN_NEEDTEXTA) {
			_wcstombsz(pTTTA->szText, sTipText, sizeof(pTTTA->szText));
		}
		else {
			lstrcpyn(pTTTW->szText, sTipText, sizeof(pTTTA->szText));
		}
#endif
 
		*pResult = 0;
	}

	return bReturn;
}

// The framework calls this member function to determine whether a point is in
// the bounding rectangle of the specified tool.
INT_PTR MyGraph::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	_ASSERTE(pTI  &&  "Bad parameter passed");

	// This works around the problem of the tip remaining visible when you move 
	// the mouse to various positions over this control.
	INT_PTR nReturn(0);
	static bool bTipPopped(false);
	static CPoint ptPrev(-1,-1);

	if (point != ptPrev) {
		ptPrev = point;

		if (bTipPopped) {
			bTipPopped = false;
			nReturn = -1;
		}
		else {
			::Sleep(50);
			bTipPopped = true;

			pTI->hwnd = m_hWnd;
			pTI->uId = (UINT) m_hWnd;
			pTI->lpszText = LPSTR_TEXTCALLBACK;

			CRect rcWnd;
			GetClientRect(&rcWnd);
			pTI->rect = rcWnd;
			nReturn = 1;
		}
	}
	else {
		nReturn = 1;
	}

	MyGraph::SpinTheMessageLoop();

	return nReturn;
}

// Build the tip text for the part of the graph that the mouse is currently
// over.
CString MyGraph::GetTipText() const
{
	VALIDATE;

	CString sTip("");

	// Get the position of the mouse.
	CPoint pt;
	VERIFY(::GetCursorPos(&pt));
	ScreenToClient(&pt);

	// Ask each part of the graph to check and see if the mouse is over it.
	if (m_rcLegend.PtInRect(pt)) {
		sTip = "Legend";
	}
	else if (m_rcTitle.PtInRect(pt)) {
		sTip = "Title";
	}
	else {
		POSITION pos(m_olMyGraphSeries.GetHeadPosition());

		while (pos && sTip=="") {
			MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
			ASSERT_VALID(pSeries);

			INT_PTR nGroup(0);
			do{
				nGroup = pSeries->HitTest(pt,nGroup);

				if (-1 != nGroup) {
					if("" != sTip){
						sTip += _T(", ");
					}
					sTip += m_saLegendLabels.GetAt(nGroup) + _T(": ");
					sTip += pSeries->GetTipText(nGroup);
					nGroup++;
				}
			}while(-1 != nGroup);
		}
	}

	return sTip;
}
 
// Handle WM_PAINT.
void MyGraph::OnPaint() 
{
	VALIDATE;

	CBufferDC dc(this);
	DrawGraph(dc);
}

// Handle WM_SIZE.
void MyGraph::OnSize(UINT nType, int cx, int cy) 
{
	VALIDATE;

	CStatic::OnSize(nType, cx, cy);
	
	Invalidate();	
}

// Change the type of the graph; the caller should call Invalidate() on this
// window to make the effect of this change visible.
void MyGraph::SetGraphType(GraphType e, bool bStackedGraph)
{
	VALIDATE;

	m_eGraphType = e;
	m_bStackedGraph = bStackedGraph;
}

// Calculate the current max legend label length in pixels.
int MyGraph::GetMaxLegendLabelLength(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);

	CString sMax;
	int nMaxChars(-1);
	CSize siz(-1,-1);

	// First get max number of characters.
	for (int nGroup = 0; nGroup < m_saLegendLabels.GetSize(); ++nGroup) {
		int nLabelLength(m_saLegendLabels.GetAt(nGroup).GetLength());

		if (nMaxChars < nLabelLength) {
			nMaxChars = nLabelLength;
			sMax = m_saLegendLabels.GetAt(nGroup);
		}
	}

	// Now calculate the pixels.
	siz = dc.GetTextExtent(sMax);

	_ASSERTE(-1 < siz.cx);

	return siz.cx;
}

// Returns the largest number of data points in any series.
int MyGraph::GetMaxSeriesSize() const
{
	VALIDATE;

	int nMax(0);
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		nMax = max(nMax, pSeries->m_dwaValues.GetSize());
	}

	return nMax;
}

// Returns the largest number of non-zero data points in any series.
int MyGraph::GetMaxNonZeroSeriesSize() const
{
	VALIDATE;

	int nMax(0);
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		nMax = max(nMax, pSeries->GetNonZeroElementCount());
	}

	return nMax;
}

// Get the largest data value in all series.
int MyGraph::GetMaxDataValue() const
{
	VALIDATE;

	int nMax(0);
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		nMax = max(nMax, pSeries->GetMaxDataValue(m_bStackedGraph));
	}

	return nMax;
}

// How many series are populated?
int MyGraph::GetNonZeroSeriesCount() const
{
	VALIDATE;

	int nCount(0);
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		if (0 < pSeries->GetNonZeroElementCount()) {
			++nCount;
		}
	}

	return nCount;
}

// Returns the group number for the sent label; -1 if not found.
int MyGraph::LookupLabel(const CString& sLabel) const
{
	VALIDATE;
	_ASSERTE(! sLabel.IsEmpty());
	
	for (int nGroup = 0; nGroup < m_saLegendLabels.GetSize(); ++nGroup) {

		if (0 == sLabel.CompareNoCase(m_saLegendLabels.GetAt(nGroup))) {
			return nGroup;
		}
	}

	return -1;
}

void MyGraph::Clear()
{
	m_dwaColors.RemoveAll();
	m_saLegendLabels.RemoveAll();
	m_olMyGraphSeries.RemoveAll();
}

//
void MyGraph::AddSeries(MyGraphSeries& rMyGraphSeries)
{
	VALIDATE;
	ASSERT_VALID(&rMyGraphSeries);
	_ASSERTE(m_saLegendLabels.GetSize() == rMyGraphSeries.m_dwaValues.GetSize());
	
	m_olMyGraphSeries.AddTail(&rMyGraphSeries);
}

//
void MyGraph::SetXAxisLabel(const CString& sLabel)
{
	VALIDATE;
	_ASSERTE(! sLabel.IsEmpty());

	m_sXAxisLabel = sLabel;
}

//
void MyGraph::SetYAxisLabel(const CString& sLabel)
{
	VALIDATE;
	_ASSERTE(! sLabel.IsEmpty());

	m_sYAxisLabel = sLabel;
}

// Returns the group number added.  Also, makes sure that all the series have 
// this many elements.
int MyGraph::AppendGroup(const CString& sLabel)
{
	VALIDATE;

	// Add the group.
	int nGroup(m_saLegendLabels.GetSize());
	SetLegend(nGroup, sLabel);

	// Make sure that all series have this element.
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {

		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		if (nGroup >= pSeries->m_dwaValues.GetSize()) {
			pSeries->m_dwaValues.SetAtGrow(nGroup, 0);
		}
	}

	return nGroup;
}

// Set this value to the legend.
void MyGraph::SetLegend(int nGroup, const CString& sLabel)
{
	VALIDATE;
	_ASSERTE(0 <= nGroup);

	m_saLegendLabels.SetAtGrow(nGroup, sLabel);
}

//
void MyGraph::SetGraphTitle(const CString& sTitle)
{
	VALIDATE;
	_ASSERTE(! sTitle.IsEmpty());

	m_sTitle = sTitle;
}

//
void MyGraph::DrawGraph(CDC& dc)
{
	VALIDATE;
	ASSERT_VALID(&dc);

	if (GetMaxSeriesSize()) {
		dc.SetBkMode(TRANSPARENT);

		// Populate the colors as a group of evenly spaced colors of maximum
		// saturation.
		int nColorsDelta(240 / GetMaxSeriesSize());

		int baseColorL = 120;
		int diffColorL = 60;
		DWORD backgroundColor = ::GetSysColor(COLOR_WINDOW);
		// If graph is a non-stacked line graph, use darker colors if system window color is light.
		if (m_eGraphType == MyGraph::Line && !m_bStackedGraph) {
			int backgroundLuma = (GetRValue(backgroundColor) + GetGValue(backgroundColor) + GetBValue(backgroundColor)) / 3;
			if (backgroundLuma > 128) {
				baseColorL = 70;
				diffColorL = 50;
			}
		}
		
		for (WORD nGroup = 0; nGroup < GetMaxSeriesSize(); ++nGroup) {
			WORD colorH = (WORD)(nColorsDelta * nGroup);
			WORD colorL = (WORD)(baseColorL+(diffColorL*(nGroup%2)));
			WORD colorS = (WORD)(180)+(30*((1-nGroup%2)*(nGroup%3)));
			COLORREF cr(MyGraph::HLStoRGB(colorH, colorL, colorS));	// Populate colors cleverly
			m_dwaColors.SetAtGrow(nGroup, cr);
		}

		// Reduce the graphable area by the frame window and status bar.  We will
		// leave GAP_PIXELS pixels blank on all sides of the graph.  So top-left 
		// side of graph is at GAP_PIXELS,GAP_PIXELS and the bottom-right side 
		// of graph is at (m_rcGraph.Height() - GAP_PIXELS), (m_rcGraph.Width() -
		// GAP_PIXELS).  These settings are altered by axis labels and legends.
		CRect rcWnd;
		GetClientRect(&rcWnd);
		m_rcGraph.left = GAP_PIXELS;
		m_rcGraph.top = GAP_PIXELS;
		m_rcGraph.right = rcWnd.Width() - GAP_PIXELS;
		m_rcGraph.bottom = rcWnd.Height() - GAP_PIXELS;

		CBrush br;
		VERIFY(br.CreateSolidBrush(backgroundColor));
		dc.FillRect(rcWnd, &br);
		br.DeleteObject();

		// Draw graph title.
		DrawTitle(dc);

		// Set the axes and origin values.
		SetupAxes(dc);

		// Draw legend if there is one and there's enough space.
		if (m_saLegendLabels.GetSize() && m_rcGraph.right-m_rcGraph.left > LEGEND_VISIBILITY_THRESHOLD) {
			DrawLegend(dc);
		}
		else{
			m_rcLegend.SetRectEmpty();
		}

		// Draw axes unless it's a pie.
		if (m_eGraphType != MyGraph::PieChart) {
			DrawAxes(dc);
		}

		// Draw series data and labels.
		switch (m_eGraphType) {
			case MyGraph::Bar:  DrawSeriesBar(dc);  break;
			case MyGraph::Line: if (m_bStackedGraph) DrawSeriesLineStacked(dc); else DrawSeriesLine(dc); break;
			case MyGraph::PieChart:  DrawSeriesPie(dc);  break;
			default: _ASSERTE(! "Bad default case"); break;
		}
	}
}

// Draw graph title; size is proportionate to width.
void MyGraph::DrawTitle(CDC& dc)
{
	VALIDATE;
	ASSERT_VALID(&dc);

	// Create the title font.
	CFont fontTitle;
	VERIFY(fontTitle.CreatePointFont(max(m_rcGraph.Width() / TITLE_DIVISOR, MIN_FONT_SIZE),
		_T("Arial"), &dc));
	CFont* pFontOld = dc.SelectObject(&fontTitle);
	ASSERT_VALID(pFontOld);

	// Draw the title.
	m_rcTitle.SetRect(GAP_PIXELS, GAP_PIXELS, m_rcGraph.Width() + GAP_PIXELS,
		m_rcGraph.Height() + GAP_PIXELS);

	dc.DrawText(m_sTitle, m_rcTitle, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE |
		DT_TOP | DT_CALCRECT);

	m_rcTitle.right = m_rcGraph.Width() + GAP_PIXELS;

	dc.DrawText(m_sTitle, m_rcTitle, DT_CENTER | DT_NOPREFIX | DT_SINGLELINE | 
		DT_TOP);

	VERIFY(dc.SelectObject(pFontOld));
	fontTitle.DeleteObject();
}

// Set the axes and origin values.
void MyGraph::SetupAxes(CDC& dc)
{
	VALIDATE;
	ASSERT_VALID(&dc);

	// Since pie has no axis lines, set to full size minus GAP_PIXELS on each 
	// side.  These are needed for legend to plot itself.
	if (MyGraph::PieChart == m_eGraphType) {
		m_nXAxisWidth = m_rcGraph.Width() - (GAP_PIXELS * 2);
		m_nYAxisHeight = m_rcGraph.Height() - m_rcTitle.bottom;
		m_ptOrigin.x = GAP_PIXELS;
		m_ptOrigin.y = m_rcGraph.Height() - GAP_PIXELS;
	}
	else {
		// Bar and Line graphs.

		// Need to find out how wide the biggest Y-axis tick label is

		// Get and store height of axis label font.
		m_nAxisLabelHeight = max(m_rcGraph.Height() / Y_AXIS_LABEL_DIVISOR, MIN_FONT_SIZE);
		// Get and store height of tick label font.
		m_nAxisTickLabelHeight = max(int(m_nAxisLabelHeight*0.8), MIN_FONT_SIZE);

		CFont fontTickLabels;
		VERIFY(fontTickLabels.CreatePointFont(m_nAxisTickLabelHeight, _T("Arial"), &dc));
		// Select font and store the old.
		CFont* pFontOld = dc.SelectObject(&fontTickLabels);
		ASSERT_VALID(pFontOld);

		// Obtain tick label dimensions.
		CString sTickLabel;
		sTickLabel.Format(_T("%d"), GetMaxDataValue());
		CSize sizTickLabel(dc.GetTextExtent(sTickLabel));

		// Set old font object again and delete temporary font object.
		VERIFY(dc.SelectObject(pFontOld));
		fontTickLabels.DeleteObject();		

		// Determine axis specifications.
		m_ptOrigin.x = m_rcGraph.left + m_nAxisLabelHeight/10 + 2*GAP_PIXELS 
			+ sizTickLabel.cx + GAP_PIXELS + TICK_PIXELS;
		m_ptOrigin.y = m_rcGraph.bottom - m_nAxisLabelHeight/10 - 2*GAP_PIXELS - 
			sizTickLabel.cy - GAP_PIXELS - TICK_PIXELS;
		m_nYAxisHeight = m_ptOrigin.y - m_rcTitle.bottom - (2 * GAP_PIXELS);
		m_nXAxisWidth = (m_rcGraph.Width() - GAP_PIXELS) - m_ptOrigin.x;
	}
}

//
void MyGraph::DrawLegend(CDC& dc)
{
	VALIDATE;
	ASSERT_VALID(&dc);

	// Create the legend font.
	CFont fontLegend;
	int pointFontHeight = max(m_rcGraph.Height() / LEGEND_DIVISOR, MIN_FONT_SIZE);
	VERIFY(fontLegend.CreatePointFont(pointFontHeight, _T("Arial"), &dc));

	// Get the height of each label.
	LOGFONT lf;
	::SecureZeroMemory(&lf, sizeof(lf));
	VERIFY(fontLegend.GetLogFont(&lf));
	int nLabelHeight(abs(lf.lfHeight));

	// Get number of legend entries
	int nLegendEntries = max(1, GetMaxSeriesSize());

	// Calculate optimal label height = AvailableLegendHeight/AllAuthors
	// Use a buffer of (GAP_PIXELS / 2) on each side inside the legend, and in addition the same
	// gab above and below the legend frame, so in total 2*GAP_PIXELS
	double optimalLabelHeight = double(m_rcGraph.Height() - 2*GAP_PIXELS)/nLegendEntries;

	// Now relate the LabelHeight to the PointFontHeight
	int optimalPointFontHeight = int(pointFontHeight*optimalLabelHeight/nLabelHeight);

	// Limit the optimal PointFontHeight to the available range
	optimalPointFontHeight = min( max(optimalPointFontHeight, MIN_FONT_SIZE), pointFontHeight);

	// If the optimalPointFontHeight is different from the initial one, create a new legend font
	if (optimalPointFontHeight != pointFontHeight) {
		fontLegend.DeleteObject();
		VERIFY(fontLegend.CreatePointFont(optimalPointFontHeight, _T("Arial"), &dc));
		VERIFY(fontLegend.GetLogFont(&lf));
		nLabelHeight = abs(lf.lfHeight);
	}

	// Calculate maximum number of authors that can be shown with the current label height
	int nShownAuthors = (m_rcGraph.Height() - 2*GAP_PIXELS)/nLabelHeight - 1;
	// Fix rounding errors.
	if (nShownAuthors+1 == GetMaxSeriesSize()) 
		++nShownAuthors;

	// Get number of authors to be shown.
	nShownAuthors = min(nShownAuthors, GetMaxSeriesSize());
	// nShownAuthors contains now the number of authors

	CFont* pFontOld = dc.SelectObject(&fontLegend);
	ASSERT_VALID(pFontOld);

	// Determine actual size of legend.  A buffer of (GAP_PIXELS / 2) on each side, 
	// plus the height of each label based on the pint size of the font.
	int nLegendHeight = (GAP_PIXELS / 2) + (nShownAuthors * nLabelHeight) + (GAP_PIXELS / 2);
	// Draw the legend border.  Allow LEGEND_COLOR_BAR_PIXELS pixels for
	// display of label bars.
	m_rcLegend.top = (m_rcGraph.Height() - nLegendHeight) / 2;
	m_rcLegend.bottom = m_rcLegend.top + nLegendHeight;
	m_rcLegend.right = m_rcGraph.Width() - GAP_PIXELS;
	m_rcLegend.left = m_rcLegend.right - GetMaxLegendLabelLength(dc) - 
		LEGEND_COLOR_BAR_WIDTH_PIXELS;
	VERIFY(dc.Rectangle(m_rcLegend));

	int skipped_row = -1; // if != -1, this is the row that we show the ... in
	if (nShownAuthors < GetMaxSeriesSize())
		skipped_row = nShownAuthors-2;
	// Draw each group's label and bar.
	for (int nGroup = 0; nGroup < nShownAuthors; ++nGroup) {

		int nLabelTop(m_rcLegend.top + (nGroup * nLabelHeight) +
			(GAP_PIXELS / 2));

		int nShownGroup = nGroup; // introduce helper variable to avoid code duplication

		// Do we have a skipped row?
		if (skipped_row != -1) 
		{
			if (nGroup == skipped_row) {
				// draw the dots
				VERIFY(dc.TextOut(m_rcLegend.left + GAP_PIXELS, nLabelTop, _T("...") ));
				continue;
			}
			if (nGroup == nShownAuthors-1) {
				// we show the last group instead of the scheduled group
				nShownGroup = GetMaxSeriesSize()-1;
			}
		}
		// Draw the label.
		VERIFY(dc.TextOut(m_rcLegend.left + GAP_PIXELS, nLabelTop,
			m_saLegendLabels.GetAt(nShownGroup)));

		// Determine the bar.
		CRect rcBar;
		rcBar.left = m_rcLegend.left + GAP_PIXELS + GetMaxLegendLabelLength(dc) + GAP_PIXELS;
		rcBar.top = nLabelTop + LEGEND_COLOR_BAR_GAP_PIXELS;
		rcBar.right = m_rcLegend.right - GAP_PIXELS;
		rcBar.bottom = rcBar.top + nLabelHeight - LEGEND_COLOR_BAR_GAP_PIXELS;
		VERIFY(dc.Rectangle(rcBar));

		// Draw bar for group.
		COLORREF crBar(m_dwaColors.GetAt(nShownGroup));
		CBrush br(crBar);

		CBrush* pBrushOld = dc.SelectObject(&br);
		ASSERT_VALID(pBrushOld);

		rcBar.DeflateRect(LEGEND_COLOR_BAR_GAP_PIXELS, LEGEND_COLOR_BAR_GAP_PIXELS);
		dc.FillRect(rcBar, &br);

		dc.SelectObject(pBrushOld);
		br.DeleteObject();
	}

	VERIFY(dc.SelectObject(pFontOld));
	fontLegend.DeleteObject();
}

//
void MyGraph::DrawAxes(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);
	_ASSERTE(MyGraph::PieChart != m_eGraphType);

	dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

	// Draw y axis.
	dc.MoveTo(m_ptOrigin);  
	VERIFY(dc.LineTo(m_ptOrigin.x, m_ptOrigin.y - m_nYAxisHeight));

	// Draw x axis.
	dc.MoveTo(m_ptOrigin);  

	if (m_saLegendLabels.GetSize()) {

		VERIFY(dc.LineTo(m_ptOrigin.x + 
			(m_nXAxisWidth - m_rcLegend.Width() - (GAP_PIXELS * 2)), 
			m_ptOrigin.y));
	}
	else {
		VERIFY(dc.LineTo(m_ptOrigin.x + m_nXAxisWidth, m_ptOrigin.y));
	}

	// Note: m_nAxisLabelHeight and m_nAxisTickLabelHeight have been calculated in SetupAxis()

	// Create the x-axis label font.
	CFont fontXAxis;
	VERIFY(fontXAxis.CreatePointFont(m_nAxisLabelHeight, _T("Arial"), &dc));

	// Obtain the height of the font in device coordinates.
	LOGFONT pLF;
	VERIFY(fontXAxis.GetLogFont(&pLF));
	int fontHeightDC = pLF.lfHeight;

	// Create the y-axis label font.
	CFont fontYAxis;
	VERIFY(fontYAxis.CreateFont( 
		/* nHeight */ fontHeightDC,
		/* nWidth */ 0, 
		/* nEscapement */ 90 * 10, 
		/* nOrientation */ 0,
		/* nWeight */ FW_DONTCARE,	
		/* bItalic */ false, 
		/* bUnderline */ false,
		/* cStrikeOut */ 0, 
		ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, 
		PROOF_QUALITY, 
		VARIABLE_PITCH | FF_DONTCARE, 
		_T("Arial"))
	);

	// Set the y-axis label font and draw the label.
	CFont* pFontOld = dc.SelectObject(&fontYAxis);
	ASSERT_VALID(pFontOld);
	CSize sizYLabel(dc.GetTextExtent(m_sYAxisLabel));
	VERIFY(dc.TextOut(GAP_PIXELS, (m_rcGraph.Height() - sizYLabel.cy) / 2,
		m_sYAxisLabel));

	// Set the x-axis label font and draw the label.
	VERIFY(dc.SelectObject(&fontXAxis));
	CSize sizXLabel(dc.GetTextExtent(m_sXAxisLabel));
	VERIFY(dc.TextOut(m_ptOrigin.x + (m_nXAxisWidth - sizXLabel.cx) / 2,
		m_rcGraph.bottom - GAP_PIXELS - sizXLabel.cy, m_sXAxisLabel));

	// We hardwire TITLE_DIVISOR y-axis ticks here for simplicity.
	int nTickCount(min(Y_AXIS_MAX_TICK_COUNT, GetMaxDataValue()));
	int nTickSpace(m_nYAxisHeight / nTickCount);

	// create tick label font and set it in the device context
	CFont fontTickLabels;
	VERIFY(fontTickLabels.CreatePointFont(m_nAxisTickLabelHeight, _T("Arial"), &dc));
	VERIFY(dc.SelectObject(&fontTickLabels));

	for (int nTick = 0; nTick < nTickCount; ++nTick) {
		int nTickYLocation(m_ptOrigin.y - (nTickSpace * (nTick + 1)));
		dc.MoveTo(m_ptOrigin.x - TICK_PIXELS, nTickYLocation);
		VERIFY(dc.LineTo(m_ptOrigin.x + TICK_PIXELS, nTickYLocation));

		// Draw tick label.
		CString sTickLabel;
		sTickLabel.Format(_T("%d"), (GetMaxDataValue() * (nTick + 1)) / nTickCount);
		CSize sizTickLabel(dc.GetTextExtent(sTickLabel));
		
		VERIFY(dc.TextOut(m_ptOrigin.x - GAP_PIXELS - sizTickLabel.cx - TICK_PIXELS,
			nTickYLocation - sizTickLabel.cy/2, sTickLabel));
	}

	// Draw X axis tick marks.
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());
	int nSeries(0);

	while (pos) {
		
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		// Ignore unpopulated series if bar chart.
		if (m_eGraphType != MyGraph::Bar  || 
			0 < pSeries->GetNonZeroElementCount()) {

			// Get the spacing of the series.
			_ASSERTE(GetNonZeroSeriesCount()  &&  "Div by zero coming");
			int nSeriesSpace(0);

			if (m_saLegendLabels.GetSize()) {

				nSeriesSpace =
					(m_nXAxisWidth - m_rcLegend.Width() - (GAP_PIXELS * 2)) /
					(m_eGraphType == MyGraph::Bar ?
					GetNonZeroSeriesCount() : m_olMyGraphSeries.GetCount());
			}
			else {
				nSeriesSpace = m_nXAxisWidth / (m_eGraphType == MyGraph::Bar ?
					GetNonZeroSeriesCount() : m_olMyGraphSeries.GetCount());
			}

			int nTickXLocation(m_ptOrigin.x + ((nSeries + 1) * nSeriesSpace) -
				(nSeriesSpace / 2));

			dc.MoveTo(nTickXLocation, m_ptOrigin.y - TICK_PIXELS);
			VERIFY(dc.LineTo(nTickXLocation, m_ptOrigin.y + TICK_PIXELS));

			// Draw x-axis tick label.
			CString sTickLabel(pSeries->GetLabel());
			CSize sizTickLabel(dc.GetTextExtent(sTickLabel));

			VERIFY(dc.TextOut(nTickXLocation - (sizTickLabel.cx / 2),
				m_ptOrigin.y + TICK_PIXELS + GAP_PIXELS, sTickLabel));

			++nSeries;
		}
	}

	VERIFY(dc.SelectObject(pFontOld));
	fontXAxis.DeleteObject();
	fontYAxis.DeleteObject();
	fontTickLabels.DeleteObject();
}

//
void MyGraph::DrawSeriesBar(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);

	// How much space does each series get (includes inter series space)?
	// We ignore series whose members are all zero.
	int nSeriesSpace(0);

	if (m_saLegendLabels.GetSize()) {

		nSeriesSpace = (m_nXAxisWidth - m_rcLegend.Width() - (GAP_PIXELS * 2)) /
			GetNonZeroSeriesCount();
	}
	else {
		nSeriesSpace = m_nXAxisWidth / GetNonZeroSeriesCount();
	}

	// Determine width of bars.  Data points with a value of zero are assumed 
	// to be empty.  This is a bad assumption.
	int nBarWidth(0);

	// This is the width of the largest series (no inter series space).
	int nMaxSeriesPlotSize(0);

	if(!m_bStackedGraph){
		nBarWidth = nSeriesSpace / GetMaxNonZeroSeriesSize();
		if (1 < GetNonZeroSeriesCount()) {
			nBarWidth = (int) ((double) nBarWidth * INTERSERIES_PERCENT_USED);
		}
		nMaxSeriesPlotSize = GetMaxNonZeroSeriesSize() * nBarWidth;
	}
	else{
		nBarWidth = (int) ((double) nSeriesSpace * INTERSERIES_PERCENT_USED);
		nMaxSeriesPlotSize = nBarWidth;
	}

	// Iterate the series.
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());
	int nSeries(0);

	while (pos) {
		
		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		// Ignore unpopulated series.
		if (0 < pSeries->GetNonZeroElementCount()) {

			// Draw each bar; empty bars are not drawn.
			int nRunningLeft(m_ptOrigin.x + ((nSeries + 1) * nSeriesSpace) - 
				nMaxSeriesPlotSize);

			int stackAccumulator(0);

			for (int nGroup = 0; nGroup < GetMaxSeriesSize(); ++nGroup) {

				if (pSeries->GetData(nGroup)) {

					CRect rcBar;
					rcBar.left = nRunningLeft; 
					rcBar.top = (m_ptOrigin.y - (m_nYAxisHeight *
						pSeries->GetData(nGroup)) / GetMaxDataValue()) - stackAccumulator;
					// Make adjacent bar borders overlap, so there's only one pixel border line between them.
					rcBar.right = rcBar.left + nBarWidth + 1;
					rcBar.bottom = m_ptOrigin.y - stackAccumulator + 1;

					if(m_bStackedGraph){
						stackAccumulator = (m_ptOrigin.y - rcBar.top);
					}

					pSeries->SetTipRegion(nGroup, rcBar);

					COLORREF crBar(m_dwaColors.GetAt(nGroup));
					CBrush br(crBar);
					CBrush* pBrushOld = dc.SelectObject(&br);
					ASSERT_VALID(pBrushOld);

					VERIFY(dc.Rectangle(rcBar));
					dc.SelectObject(pBrushOld);
					br.DeleteObject();

					if(!m_bStackedGraph){
						nRunningLeft += nBarWidth;
					}
				}
			}

			++nSeries;
		}
	}
}

//
void MyGraph::DrawSeriesLine(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);
	_ASSERTE(!m_bStackedGraph);

	// Iterate the groups.
	CPoint ptLastLoc(0,0);
	int dataLastLoc(0);

	for (int nGroup = 0; nGroup < GetMaxSeriesSize(); nGroup++) {

		// How much space does each series get (includes inter series space)?
		int nSeriesSpace(0);

		if (m_saLegendLabels.GetSize()) {

			nSeriesSpace = (m_nXAxisWidth - m_rcLegend.Width() - (GAP_PIXELS * 2)) /
				m_olMyGraphSeries.GetCount();
		}
		else {
			nSeriesSpace = m_nXAxisWidth / m_olMyGraphSeries.GetCount();
		}

		// Determine width of bars.
		int nBarWidth(nSeriesSpace / GetMaxSeriesSize());

		if (1 < m_olMyGraphSeries.GetCount()) {
			nBarWidth = (int) ((double) nBarWidth * INTERSERIES_PERCENT_USED);
		}

		// This is the width of the largest series (no inter series space).
		//int nMaxSeriesPlotSize(GetMaxSeriesSize() * nBarWidth);

		// Iterate the series.
		POSITION pos(m_olMyGraphSeries.GetHeadPosition());
	
		// Build objects.
		COLORREF crLine(m_dwaColors.GetAt(nGroup));
		CBrush br(crLine);
		CBrush* pBrushOld = dc.SelectObject(&br);
		ASSERT_VALID(pBrushOld);
		CPen penLine(PS_SOLID, 1, crLine);
		CPen* pPenOld = dc.SelectObject(&penLine);
		ASSERT_VALID(pPenOld);

		for (int nSeries = 0; nSeries < m_olMyGraphSeries.GetCount(); ++nSeries) {

			MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
			ASSERT_VALID(pSeries);
			
			// Get x and y location of center of ellipse.
			CPoint ptLoc(0,0);
			
			ptLoc.x = m_ptOrigin.x + (((nSeries + 1) * nSeriesSpace) - 
				(nSeriesSpace / 2));
			
			double dLineHeight(pSeries->GetData(nGroup) * m_nYAxisHeight /
				GetMaxDataValue());

			ptLoc.y = (int) ((double) m_ptOrigin.y - dLineHeight);
			

			// Draw line back to last data member.
			if (nSeries > 0 && (pSeries->GetData(nGroup)!=0 || dataLastLoc != 0)) {

				dc.MoveTo(ptLastLoc.x, ptLastLoc.y - 1);
				VERIFY(dc.LineTo(ptLoc.x - 1, ptLoc.y - 1));
			}

			// Now draw ellipse.
			CRect rcEllipse(ptLoc.x - 3, ptLoc.y - 3, ptLoc.x + 3, ptLoc.y + 3);
			if(pSeries->GetData(nGroup)!=0){
				VERIFY(dc.Ellipse(rcEllipse));
			}
			if (m_olMyGraphSeries.GetCount() < 40)
			{
				pSeries->SetTipRegion(nGroup, rcEllipse);
			}

			// Save last pt and data
			ptLastLoc = ptLoc;
			dataLastLoc = pSeries->GetData(nGroup);
		}
		VERIFY(dc.SelectObject(pPenOld));
		penLine.DeleteObject();
		VERIFY(dc.SelectObject(pBrushOld));
		br.DeleteObject();
	}
}

//
void MyGraph::DrawSeriesLineStacked(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);
	_ASSERTE(m_bStackedGraph);

	CArray<int> stackAccumulator;
	stackAccumulator.SetSize(m_olMyGraphSeries.GetCount());

	CArray<CPoint> polygon;
	polygon.SetSize(m_olMyGraphSeries.GetCount() * 2);

	// How much space does each series get (includes inter series space)?
	int nSeriesSpace(0);
	if (m_saLegendLabels.GetSize()) {
		nSeriesSpace = (m_nXAxisWidth - m_rcLegend.Width() - (GAP_PIXELS * 2)) /
			m_olMyGraphSeries.GetCount();
	}
	else {
		nSeriesSpace = m_nXAxisWidth / m_olMyGraphSeries.GetCount();
	}

	// Determine width of bars.
	int nBarWidth(nSeriesSpace / GetMaxSeriesSize());
	if (1 < m_olMyGraphSeries.GetCount()) {
		nBarWidth = (int) ((double) nBarWidth * INTERSERIES_PERCENT_USED);
	}

	double dYScaling = double(m_nYAxisHeight) / GetMaxDataValue();
	int nSeriesCount = m_olMyGraphSeries.GetCount();

	// Iterate the groups.
	for (int nGroup = 0; nGroup < GetMaxSeriesSize(); nGroup++) {
	
		// Build objects.
		COLORREF crGroup(m_dwaColors.GetAt(nGroup));
		CBrush br(crGroup);
		CBrush* pBrushOld = dc.SelectObject(&br);
		ASSERT_VALID(pBrushOld);
		// For polygon outline, use average of this and previous color, and darken it.
		COLORREF crPrevGroup(nGroup > 0 ? m_dwaColors.GetAt(nGroup-1) : crGroup);
		COLORREF crOutline = RGB(
			(GetRValue(crGroup)+GetRValue(crPrevGroup))/3,
			(GetGValue(crGroup)+GetGValue(crPrevGroup))/3,
			(GetBValue(crGroup)+GetBValue(crPrevGroup))/3);
		CPen penLine(PS_SOLID, 1, crOutline);
		CPen* pPenOld = dc.SelectObject(&penLine);
		ASSERT_VALID(pPenOld);

		// Construct bottom part of polygon from current stack accumulator
		for (int nPolyBottom = 0; nPolyBottom < nSeriesCount; ++nPolyBottom) {
			CPoint ptLoc;
			ptLoc.x = m_ptOrigin.x + (((nPolyBottom + 1) * nSeriesSpace) - (nSeriesSpace / 2));
			double dLineHeight((stackAccumulator[nPolyBottom]) * dYScaling);
			ptLoc.y = (int) ((double) m_ptOrigin.y - dLineHeight);
			polygon[nSeriesCount-nPolyBottom-1] = ptLoc;
		}

		// Iterate the series, construct upper part of polygon and upadte stack accumulator
		POSITION pos(m_olMyGraphSeries.GetHeadPosition());
		for (int nSeries = 0; nSeries < nSeriesCount; ++nSeries) {

			MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
			ASSERT_VALID(pSeries);
			
			CPoint ptLoc;			
			ptLoc.x = m_ptOrigin.x + (((nSeries + 1) * nSeriesSpace) - 
				(nSeriesSpace / 2));
			double dLineHeight((pSeries->GetData(nGroup) + stackAccumulator[nSeries]) * dYScaling);			
			ptLoc.y = (int) ((double) m_ptOrigin.y - dLineHeight);
			polygon[nSeriesCount+nSeries] = ptLoc;

			stackAccumulator[nSeries] += pSeries->GetData(nGroup);
		}

		// Draw polygon
		VERIFY(dc.Polygon(polygon.GetData(), polygon.GetSize()));

		VERIFY(dc.SelectObject(pPenOld));
		penLine.DeleteObject();
		VERIFY(dc.SelectObject(pBrushOld));
		br.DeleteObject();
	}
}

//
void MyGraph::DrawSeriesPie(CDC& dc) const
{
	VALIDATE;
	ASSERT_VALID(&dc);
	_ASSERTE(0 < GetNonZeroSeriesCount()  &&  "Div by zero");

	// Determine width of pie display area (pie and space).
	int nSeriesSpace(0);

	if (m_saLegendLabels.GetSize()) {
		
		int nPieAndSpaceWidth((m_nXAxisWidth - m_rcLegend.Width() - 
			(GAP_PIXELS * 2)) / GetNonZeroSeriesCount());

		// Height is limiting factor.
		if (nPieAndSpaceWidth > m_nYAxisHeight - (GAP_PIXELS * 2)) {
			nSeriesSpace = (m_nYAxisHeight - (GAP_PIXELS * 2)) /
				GetNonZeroSeriesCount();
		}
		else {
			// Width is limiting factor.
			nSeriesSpace = nPieAndSpaceWidth;
		}
	}
	else {
		// No legend box.

		// Height is limiting factor.
		if (m_nXAxisWidth > m_nYAxisHeight) {
			nSeriesSpace = m_nYAxisHeight / GetNonZeroSeriesCount();
		}
		else {
			// Width is limiting factor.
			nSeriesSpace = m_nXAxisWidth / GetNonZeroSeriesCount();
		}
	}

	// Create font for labels.
	CFont fontLabels;
	int pointFontHeight = max(m_rcGraph.Height() / Y_AXIS_LABEL_DIVISOR, MIN_FONT_SIZE);
	VERIFY(fontLabels.CreatePointFont(pointFontHeight, _T("Arial"), &dc));
	CFont* pFontOld = dc.SelectObject(&fontLabels);
	ASSERT_VALID(pFontOld);

	// Draw each pie.
	int nPie(0);
	int nRadius((int) (nSeriesSpace * INTERSERIES_PERCENT_USED / 2.0));
	POSITION pos(m_olMyGraphSeries.GetHeadPosition());

	while (pos) {

		MyGraphSeries* pSeries = m_olMyGraphSeries.GetNext(pos);
		ASSERT_VALID(pSeries);

		// Don't leave a space for empty pies.
		if (0 < pSeries->GetNonZeroElementCount()) {

			// Locate this pie.
			CRect rcPie;
			rcPie.left = m_ptOrigin.x + GAP_PIXELS + (nSeriesSpace * nPie);
			rcPie.right = rcPie.left + (2 * nRadius);
			rcPie.top = (m_nYAxisHeight / 2) - nRadius;
			rcPie.bottom = (m_nYAxisHeight / 2) + nRadius;

			CPoint ptCenter((rcPie.left + rcPie.right) / 2,
				(rcPie.top + rcPie.bottom) / 2);

			// Draw series label.
			CSize sizPieLabel(dc.GetTextExtent(pSeries->GetLabel()));
			
			VERIFY(dc.TextOut((rcPie.left + nRadius) - (sizPieLabel.cx / 2),
			  ptCenter.y + nRadius + GAP_PIXELS, pSeries->GetLabel()));
			
			// How much do the wedges total to?
			double dPieTotal(pSeries->GetDataTotal());

			// Draw each wedge in this pie.
			CPoint ptStart(rcPie.left, ptCenter.y);
			double dRunningWedgeTotal(0.0);
			
			for (int nGroup = 0; nGroup < m_saLegendLabels.GetSize(); ++nGroup) {

				// Ignore empty wedges.
				if (0 < pSeries->GetData(nGroup)) {

					// Get the degrees of this wedge.
					dRunningWedgeTotal += pSeries->GetData(nGroup);
					double dPercent(dRunningWedgeTotal * 100.0 / dPieTotal);
					int nDegrees((int) (360.0 * dPercent / 100.0));

					// Find the location of the wedge's endpoint.
					CPoint ptEnd(WedgeEndFromDegrees(nDegrees, ptCenter, nRadius));

					// Special case: a wedge that takes up the whole pie would
					// otherwise be confused with an empty wedge.
					if (1 == pSeries->GetNonZeroElementCount()) {
						_ASSERTE(360 == nDegrees  &&  ptStart == ptEnd  &&  "This is the problem we're correcting");
						--ptEnd.y;
					}

					// If the wedge is of zero size, don't paint it!
					if (ptStart != ptEnd) {

						// Draw wedge.
						COLORREF crWedge(m_dwaColors.GetAt(nGroup));
						CBrush br(crWedge);
						CBrush* pBrushOld = dc.SelectObject(&br);
						ASSERT_VALID(pBrushOld);
						VERIFY(dc.Pie(rcPie, ptStart, ptEnd));

						// Create a region from the path we create.
						VERIFY(dc.BeginPath());
						VERIFY(dc.Pie(rcPie, ptStart, ptEnd));
						VERIFY(dc.EndPath());
						CRgn * prgnWedge = new CRgn;
						VERIFY(prgnWedge->CreateFromPath(&dc));
						pSeries->SetTipRegion(nGroup, prgnWedge);

						// Cleanup.
						dc.SelectObject(pBrushOld);
						br.DeleteObject();
						ptStart = ptEnd;
					}
				}
			}

			++nPie;
		}
	}

	// Draw X axis title after we know how many pies we actually have
	CSize sizXLabel(dc.GetTextExtent(m_sXAxisLabel));
	int nTotalSpaceOfPies = nSeriesSpace * nPie - (nSeriesSpace - nRadius*2);
	VERIFY(dc.TextOut(m_ptOrigin.x + GAP_PIXELS + (nTotalSpaceOfPies - sizXLabel.cx)/2,
		m_nYAxisHeight/2 + nRadius + GAP_PIXELS*2 + sizXLabel.cy, m_sXAxisLabel));

	VERIFY(dc.SelectObject(pFontOld));
	fontLabels.DeleteObject();
}

// Convert degrees to x and y coords.
CPoint MyGraph::WedgeEndFromDegrees(int nDegrees, const CPoint& ptCenter,
												int nRadius) const
{
	VALIDATE;

	CPoint pt;

	pt.x = (int) ((double) nRadius * cos((double) nDegrees / 360.0 * PI * 2.0));
	pt.x = ptCenter.x - pt.x;

	pt.y = (int) ((double) nRadius * sin((double) nDegrees / 360.0 * PI * 2.0));
	pt.y = ptCenter.y + pt.y;

	return pt;
}

// Spin The Message Loop: C++ version.  See "Advanced Windows Programming", 
// M. Heller, p. 153, and the MS TechNet CD, PSS ID Number: Q99999.
/* static */ UINT MyGraph::SpinTheMessageLoop(bool bNoDrawing /* = false */ ,
															 bool bOnlyDrawing /* = false */ ,
															 UINT uiMsgAllowed /* = WM_NULL */ )
{
	MSG msg;
	::SecureZeroMemory(&msg, sizeof(msg));

	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

		// Do painting only.
		if (bOnlyDrawing  &&  WM_PAINT == msg.message)  {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			// Update user interface.
			AfxGetApp()->OnIdle(0);
		}
		// Do everything *but* painting.
		else if (bNoDrawing  &&  WM_PAINT == msg.message)  {
			break;
		}
		// Special handling for this message.
		else if (WM_QUIT == msg.message) {
			::PostQuitMessage(static_cast<int>(msg.wParam));
			break;
		}
		// Allow one message (like WM_LBUTTONDOWN).
		else if (uiMsgAllowed == msg.message
		  &&  ! AfxGetApp()->PreTranslateMessage(&msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			break;
		}
		// This is the general case.
		else if (! bOnlyDrawing  &&  ! AfxGetApp()->PreTranslateMessage(&msg)) {
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);

			// Update user interface, then free temporary objects.
			AfxGetApp()->OnIdle(0);
			AfxGetApp()->OnIdle(1);
		}
	}

	return msg.message;
}


/////////////////////////////////////////////////////////////////////////////
// Conversion routines: RGB to HLS (Red-Green-Blue to Hue-Luminosity-Saturation).
// See Microsoft KnowledgeBase article Q29240.

#define  HLSMAX   240		// H,L, and S vary over 0-HLSMAX
#define  RGBMAX   255		// R,G, and B vary over 0-RGBMAX
									// HLSMAX BEST IF DIVISIBLE BY 6
									// RGBMAX, HLSMAX must each fit in a byte (255).

#define  UNDEFINED  (HLSMAX * 2 / 3)		// Hue is undefined if Saturation is 0 
														// (grey-scale).  This value determines 
														// where the Hue scrollbar is initially 
														// set for achromatic colors.


// Convert HLS to RGB.
/* static */ COLORREF MyGraph::HLStoRGB(WORD wH, WORD wL, WORD wS)
{
	_ASSERTE(0 <= wH  &&  240 >= wH  &&  "Illegal hue value");
	_ASSERTE(0 <= wL  &&  240 >= wL  &&  "Illegal lum value");
	_ASSERTE(0 <= wS  &&  240 >= wS  &&  "Illegal sat value");

	WORD wR(0);
	WORD wG(0);
	WORD wB(0);
	
	// Achromatic case.
	if (0 == wS) {
		wR = wG = wB = (wL * RGBMAX) / HLSMAX;

		if (UNDEFINED != wH) {
			_ASSERTE(! "ERROR");
		}
	}
	else {
		// Chromatic case.
		WORD Magic1(0);
		WORD Magic2(0);

		// Set up magic numbers.
		if (wL <= HLSMAX / 2) {
			Magic2 = (wL * (HLSMAX + wS) + (HLSMAX / 2)) / HLSMAX;
		}
		else {
			Magic2 = wL + wS - ((wL * wS) + (HLSMAX / 2)) / HLSMAX;
		}

		Magic1 = 2 * wL - Magic2;
		
		// Get RGB, change units from HLSMAX to RGBMAX.
		wR = (HueToRGB(Magic1, Magic2, wH + (HLSMAX / 3)) * RGBMAX + (HLSMAX / 2)) / HLSMAX;
		wG = (HueToRGB(Magic1, Magic2, wH)                * RGBMAX + (HLSMAX / 2)) / HLSMAX;
		wB = (HueToRGB(Magic1, Magic2, wH - (HLSMAX / 3)) * RGBMAX + (HLSMAX / 2)) / HLSMAX;
	}
	
	return RGB(wR,wG,wB);
}

// Utility routine for HLStoRGB.
/* static */ WORD MyGraph::HueToRGB(WORD w1, WORD w2, WORD wH)
{
	// Range check: note values passed add/subtract thirds of range.
	if (wH < 0) {
		wH += HLSMAX;
	}

	if (wH > HLSMAX) {
		wH -= HLSMAX;
	}

	// Return r, g, or b value from this tridrant.
	if (wH < HLSMAX / 6) {
		return w1 + (((w2 - w1) * wH + (HLSMAX / 12)) / (HLSMAX / 6));
	}

	if (wH < HLSMAX / 2) {
		return w2;
	}

	if (wH < (HLSMAX * 2) / 3) {
		return w1 + (((w2 - w1) * (((HLSMAX * 2) / 3) - wH) + (HLSMAX / 12)) / (HLSMAX / 6));
	}
	else {
		return w1;
	}
}

