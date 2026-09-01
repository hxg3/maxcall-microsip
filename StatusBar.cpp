/*
 * Copyright (C) 2011-2024 MicroSIP (http://www.microsip.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "StdAfx.h"   
#include "StatusBar.h"   
#include "global.h"
#include "define.h"

IMPLEMENT_DYNAMIC(StatusBar, CStatusBar)
StatusBar::StatusBar()   
{   
	CStatusBar::CStatusBar();
}
   
StatusBar::~StatusBar()   
{   
}

BEGIN_MESSAGE_MAP(StatusBar, CStatusBar)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()

void StatusBar::OnLButtonUp(UINT nFlags, CPoint point)
{
}

void StatusBar::OnMouseMove(UINT nFlags, CPoint point)
{
}

LRESULT StatusBar::OnCustomDraw(WPARAM wParam, LPARAM lParam)
{
	NMTBCUSTOMDRAW *pNM = reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);
	if (!pNM) return CDRF_DODEFAULT;

	switch (pNM->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		return CDRF_NOTIFYITEMDRAW;
	case CDDS_ITEMPREPAINT:
		pNM->clrBtnFace = MAXCARE_SURFACE;
		pNM->clrText = MAXCARE_TEXT_SEC;
		return CDRF_NEWFONT;
	}
	return CDRF_DODEFAULT;
}
