#include "StdAfx.h"
#include "LoginDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "microsip.h"
#include "lib/utf.h"

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

	CStringA jsonData;
	jsonData.Format("{\"username\":\"%s\",\"password\":\"%s\"}",
		(LPCSTR)Utf8EncodeUcs2(m_username.GetBuffer()),
		(LPCSTR)Utf8EncodeUcs2(m_password.GetBuffer()));
	m_username.ReleaseBuffer();
	m_password.ReleaseBuffer();

	CString url = _T("http://192.168.1.165:3001/api/agent/login");
	CString headers = _T("Content-Type: application/json");

	URLGetAsyncData result = URLGetSync(url, true, CString(jsonData), headers);

	if (result.statusCode == 200) {
		bool hasOk = (result.body.Find("\"ok\":true") >= 0)
			|| (result.body.Find("\"ok\": true") >= 0);

		if (hasOk) {
			CStringA extVal = "";
			const char* extPos = strstr(result.body, "\"extension\":\"");
			if (!extPos) extPos = strstr(result.body, "\"extension\": \"");
			if (extPos) {
				extPos += 13;
				const char* extEnd = strchr(extPos, '\"');
				if (extEnd) {
					extVal = CStringA(extPos, extEnd - extPos);
				}
			}

			CStringA nameVal = "";
			const char* namePos = strstr(result.body, "\"name\":\"");
			if (!namePos) namePos = strstr(result.body, "\"name\": \"");
			if (namePos) {
				namePos += 8;
				const char* nameEnd = strchr(namePos, '\"');
				if (nameEnd) {
					nameVal = CStringA(namePos, nameEnd - namePos);
				}
			}

			accountSettings.account.server = _T("192.168.1.165");
			accountSettings.account.port = 5060;
			accountSettings.account.username = CString(extVal);
			accountSettings.account.password = m_password;
			accountSettings.account.authID = CString(extVal);
			accountSettings.account.displayName = nameVal.IsEmpty() ? m_username : CString(nameVal);
			accountSettings.account.domain = _T("192.168.1.165");
			accountSettings.account.rememberPassword = true;
			accountSettings.account.transport = _T("udp");

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
