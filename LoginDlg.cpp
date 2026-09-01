#include "StdAfx.h"
#include "LoginDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "microsip.h"
#include "lib/utf.h"
#include "json.h"

static CString EscapeLoginJson(const CString& value)
{
	CString escaped;
	for (int i = 0; i < value.GetLength(); i++) {
		switch (value[i]) {
		case L'"': escaped += _T("\\\""); break;
		case L'\\': escaped += _T("\\\\"); break;
		case L'\n': escaped += _T("\\n"); break;
		case L'\r': escaped += _T("\\r"); break;
		case L'\t': escaped += _T("\\t"); break;
		default: escaped += value[i]; break;
		}
	}
	return escaped;
}

LoginDlg::LoginDlg(CWnd* pParent)
	: CBaseDialog(LoginDlg::IDD, pParent)
{
	loginSuccess = false;
}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGIN_USERNAME, m_username);
	DDX_Text(pDX, IDC_LOGIN_PASSWORD, m_password);
}

BEGIN_MESSAGE_MAP(LoginDlg, CBaseDialog)
	ON_BN_CLICKED(IDC_LOGIN_BTN, &LoginDlg::OnBnClickedLogin)
	ON_BN_CLICKED(IDCANCEL, &LoginDlg::OnBnClickedCancel)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

BOOL LoginDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	TranslateDialog(this->m_hWnd);

	SetIcon(theApp.LoadIcon(IDR_MAINFRAME), TRUE);
	SetIcon(theApp.LoadIcon(IDR_MAINFRAME), FALSE);

	m_username = _T("");
	m_password = _T("");

	UpdateData(FALSE);

	GetDlgItem(IDC_LOGIN_USERNAME)->SetFocus();

	return FALSE;
}

void LoginDlg::OnBnClickedLogin()
{
	UpdateData(TRUE);

	if (m_username.IsEmpty() || m_password.IsEmpty()) {
		AfxMessageBox(Translate(_T("Please enter username and password.")), MB_ICONWARNING);
		return;
	}

	CString jsonData;
	jsonData.Format(_T("{\"username\":\"%s\",\"password\":\"%s\"}"),
		(LPCTSTR)EscapeLoginJson(m_username),
		(LPCTSTR)EscapeLoginJson(m_password));

	CString url = _T("http://maxcare.local:3001/api/agent/login");
	CString headers = _T("Content-Type: application/json");

	URLGetAsyncData result = URLGetSync(url, true, jsonData, headers);

	if (result.statusCode == 200) {
		Json::Value response;
		Json::Reader reader;
		bool parsed = reader.parse((LPCSTR)CStringA(result.body), response);

		if (parsed && response.get("ok", false).asBool()) {
			Json::Value extension = response["extension"];
			CString extVal;
			if (extension.isString()) {
				extVal = MSIP::Utf8DecodeUni(extension.asCString());
			}
			else if (extension.isInt() || extension.isUInt()) {
				extVal.Format(_T("%d"), extension.asInt());
			}
			if (extVal.IsEmpty()) {
				AfxMessageBox(Translate(_T("The account has no assigned extension.")), MB_ICONERROR);
				return;
			}

			Json::Value displayName = response["name"];
			CString nameVal = displayName.isString() ? MSIP::Utf8DecodeUni(displayName.asCString()) : _T("");

			// SIP server from API response (falls back to maxcare.local)
			Json::Value sipServer = response["sip_server"];
			CString sipServerVal = sipServer.isString() && !sipServer.asString().empty() 
				? MSIP::Utf8DecodeUni(sipServer.asCString()) 
				: _T("maxcare.local");

			// الامتداد مؤقت لهذه الجلسة فقط؛ نحذف بيانات المستخدم السابق قبل التهيئة.
			accountSettings.AccountDelete(1);

			accountSettings.account.server = sipServerVal;
			accountSettings.account.port = 5060;
			accountSettings.account.username = extVal;
			accountSettings.account.password = m_password;
			accountSettings.account.authID = extVal;
			accountSettings.account.displayName = nameVal.IsEmpty() ? m_username : nameVal;
			accountSettings.account.domain = sipServerVal;
			accountSettings.account.rememberPassword = false;
			accountSettings.account.transport = _T("udp");
			accountSettings.accountId = 1;

			loginSuccess = true;
			EndDialog(IDOK);
			return;
		}
		else {
			AfxMessageBox(Translate(_T("Invalid username or password.")), MB_ICONERROR);
		}
	}
	else {
		AfxMessageBox(Translate(_T("Connection failed. Check server address.")), MB_ICONERROR);
	}
}

void LoginDlg::OnBnClickedCancel()
{
	EndDialog(IDCANCEL);
}

void LoginDlg::OnClose()
{
	EndDialog(IDCANCEL);
}
