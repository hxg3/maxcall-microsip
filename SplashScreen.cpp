#include "stdafx.h"
#include "SplashScreen.h"
#include "define.h"

BEGIN_MESSAGE_MAP(CSplashScreen, CWnd)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_NCHITTEST()
END_MESSAGE_MAP()

CSplashScreen::CSplashScreen()
	: m_brTeal(MAXCARE_TEAL)
	, m_brSurface(MAXCARE_SURFACE)
{
}

CSplashScreen::~CSplashScreen()
{
}

BOOL CSplashScreen::Create(CWnd* pParentWnd)
{
	int width = 340;
	int height = 200;

	CRect screenRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
	int x = (screenRect.Width() - width) / 2;
	int y = (screenRect.Height() - height) / 2;

	if (!CreateEx(0, AfxRegisterWndClass(0), NULL,
		WS_POPUP | WS_VISIBLE | WS_EX_TOOLWINDOW,
		x, y, width, height,
		pParentWnd ? pParentWnd->GetSafeHwnd() : NULL, NULL))
	{
		return FALSE;
	}

	CDC* pDC = GetDC();
	int dpiY = pDC ? GetDeviceCaps(pDC->m_hDC, LOGPIXELSY) : 96;

	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = MulDiv(28, dpiY, 96);
	lf.lfWeight = FW_BOLD;
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
	m_fontTitle.CreateFontIndirect(&lf);

	lf.lfHeight = MulDiv(11, dpiY, 96);
	lf.lfWeight = FW_NORMAL;
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
	m_fontSub.CreateFontIndirect(&lf);

	lf.lfHeight = MulDiv(9, dpiY, 96);
	lf.lfWeight = FW_NORMAL;
	StringCchCopy(lf.lfFaceName, LF_FACESIZE, _T("Segoe UI"));
	m_fontLoading.CreateFontIndirect(&lf);

	if (pDC) ReleaseDC(pDC);

	ShowWindow(SW_SHOW);
	UpdateWindow();

	SetTimer(1, 2500, NULL);

	return TRUE;
}

void CSplashScreen::Close()
{
	KillTimer(1);
	DestroyWindow();
}

void CSplashScreen::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);

	dc.FillSolidRect(rect, MAXCARE_SURFACE);

	int headerH = 55;
	CRect headerRect(0, 0, rect.Width(), headerH);
	dc.FillSolidRect(headerRect, MAXCARE_TEAL);

	dc.SetBkMode(TRANSPARENT);

	HICON hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	if (hIcon) {
		DrawIconEx(dc.m_hDC, 18, 12, hIcon, 32, 32, 0, NULL, DI_NORMAL);
	}

	dc.SetTextColor(MAXCARE_WHITE);
	dc.SelectObject(&m_fontTitle);
	dc.DrawText(_T("MaxCall"), CRect(58, 8, rect.Width() - 10, 45), DT_LEFT | DT_VCENTER | DT_SINGLELINE);

	dc.SetTextColor(MAXCARE_GOLD_SOFT);
	dc.SelectObject(&m_fontSub);
	dc.DrawText(_T("MaxCare Hospital"), CRect(58, 38, rect.Width() - 10, 52), DT_LEFT | DT_SINGLELINE);

	dc.SetTextColor(MAXCARE_TEXT_MUTED);
	dc.SelectObject(&m_fontLoading);
	dc.DrawText(_T("Loading..."), CRect(0, headerH + 15, rect.Width(), headerH + 35), DT_CENTER | DT_SINGLELINE);

	int barW = 120;
	int barH = 3;
	int barX = (rect.Width() - barW) / 2;
	int barY = headerH + 45;
	dc.FillSolidRect(barX, barY, barW, barH, MAXCARE_BORDER);

	CRgn rgnBar;
	rgnBar.CreateRoundRectRgn(barX, barY, barX + barW, barY + barH, barH, barH);
	dc.SelectClipRgn(&rgnBar);
	dc.FillSolidRect(barX, barY, barW / 3, barH, MAXCARE_TEAL);
	dc.SelectClipRgn(NULL);
}

void CSplashScreen::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		KillTimer(1);
		AfxGetMainWnd()->PostMessage(WM_CLOSE, 0, 0);
		PostMessage(WM_CLOSE, 0, 0);
	}
}

LRESULT CSplashScreen::OnNcHitTest(CPoint point)
{
	return HTCAPTION;
}
