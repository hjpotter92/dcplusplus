/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if !defined(AFX_EXLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_)
#define AFX_EXLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../client/Util.h"

class ExListViewCtrl : public CWindowImpl<ExListViewCtrl, CListViewCtrl, CControlWinTraits>
{
	int sortColumn;
	int sortType;
	bool ascending;
	int (*fun)(LPARAM, LPARAM);
	HBITMAP upArrow;
	HBITMAP downArrow;

public:
	enum {	
		SORT_FUNC = 2,
 		SORT_STRING,
		SORT_STRING_NOCASE,
		SORT_INT,
		SORT_FLOAT
	};

	BEGIN_MSG_MAP(ExListViewCtrl)
		MESSAGE_HANDLER(WM_CREATE, onCreate)
		MESSAGE_HANDLER(WM_DESTROY, onDestroy)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, onSettingChange)
	END_MSG_MAP()

	void setSort(int aColumn, int aType, bool aAscending = true, int (*aFun)(LPARAM, LPARAM) = NULL) {
		bool doUpdateArrow = (aColumn != sortColumn || aAscending != ascending);
		
		sortColumn = aColumn;
		sortType = aType;
		ascending = aAscending;
		fun = aFun;
		resort();
		if (doUpdateArrow)
			updateArrow();
	}

	void resort() {
		if(sortColumn != -1) {
			SortItemsEx(&CompareFunc, (LPARAM)this);
		}
	}

	bool getSortDirection() { return ascending; };
	int getSortColumn() { return sortColumn; };
	int getSortType() { return sortType; };

	int insert(int nItem, StringList& aList, int iImage = 0, LPARAM lParam = NULL);
	int insert(StringList& aList, int iImage = 0, LPARAM lParam = NULL);
	int insert(int nItem, const string& aString, int iImage = 0, LPARAM lParam = NULL) {
		return InsertItem(LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE, nItem, aString.c_str(), 0, 0, iImage, lParam);
	}

	int getItemImage(int aItem) {
		LVITEM lvi;
		lvi.iItem = aItem;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_IMAGE;
		GetItem(&lvi);
		return lvi.iImage;
	}
	
	int find(LPARAM lParam, int aStart = -1) {
		LV_FINDINFO fi;
		fi.flags = LVFI_PARAM;
		fi.lParam = lParam;
		return FindItem(&fi, aStart);
	}

	int find(const string& aText, int aStart = -1, bool aPartial = false) {
		LV_FINDINFO fi;
		fi.flags = aPartial ? LVFI_PARTIAL : LVFI_STRING;
		fi.psz = aText.c_str();
		return FindItem(&fi, aStart);
	}
	void deleteItem(const string& aItem, int col = 0) {
		for(int i = 0; i < GetItemCount(); i++) {
			char buf[256];
			GetItemText(i, col, buf, 256);
			if(aItem == buf) {
				DeleteItem(i);
				break;
			}
		}
	}
	int moveItem(int oldPos, int newPos);
	void setSortDirection(bool aAscending) { setSort(sortColumn, sortType, aAscending, fun); };

	static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
		ExListViewCtrl* p = (ExListViewCtrl*) lParamSort;
		char buf[128];
		char buf2[128];
		// This is a trick, so that if fun() returns something bigger than one, use the
		// internal default sort functions
		int result = p->sortType;
		if(result == SORT_FUNC) {
			result = p->fun(p->GetItemData(lParam1), p->GetItemData(lParam2));
		} 

		if(result == SORT_STRING) {
			p->GetItemText(lParam1, p->sortColumn, buf, 128);
			p->GetItemText(lParam2, p->sortColumn, buf2, 128);
			result = strcmp(buf, buf2);			
		} else if(result == SORT_STRING_NOCASE) {
			p->GetItemText(lParam1, p->sortColumn, buf, 128);
			p->GetItemText(lParam2, p->sortColumn, buf2, 128);
			result = Util::stricmp(buf, buf2);			
		} else if(result == SORT_INT) {
			p->GetItemText(lParam1, p->sortColumn, buf, 128);
			p->GetItemText(lParam2, p->sortColumn, buf2, 128);
			result = compare(atoi(buf), atoi(buf2));
		} else if(result == SORT_FLOAT) {
			p->GetItemText(lParam1, p->sortColumn, buf, 128);
			p->GetItemText(lParam2, p->sortColumn, buf2, 128);
			result = compare(atof(buf), atof(buf2));
		}
		if(!p->ascending)
			result = -result;
		return result;
	}
	
	template<class T> static int compare(const T& a, const T& b) {
		return (a < b) ? -1 : ( (a == b) ? 0 : 1);
	}

	ExListViewCtrl() : sortType(SORT_STRING), ascending(true), sortColumn(-1), upArrow(0), downArrow(0) { };

	virtual ~ExListViewCtrl()
	{
		if (upArrow)
			::DeleteObject(upArrow);
		if (downArrow)
			::DeleteObject(downArrow);
	};

protected:
	void rebuildArrows();
	void updateArrow();

	LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		rebuildArrows();
		_Module.AddSettingChangeNotify(m_hWnd);
		bHandled = FALSE;
		return 0;
	}

	LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		_Module.RemoveSettingChangeNotify(m_hWnd);
		bHandled = FALSE;
		return 0;
	}

	LRESULT onSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) { 
		rebuildArrows();
		return 1;
	}
};

#endif // !defined(AFX_EXLISTVIEWCTRL_H__45847002_68C2_4C8A_9C2D_C4D8F65DA841__INCLUDED_)

/**
 * @file
 * $Id: ExListViewCtrl.h,v 1.10 2003/11/07 00:42:41 arnetheduck Exp $
 */

