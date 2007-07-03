/*
 * Copyright (C) 2001-2007 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_TABS_PAGE_H
#define DCPLUSPLUS_WIN32_TABS_PAGE_H

#include "PropPage.h"
#include "WidgetFactory.h"

class TabsPage : public WidgetFactory<SmartWin::WidgetDialog, TabsPage>, public PropPage
{
public:
	TabsPage(SmartWin::Widget* parent);
	virtual ~TabsPage();

#ifdef PORT_ME
	BEGIN_MSG_MAP(TabsPage)
		NOTIFY_CODE_HANDLER_EX(PSN_HELP, onHelpInfo)
		MESSAGE_HANDLER(WM_HELP, onHelp)
	END_MSG_MAP()

	LRESULT onHelp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT onHelpInfo(LPNMHDR /*pnmh*/);
#endif

	virtual void write();

private:
	static TextItem texts[];
	static Item items[];
	static ListItem listItems[];
};

#endif // !defined(DCPLUSPLUS_WIN32_TABS_PAGE_H)
