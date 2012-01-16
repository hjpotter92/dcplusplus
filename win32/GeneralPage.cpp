/*
 * Copyright (C) 2001-2012 Jacek Sieka, arnetheduck on gmail point com
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
#include "GeneralPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Grid.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "WinUtil.h"

using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

GeneralPage::GeneralPage(dwt::Widget* parent) :
PropPage(parent, 2, 1),
nick(0),
connections(0)
{
	setHelpId(IDH_GENERALPAGE);

	grid->column(0).mode = GridInfo::FILL;

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Personal information")));
		group->setHelpId(IDH_SETTINGS_GENERAL_PERSONAL_INFORMATION);

		auto cur = group->addChild(Grid::Seed(4, 2));
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		cur->addChild(Label::Seed(T_("Nick")))->setHelpId(IDH_SETTINGS_GENERAL_NICK);
		nick = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(nick, SettingsManager::NICK, PropPage::T_STR));
		nick->setHelpId(IDH_SETTINGS_GENERAL_NICK);

		cur->addChild(Label::Seed(T_("E-Mail")))->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);
		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::EMAIL, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_GENERAL_EMAIL);

		cur->addChild(Label::Seed(T_("Description")))->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);
		box = cur->addChild(WinUtil::Seeds::Dialog::textBox);
		items.push_back(Item(box, SettingsManager::DESCRIPTION, PropPage::T_STR));
		box->setHelpId(IDH_SETTINGS_GENERAL_DESCRIPTION);

		cur->addChild(Label::Seed(T_("Line speed (upload)")))->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

		{
			auto conn = cur->addChild(Grid::Seed(1, 2));
			conn->setSpacing(cur->getSpacing());

			connections = conn->addChild(WinUtil::Seeds::Dialog::comboBox);
			connections->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);

			conn->addChild(Label::Seed(T_("MiBits/s")))->setHelpId(IDH_SETTINGS_GENERAL_CONNECTION);
		}
	}

	{
		auto group = grid->addChild(GroupBox::Seed(T_("Away mode")));
		group->setHelpId(IDH_SETTINGS_GENERAL_AWAY_MODE);

		auto cur = group->addChild(Grid::Seed(3, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->setSpacing(grid->getSpacing());

		{
			group = cur->addChild(GroupBox::Seed(T_("Away message (empty = no away message)")));
			group->setHelpId(IDH_SETTINGS_GENERAL_DEFAULT_AWAY_MESSAGE);

			auto seed = WinUtil::Seeds::Dialog::textBox;
			seed.style |= ES_MULTILINE | WS_VSCROLL | ES_WANTRETURN;
			items.push_back(Item(group->addChild(seed), SettingsManager::DEFAULT_AWAY_MESSAGE, PropPage::T_STR));
		}

		// dummy grid so that the check-box doesn't fill the whole row.
		auto box = cur->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Auto-away on minimize (and back on restore)")));
		box->setHelpId(IDH_SETTINGS_GENERAL_AUTO_AWAY);
		items.push_back(Item(box, SettingsManager::AUTO_AWAY, PropPage::T_BOOL));

		box = cur->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Auto-away when Windows is locked (and back when unlocked)")));
		box->setHelpId(IDH_SETTINGS_GENERAL_AWAY_COMP_LOCK);
		items.push_back(Item(box, SettingsManager::AWAY_COMP_LOCK, PropPage::T_BOOL));
	}

	PropPage::read(items);

	nick->onUpdated([this] { handleNickTextChanged(); });

	if(SETTING(NICK).empty()) {
		// fill the Nick field with the Win user account name.
		DWORD size = 0;
		::GetUserName(0, &size);
		if(size > 1) {
			tstring str(size - 1, 0);
			if(::GetUserName(&str[0], &size)) {
				nick->setText(str);
			}
		}
	}

	int selected = 0, j = 0;
	for(auto i = SettingsManager::connectionSpeeds.begin(); i != SettingsManager::connectionSpeeds.end(); ++i, ++j) {
		connections->addValue(Text::toT(*i).c_str());
		if(selected == 0 && SETTING(UPLOAD_SPEED) == *i) {
			selected = j;
		}
	}
	connections->setSelected(selected);
}

GeneralPage::~GeneralPage() {
}

void GeneralPage::write() {
	PropPage::write(items);

	SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, Text::fromT(connections->getText()));
}

void GeneralPage::handleNickTextChanged() {
	tstring text = nick->getText();
	bool update = false;

	// Strip ' '
	tstring::size_type i;
	while((i = text.find(' ')) != string::npos) {
		text.erase(i, 1);
		update = true;
	}

	if(update) {
		// Something changed; update window text without changing cursor pos
		long caretPos = nick->getCaretPos() - 1;
		nick->setText(text);
		nick->setSelection(caretPos, caretPos);
	}
}
