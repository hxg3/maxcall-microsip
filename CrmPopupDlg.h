#pragma once

#include "resource.h"
#include "BaseDialog.h"
#include <pjsua-lib/pjsua.h>
#include "json.h"

struct ICoreWebView2Controller;
struct ICoreWebView2;
struct ICoreWebView2Environment;

#define WM_WEBVIEW_READY (WM_USER + 100)
#define WM_WEBVIEW_MESSAGE (WM_USER + 101)

class CrmPopupDlg : public CBaseDialog
{
public:
	CrmPopupDlg(CWnd* pParent = NULL);
	~CrmPopupDlg();
	enum { IDD = IDD_CRM_POPUP };

	pjsua_call_id call_id;
	CString callerNumber;
	CString callerName;
	CString notes;

	void LoadCallerInfo();
	void Restore();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void TabFocusSet() {}
	virtual bool GotoTab(int i, CTabCtrl* tab = NULL) { return true; }
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnWebViewReady(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebViewMessage(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	void* m_controller;
	void* m_webView;
	void* m_env;

private:
	void InitWebView2();
	void UpdateWebView();
	void ProcessWebViewMessage(CString& message);
	void SaveCallerInfo();
	void OnClose();

	static CString JsonStringToCString(const Json::Value& value);
	static CString EscapeJson(const CString& input);
	static CString UrlEncodeCallerNumber(const CString& number);
};
