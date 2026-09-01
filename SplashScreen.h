#pragma once

#include <afxwin.h>
#include "define.h"

class CSplashScreen : public CWnd
{
public:
	CSplashScreen();
	virtual ~CSplashScreen();

	BOOL Create(CWnd* pParentWnd);
	void Close();

protected:
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	DECLARE_MESSAGE_MAP()

private:
	CFont m_fontTitle;
	CFont m_fontSub;
	CFont m_fontLoading;
	CBrush m_brTeal;
	CBrush m_brSurface;
};
