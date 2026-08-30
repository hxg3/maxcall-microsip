#include "StdAfx.h"
#include "CrmPopupDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "lib/utf.h"

CrmPopupDlg::CrmPopupDlg(CWnd* pParent)
	: CBaseDialog(CrmPopupDlg::IDD, pParent)
{
	call_id = PJSUA_INVALID_ID;
}

CrmPopupDlg::~CrmPopupDlg(void)
{
}

void CrmPopupDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CRM_CALLER_NAME, callerName);
	DDX_Text(pDX, IDC_CRM_NOTES, notes);
}

BEGIN_MESSAGE_MAP(CrmPopupDlg, CBaseDialog)
	ON_BN_CLICKED(IDC_CRM_SAVE, &CrmPopupDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_CRM_DISMISS, &CrmPopupDlg::OnBnClickedDismiss)
	ON_WM_CLOSE()
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CrmPopupDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	TranslateDialog(this->m_hWnd);

	CFont* font = this->GetFont();
	LOGFONT lf;
	font->GetLogFont(&lf);

	lf.lfHeight = 18;
	lf.lfWeight = FW_BOLD;
	m_fontName.CreateFontIndirect(&lf);
	GetDlgItem(IDC_CRM_CALLER_NAME)->SetFont(&m_fontName);

	lf.lfHeight = 11;
	lf.lfWeight = FW_NORMAL;
	m_fontLabel.CreateFontIndirect(&lf);
	GetDlgItem(IDC_CRM_CALLER_NUMBER)->SetFont(&m_fontLabel);

	GetDlgItem(IDC_CRM_CALLER_NUMBER)->SetWindowText(callerNumber);

	LoadCallerInfo();

	CRect screenRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
	CRect dlgRect;
	GetWindowRect(&dlgRect);
	int x = screenRect.right - dlgRect.Width() - 20;
	int y = screenRect.top + 20;
	SetWindowPos(&wndTopMost, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

	SetTimer(1, 120000, NULL);

	return TRUE;
}

void CrmPopupDlg::LoadCallerInfo()
{
	CString server = accountSettings.account.server;
	if (server.IsEmpty()) {
		UpdateData(FALSE);
		return;
	}

	CString url;
	url.Format(_T("http://%s:3001/api/callers/%s"), server, callerNumber);

	URLGetAsyncData result = URLGetSync(url);

	if (result.statusCode >= 200 && result.statusCode < 300 && !result.body.IsEmpty()) {
		CStringA bodyA = result.body;
		CStringA nameA, notesA;

		int nameIdx = bodyA.Find("\"name\":\"");
		if (nameIdx >= 0) {
			nameA = bodyA.Mid(nameIdx + 8);
			int endQuote = nameA.Find('"');
			if (endQuote >= 0)
				nameA = nameA.Left(endQuote);
		}

		int notesIdx = bodyA.Find("\"notes\":\"");
		if (notesIdx >= 0) {
			notesA = bodyA.Mid(notesIdx + 9);
			int endQuote = notesA.Find('"');
			if (endQuote >= 0)
				notesA = notesA.Left(endQuote);
		}

		if (!nameA.IsEmpty()) {
			callerName = nameA;
		}
		if (!notesA.IsEmpty()) {
			notes = notesA;
		}
	}

	UpdateData(FALSE);
}

void CrmPopupDlg::OnBnClickedSave()
{
	UpdateData(TRUE);

	CString server = accountSettings.account.server;
	if (server.IsEmpty()) {
		OnBnClickedDismiss();
		return;
	}

	CString url;
	url.Format(_T("http://%s:3001/api/callers"), server);

	CStringA phoneA = Utf8EncodeUcs2(callerNumber);
	CStringA nameA = Utf8EncodeUcs2(callerName);
	CStringA notesA = Utf8EncodeUcs2(notes);

	CStringA postData;
	postData.Format("{\"phone\":\"%s\",\"name\":\"%s\",\"notes\":\"%s\"}", phoneA, nameA, notesA);

	CString headers = _T("Content-Type: application/json");
	URLGetSync(url, true, CString(postData), headers);

	OnBnClickedDismiss();
}

void CrmPopupDlg::OnBnClickedDismiss()
{
	KillTimer(1);
	DestroyWindow();
}

void CrmPopupDlg::OnClose()
{
	OnBnClickedDismiss();
}

void CrmPopupDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		OnBnClickedDismiss();
	}
	CBaseDialog::OnTimer(nIDEvent);
}
