#include "StdAfx.h"
#include "CrmPopupDlg.h"
#include "langpack.h"
#include "mainDlg.h"
#include "settings.h"
#include "global.h"
#include "lib/utf.h"
#include "json.h"

// WebView2 headers (only in .cpp to avoid NTDDI_VERSION issues)
#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"
#include "WebView2Environment.h"

using namespace Microsoft::WRL;

CrmPopupDlg::CrmPopupDlg(CWnd* pParent)
	: CBaseDialog(CrmPopupDlg::IDD, pParent)
{
	call_id = PJSUA_INVALID_ID;
	m_controller = nullptr;
	m_webView = nullptr;
	m_env = nullptr;
}

CrmPopupDlg::~CrmPopupDlg(void)
{
	if (m_controller) {
		auto ctrl = static_cast<ICoreWebView2Controller*>(m_controller);
		ctrl->Close();
		ctrl->Release();
		m_controller = nullptr;
	}
	if (m_webView) {
		static_cast<ICoreWebView2*>(m_webView)->Release();
		m_webView = nullptr;
	}
	if (m_env) {
		static_cast<ICoreWebView2Environment*>(m_env)->Release();
		m_env = nullptr;
	}
}

void CrmPopupDlg::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CrmPopupDlg, CBaseDialog)
	ON_WM_SIZE()
	ON_MESSAGE(WM_WEBVIEW_READY, &CrmPopupDlg::OnWebViewReady)
	ON_MESSAGE(WM_WEBVIEW_MESSAGE, &CrmPopupDlg::OnWebViewMessage)
END_MESSAGE_MAP()

BOOL CrmPopupDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	TranslateDialog(this->m_hWnd);

	// Set window background to MaxCare surface color
	SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(MAXCARE_SURFACE));

	// Position at top-right of screen
	CRect screenRect;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);
	CRect dlgRect;
	GetWindowRect(&dlgRect);
	int x = screenRect.right - dlgRect.Width() - 20;
	int y = screenRect.top + 20;
	SetWindowPos(&wndTopMost, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);

	// Initialize WebView2
	InitWebView2();

	SetTimer(1, 120000, NULL);

	return TRUE;
}

void CrmPopupDlg::InitWebView2()
{
	// Create WebView2 environment
	auto callback = Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
		[this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
			if (SUCCEEDED(result) && env) {
				m_env = env;
				env->AddRef();
				// Create WebView2 controller
				auto ctrlCallback = Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
					[this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
						if (SUCCEEDED(result) && controller) {
							m_controller = controller;
							controller->AddRef();
							ICoreWebView2* wv = nullptr;
							controller->get_CoreWebView2(&wv);
							if (wv) {
								m_webView = wv;
								// Navigate to the HTML file
								CString htmlPath;
								TCHAR modulePath[MAX_PATH];
								GetModuleFileName(NULL, modulePath, MAX_PATH);
								CString path(modulePath);
								int pos = path.ReverseFind(_T('\\'));
								if (pos >= 0) {
									htmlPath = path.Left(pos + 1) + _T("crm_popup.html");
								}
								CStringW wPath(htmlPath);
								wv->Navigate(wPath.GetString());
							}
						}
						return S_OK;
					});
				RECT bounds;
				GetClientRect(&bounds);
				env->CreateCoreWebView2Controller(m_hWnd, ctrlCallback.Get(), bounds);
			}
			return S_OK;
		});

	ComPtr<ICoreWebView2EnvironmentFactory> factory;
	HRESULT hr = GetWebView2Factory(&factory);
	if (SUCCEEDED(hr) && factory) {
		factory->CreateWebView2Environment(callback.Get());
	}
}

LRESULT CrmPopupDlg::OnWebViewReady(WPARAM wParam, LPARAM lParam)
{
	// WebView is ready, update with caller data
	UpdateWebView();
	return 0;
}

LRESULT CrmPopupDlg::OnWebViewMessage(WPARAM wParam, LPARAM lParam)
{
	CString* msg = reinterpret_cast<CString*>(lParam);
	if (msg) {
		ProcessWebViewMessage(*msg);
		delete msg;
	}
	return 0;
}

void CrmPopupDlg::ProcessWebViewMessage(CString& message)
{
	// Parse JSON message from JavaScript
	Json::Value root;
	Json::Reader reader;
	std::string utf8 = CT2A(message, CP_UTF8);
	if (reader.parse(utf8, root)) {
		CString action = JsonStringToCString(root["action"]);
		if (action == _T("save")) {
			callerName = JsonStringToCString(root["name"]);
			notes = JsonStringToCString(root["notes"]);
			SaveCallerInfo();
		}
		else if (action == _T("dismiss")) {
			OnBnClickedDismiss();
		}
		else if (action == _T("answer")) {
			OnBnClickedAnswer();
		}
	}
}

void CrmPopupDlg::LoadCallerInfo()
{
	CString server = accountSettings.account.server;
	if (server.IsEmpty()) {
		UpdateWebView();
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

	UpdateWebView();
}

void CrmPopupDlg::UpdateWebView()
{
	if (!m_webView) return;

	// Escape strings for JavaScript
	CString escapedNumber = EscapeJson(callerNumber);
	CString escapedName = EscapeJson(callerName);
	CString escapedNotes = EscapeJson(notes);

	CString js;
	js.Format(_T("updateCallerInfo('%s', '%s', '%s', %d)"),
		escapedNumber, escapedName, escapedNotes, call_id);

	// Execute JavaScript
	CStringW wJs(js);
	ICoreWebView2* wv = static_cast<ICoreWebView2*>(m_webView);
	if (wv) {
		wv->ExecuteScript(wJs.GetString(), nullptr);
	}
}

void CrmPopupDlg::SaveCallerInfo()
{
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
	ShowWindow(SW_HIDE);
}

void CrmPopupDlg::OnBnClickedAnswer()
{
	// Answer the call
	if (mainDlg) {
		mainDlg->onCallAnswer((WPARAM)call_id, (LPARAM)0);
	}
	OnBnClickedDismiss();
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

void CrmPopupDlg::OnSize(UINT nType, int cx, int cy)
{
	CBaseDialog::OnSize(nType, cx, cy);
	if (m_controller) {
		ICoreWebView2Controller* ctrl = static_cast<ICoreWebView2Controller*>(m_controller);
		RECT bounds;
		GetClientRect(&bounds);
		ctrl->put_Bounds(bounds);
	}
}

void CrmPopupDlg::Restore()
{
	ShowWindow(SW_SHOWNORMAL);
	SetForegroundWindow();
	SetTimer(1, 120000, NULL);
}

// Helper functions
CString CrmPopupDlg::JsonStringToCString(const Json::Value& value)
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

CString CrmPopupDlg::EscapeJson(const CString& input)
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

CString CrmPopupDlg::UrlEncodeCallerNumber(const CString& number)
{
	char* utf8 = Utf8EncodeUcs2(number);
	CStringA encoded = urlencode(utf8 ? CStringA(utf8) : CStringA());
	if (utf8) {
		free(utf8);
	}
	return CString(encoded);
}
