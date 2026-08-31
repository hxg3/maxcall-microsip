#include "StdAfx.h"
#include "CrmPopupDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "lib/utf.h"
#include "json.h"

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

static CString JsonStringToCString(const Json::Value& value)
{
	if (!value.isString()) {
		return _T("");
	}

	std::string utf8 = value.asString();
	wchar_t* ucs2 = NULL;
	Utf8DecodeCP((char*)utf8.c_str(), CP_UTF8, &ucs2);
	CString res;
	if (ucs2) {
		res = ucs2;
		free(ucs2);
	}
	return res;
}

static CString EscapeJson(const CString& input)
{
	CString escaped;
	for (int i = 0; i < input.GetLength(); i++) {
		wchar_t c = input[i];
		if (c == L'"') escaped += _T("\\\"");
		else if (c == L'\\') escaped += _T("\\\\");
		else if (c == L'\b') escaped += _T("\\b");
		else if (c == L'\f') escaped += _T("\\f");
		else if (c == L'\n') escaped += _T("\\n");
		else if (c == L'\r') escaped += _T("\\r");
		else if (c == L'\t') escaped += _T("\\t");
		else if (c < 0x20) escaped.AppendFormat(_T("\\u%04x"), c);
		else escaped += c;
	}
	return escaped;
}

static CString UrlEncodeCallerNumber(const CString& number)
{
	char* utf8 = Utf8EncodeUcs2(number);
	CStringA encoded = urlencode(utf8 ? CStringA(utf8) : CStringA());
	if (utf8) {
		free(utf8);
	}
	return CString(encoded);
}

void CrmPopupDlg::LoadCallerInfo()
{
	CString server = accountSettings.account.server;
	if (server.IsEmpty()) {
		UpdateData(FALSE);
		return;
	}

	CString url;
	url.Format(_T("http://%s:3001/api/callers/%s"), server, UrlEncodeCallerNumber(callerNumber));

	URLGetAsyncData result = URLGetSync(url);

	if (result.statusCode >= 200 && result.statusCode < 300 && !result.body.IsEmpty()) {
		Json::Value caller;
		Json::Reader reader;
		if (reader.parse((LPCSTR)result.body, caller)) {
			CString savedName = JsonStringToCString(caller["name"]);
			if (!savedName.IsEmpty()) {
				callerName = savedName;
			}
			if (caller["notes"].isString()) {
				notes = JsonStringToCString(caller["notes"]);
			}
		}
	}

	UpdateData(FALSE);
}

void CrmPopupDlg::OnBnClickedSave()
{
	UpdateData(TRUE);

	CString server = accountSettings.account.server;
	if (server.IsEmpty()) {
		AfxMessageBox(Translate(_T("Unable to save caller details. Check the server connection and try again.")), MB_ICONERROR);
		return;
	}

	CString url;
	url.Format(_T("http://%s:3001/api/callers"), server);

	CString phone = EscapeJson(callerNumber);
	CString name = EscapeJson(callerName);
	CString callerNotes = EscapeJson(notes);

	CString postData;
	postData.Format(_T("{\"phone\":\"%s\",\"name\":\"%s\",\"notes\":\"%s\"}"), phone, name, callerNotes);

	CString headers = _T("Content-Type: application/json; charset=utf-8");
	URLGetAsyncData result = URLGetSync(url, true, postData, headers);
	if (result.statusCode < 200 || result.statusCode >= 300) {
		AfxMessageBox(Translate(_T("Unable to save caller details. Check the server connection and try again.")), MB_ICONERROR);
		return;
	}

	OnBnClickedDismiss();
}

void CrmPopupDlg::OnBnClickedDismiss()
{
	KillTimer(1);
	UpdateData(TRUE);
	ShowWindow(SW_HIDE);
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

void CrmPopupDlg::Restore()
{
	ShowWindow(SW_SHOWNORMAL);
	SetForegroundWindow();
	SetTimer(1, 120000, NULL);
}
