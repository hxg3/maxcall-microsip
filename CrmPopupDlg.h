#pragma once

#include "resource.h"
#include "BaseDialog.h"
#include <pjsua-lib/pjsua.h>

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
	CFont m_fontName;
	CFont m_fontLabel;

	void LoadCallerInfo();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void TabFocusSet() {}
	virtual bool GotoTab(int i, CTabCtrl* tab = NULL) { return true; }
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedDismiss();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
