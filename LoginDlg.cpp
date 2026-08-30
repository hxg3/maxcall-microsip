#include "StdAfx.h"
#include "LoginDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include <afxinet.h>

LoginDlg::LoginDlg(CWnd* pParent)
	: CBaseDialog(LoginDlg::IDD, pParent)
{
	loginSuccess = false;
	m_port = 5060;
	m_remember = TRUE;
}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LOGIN_USERNAME, m_username);
	DDX_Text(pDX, IDC_LOGIN_PASSWORD, m_password);
	DDX_Text(pDX, IDC_LOGIN_SERVER, m_server);
	DDX_Text(pDX, IDC_LOGIN_PORT, m_port);
	DDX_Check(pDX, IDC_LOGIN_REMEMBER, m_remember);
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

	m_server = accountSettings.account.server;
	m_port = _wtoi(accountSettings.account.domain);
	if (m_port <= 0) m_port = 5060;
	m_username = accountSettings.account.username;
	m_password = accountSettings.account.password;
	m_remember = accountSettings.account.rememberPassword;

	if (m_server.IsEmpty()) m_server = _T("192.168.1.165");
	if (m_port <= 0) m_port = 5060;

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

	if (m_server.IsEmpty()) {
		AfxMessageBox(Translate(_T("Please enter SIP server address.")), MB_ICONWARNING);
		return;
	}

	CString url;
	url.Format(_T("http://%s:3001/api/auth/login"), m_server);

	CString postData;
	postData.Format(_T("username=%s&password=%s"), m_username, m_password);

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
			bool status = pFile->SendRequest(headers, (LPVOID)strFormData.GetBuffer(), strFormData.GetLength());

			if (status) {
				DWORD statusCode;
				pFile->QueryInfoStatusCode(statusCode);

				CStringA buf;
				UINT len = 0;
				int i;
				do {
					LPSTR p = buf.GetBuffer(len + 1024);
					i = pFile->Read(p + len, 1024);
					len += i;
					buf.ReleaseBuffer(len);
				} while (i > 0);

				pFile->Close();
				session.Close();

				if (statusCode == 200) {
					accountSettings.account.server = m_server;
					accountSettings.account.domain.Format(_T("%d"), m_port);
					accountSettings.account.username = m_username;
					accountSettings.account.password = m_password;
					accountSettings.account.rememberPassword = m_remember ? 1 : 0;
					accountSettings.account.displayName = m_username;
					accountSettings.account.authID = m_username;

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
		else {
			AfxMessageBox(Translate(_T("Invalid server URL.")), MB_ICONERROR);
		}
	}
	catch (CInternetException* e) {
		AfxMessageBox(Translate(_T("Connection error. Check server address and try again.")), MB_ICONERROR);
		e->Delete();
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
