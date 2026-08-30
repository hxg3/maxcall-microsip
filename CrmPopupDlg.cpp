#include "StdAfx.h"
#include "CrmPopupDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "lib/utf.h"
#include <afxinet.h>

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

	GetDlgItem(IDC_CRM_CALLER_NAME)->SetWindowText(callerName);
	GetDlgItem(IDC_CRM_CALLER_NUMBER)->SetWindowText(callerNumber);

	CRect screenRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
	CRect dlgRect;
	GetWindowRect(&dlgRect);
	int x = screenRect.right - dlgRect.Width() - 20;
	int y = screenRect.top + 20;
	SetWindowPos(&wndTopMost, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

	SetTimer(1, 30000, NULL);

	return TRUE;
}

void CrmPopupDlg::OnBnClickedSave()
{
	UpdateData(TRUE);

	CString server = accountSettings.account.server;
	CString url;
	url.Format(_T("http://%s:3001/api/callers"), server);

	CString postData;
	postData.Format(_T("phone=%s&name=%s&notes=%s"), callerNumber, callerName, notes);

	try {
		CInternetSession session;
		CHttpConnection* pHttp = NULL;
		CHttpFile* pFile = NULL;

		DWORD dwServiceType;
		CString strServer;
		CString strObject;
		INTERNET_PORT nPort;
		CString strUsername;
		CString strPassword;

		if (AfxParseURLEx(url, dwServiceType, strServer, strObject, nPort, strUsername, strPassword)) {
			pHttp = session.GetHttpConnection(strServer, 0, nPort);
			CStringA strFormData = Utf8EncodeUcs2(postData);
			pFile = pHttp->OpenRequest(CHttpConnection::HTTP_VERB_POST, strObject, 0, 1, 0, 0,
				INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE);

			CString headers = _T("Content-Type: application/x-www-form-urlencoded");
			pFile->SendRequest(headers, (LPVOID)strFormData.GetBuffer(), strFormData.GetLength());

			pFile->Close();
			session.Close();
		}
	}
	catch (CInternetException* e) {
		e->Delete();
	}

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
