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

#include "stdafx.h"
#include "../client/DCPlusPlus.h"
#include "Resource.h"

#include "FinishedULFrame.h"
#include "WinUtil.h"

#include "../client/ClientManager.h"
#include "../client/StringTokenizer.h"

FinishedULFrame* FinishedULFrame::frame = NULL;

int FinishedULFrame::columnIndexes[] = { COLUMN_DONE, COLUMN_PATH, COLUMN_NICK, COLUMN_SIZE, COLUMN_SPEED };
int FinishedULFrame::columnSizes[] = { 110, 390, 125, 80, 80 };
static ResourceManager::Strings columnNames[] = { ResourceManager::TIME, ResourceManager::PATH, 
ResourceManager::NICK, ResourceManager::SIZE, ResourceManager::SPEED
};

LRESULT FinishedULFrame::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// Only one of this window please...
	dcassert(frame == NULL);
	frame = this;

	CreateSimpleStatusBar(ATL_IDS_IDLEMESSAGE, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP);
	ctrlStatus.Attach(m_hWndStatusBar);
	
	SetWindowText(CSTRING(FINISHED_UPLOADS));
	
	ctrlList.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS , WS_EX_CLIENTEDGE, IDC_FINISHED_UL);

	if(BOOLSETTING(FULL_ROW_SELECT)) {
		ctrlList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
	} else {
		ctrlList.SetExtendedListViewStyle(LVS_EX_HEADERDRAGDROP);
	}
	
	ctrlList.SetBkColor(WinUtil::bgColor);
	ctrlList.SetTextBkColor(WinUtil::bgColor);
	ctrlList.SetTextColor(WinUtil::textColor);
	
	// Create listview columns
	WinUtil::splitTokens(columnIndexes, SETTING(FINISHED_UL_ORDER), COLUMN_LAST);
	WinUtil::splitTokens(columnSizes, SETTING(FINISHED_UL_WIDTHS), COLUMN_LAST);
	
	for(int j=0; j<COLUMN_LAST; j++) {
		int fmt = (j == COLUMN_SIZE || j == COLUMN_SPEED) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ctrlList.InsertColumn(j, CSTRING_I(columnNames[j]), fmt, columnSizes[j], j);
	}
	
	ctrlList.SetColumnOrderArray(COLUMN_LAST, columnIndexes);
	
	UpdateLayout();
	
	FinishedManager::getInstance()->addListener(this);
	updateList(FinishedManager::getInstance()->lockList(true));
	FinishedManager::getInstance()->unlockList();
	
	ctxMenu.CreatePopupMenu();
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FILE, CSTRING(OPEN));
	ctxMenu.AppendMenu(MF_STRING, IDC_OPEN_FOLDER, CSTRING(OPEN_FOLDER));
	ctxMenu.AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);
	ctxMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(REMOVE));
	ctxMenu.AppendMenu(MF_STRING, IDC_TOTAL, CSTRING(REMOVE_ALL));

	bHandled = FALSE;
	return TRUE;
}

LRESULT FinishedULFrame::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	
	NMITEMACTIVATE * const item = (NMITEMACTIVATE*) pnmh;

	if(item->iItem != -1) {
		FinishedItem* entry = (FinishedItem*)ctrlList.GetItemData(item->iItem);
		ShellExecute(NULL, NULL, entry->getTarget().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFile(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		ShellExecute(NULL, NULL, entry->getTarget().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	return 0;
}

LRESULT FinishedULFrame::onOpenFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	int i;
	if((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		FinishedItem * const entry = (FinishedItem*)ctrlList.GetItemData(i);
		ShellExecute(NULL, NULL, Util::getFilePath(entry->getTarget()).c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	return 0;
}

LRESULT FinishedULFrame::onRemove(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	switch(wID)
	{
	case IDC_REMOVE:
		{
			int i = -1;
			while((i = ctrlList.GetNextItem(-1, LVNI_SELECTED)) != -1) {
				FinishedManager::getInstance()->remove((FinishedItem*)ctrlList.GetItemData(i), true);
				ctrlList.DeleteItem(i);
			}
			break;
		}
	case IDC_TOTAL:
		FinishedManager::getInstance()->removeAll(true);
		break;
	}
	return 0;
}

LRESULT FinishedULFrame::onClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	FinishedManager::getInstance()->removeListener(this);
	
	string tmp1, tmp2;

	ctrlList.GetColumnOrderArray(COLUMN_LAST, columnIndexes);
	for(int j = COLUMN_FIRST; j != COLUMN_LAST; j++) {
		columnSizes[j] = ctrlList.GetColumnWidth(j);
		tmp1 += Util::toString(columnIndexes[j]) + ",";
		tmp2 += Util::toString(columnSizes[j]) + ",";
	}
	tmp1.erase(tmp1.size()-1, 1);
	tmp2.erase(tmp2.size()-1, 1);
	
	SettingsManager::getInstance()->set(SettingsManager::FINISHED_UL_ORDER, tmp1);
	SettingsManager::getInstance()->set(SettingsManager::FINISHED_UL_WIDTHS, tmp2);
	
	bHandled = FALSE;
	return 0;
}

void FinishedULFrame::onAction(FinishedManagerListener::Types type, FinishedItem* entry)  throw() {
	switch(type) {
		case FinishedManagerListener::ADDED_UL: addEntry(entry); updateStatus();
			break;

		case FinishedManagerListener::MAJOR_CHANGES_UL: 
			updateList(FinishedManager::getInstance()->lockList(true));
			totalBytes = 0;
			totalTime = 0;
			ctrlList.DeleteAllItems();
			FinishedManager::getInstance()->unlockList();
			updateStatus();
			break;
		case FinishedManagerListener::REMOVED_UL:
			totalBytes -= entry->getChunkSize();
			totalTime -= entry->getMilliSeconds();
			updateStatus();
			break;
	}
};

void FinishedULFrame::addEntry(FinishedItem* entry, bool dirty /* = true */) {
	StringList l;
	l.push_back(entry->getTime());
	l.push_back(entry->getTarget());
	l.push_back(entry->getUser() + " (" + entry->getHub() + ")");
	l.push_back(Util::formatBytes(entry->getSize()));
	l.push_back(Util::formatBytes(entry->getAvgSpeed()) + "/s");
	int loc = ctrlList.insert(l, 0, (LPARAM)entry);
	ctrlList.EnsureVisible(loc, FALSE);

	totalBytes += entry->getChunkSize();
	totalTime += entry->getMilliSeconds();
	if(dirty && BOOLSETTING(FINISHED_DIRTY))
		setDirty();
}


/**
 * @file
 * $Id: FinishedULFrame.cpp,v 1.2 2003/04/15 10:14:01 arnetheduck Exp $
 */
