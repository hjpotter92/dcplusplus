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
#include "StylesPage.h"

#include <dcpp/SettingsManager.h>

#include <dwt/widgets/Button.h>
#include <dwt/widgets/ColorDialog.h>
#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>

#include "resource.h"
#include "SettingsDialog.h"
#include "TypedTable.h"
#include "UserMatchPage.h"
#include "WinUtil.h"

using dwt::Button;
using dwt::ColorDialog;
using dwt::FontDialog;
using dwt::Grid;
using dwt::GridInfo;
using dwt::Label;

static const ColumnInfo columns[] = {
	{ "", 100, false }
};

StylesPage::StylesPage(dwt::Widget* parent) :
PropPage(parent, 1, 1),
globalData(0),
table(0),
preview(0),
customFont(0),
font(0),
customTextColor(0),
textColor(0),
customBgColor(0),
bgColor(0),
showGen(0)
{
	setHelpId(IDH_STYLESPAGE);

	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		auto cur = grid->addChild(GroupBox::Seed(T_("Styles")))->addChild(Grid::Seed(5, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(grid->getSpacing());

		{
			auto seed = WinUtil::Seeds::Dialog::table;
			seed.style &= ~LVS_SHOWSELALWAYS;
			seed.style |= LVS_SINGLESEL | LVS_NOCOLUMNHEADER;
			table = cur->addChild(Table::Seed(seed));

			dwt::ImageListPtr images(new dwt::ImageList(dwt::Point(16, 16)));
			images->add(*WinUtil::createIcon(IDI_DCPP, 16));
			images->add(*WinUtil::createIcon(IDI_NET_STATS, 16)); /// @todo better icon for the "transfers" group?
			images->add(*WinUtil::createIcon(IDI_USERS, 16));
			table->setGroupImageList(images);
		}

		{
			Label::Seed seed;
			seed.exStyle |= WS_EX_CLIENTEDGE;
			preview = cur->addChild(seed);
			preview->setHelpId(IDH_SETTINGS_STYLES_PREVIEW);
		}

		auto row = cur->addChild(Grid::Seed(1, 2));
		row->column(0).mode = GridInfo::FILL;
		row->setSpacing(cur->getSpacing());

		auto cur2 = row->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_FONT);

		customFont = cur2->addChild(CheckBox::Seed(T_("Custom font")));
		customFont->onClicked([this] { handleCustomFont(); });

		font = cur2->addChild(Button::Seed(T_("Select font")));
		font->onClicked([this] { handleFont(); });

		cur2 = row->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_TEXT);

		customTextColor = cur2->addChild(CheckBox::Seed(T_("Custom text color")));
		customTextColor->onClicked([this] { handleCustomTextColor(); });

		textColor = cur2->addChild(Button::Seed(T_("Select color")));
		textColor->onClicked([this] { handleTextColor(); });

		row = cur->addChild(Grid::Seed(1, 1));
		row->column(0).mode = GridInfo::FILL;
		row->column(0).align = GridInfo::BOTTOM_RIGHT;
		row->setSpacing(cur->getSpacing());

		cur2 = row->addChild(Grid::Seed(1, 2));
		cur2->setHelpId(IDH_SETTINGS_STYLES_BG);

		customBgColor = cur2->addChild(CheckBox::Seed(T_("Custom background color")));
		customBgColor->onClicked([this] { handleCustomBgColor(); });

		bgColor = cur2->addChild(Button::Seed(T_("Select color")));
		bgColor->onClicked([this] { handleBgColor(); });

		row = cur->addChild(Grid::Seed(1, 2));
		row->setSpacing(cur->getSpacing());

		auto button = row->addChild(Button::Seed(T_("Configure user matching definitions")));
		button->setHelpId(IDH_SETTINGS_STYLES_CONF_USER_MATCHING);
		button->setImage(WinUtil::buttonIcon(IDI_USERS));
		button->onClicked([this] { static_cast<SettingsDialog*>(getRoot())->activatePage<UserMatchPage>(); });

		showGen = row->addChild(CheckBox::Seed(T_("Show those generated by DC++")));
		showGen->setHelpId(IDH_SETTINGS_STYLES_SHOW_GEN_MATCHERS);
		showGen->onClicked([this] { static_cast<SettingsDialog*>(getRoot())->getPage<UserMatchPage>()->updateStyles(); });
	}

	WinUtil::makeColumns(table, columns, COLUMN_LAST);

	TStringList groups(GROUP_LAST);
	groups[GROUP_GENERAL] = T_("General");
	groups[GROUP_TRANSFERS] = T_("Transfers");
	groups[GROUP_CHAT] = T_("Chat");
	groups[GROUP_USERS] = T_("Users");
	table->setGroups(groups);
	auto grouped = table->isGrouped();

	auto add = [this, grouped](tstring&& text, unsigned helpId, int group, int fontSetting, int textColorSetting, int bgColorSetting) -> Data* {
		auto data = new SettingsData(forward<tstring>(text), helpId, fontSetting, textColorSetting, bgColorSetting);
		this->table->insert(grouped ? group : -1, data);
		return data;
	};

	globalData = add(T_("Global application style"), IDH_SETTINGS_STYLES_GLOBAL, GROUP_GENERAL,
		SettingsManager::MAIN_FONT, SettingsManager::TEXT_COLOR, SettingsManager::BACKGROUND_COLOR);

	add(T_("Uploads"), IDH_SETTINGS_STYLES_UPLOADS, GROUP_TRANSFERS,
		SettingsManager::UPLOAD_FONT, SettingsManager::UPLOAD_TEXT_COLOR, SettingsManager::UPLOAD_BG_COLOR);
	add(T_("Downloads"), IDH_SETTINGS_STYLES_DOWNLOADS, GROUP_TRANSFERS,
		SettingsManager::DOWNLOAD_FONT, SettingsManager::DOWNLOAD_TEXT_COLOR, SettingsManager::DOWNLOAD_BG_COLOR);

	add(T_("Links"), IDH_SETTINGS_STYLES_LINKS, GROUP_CHAT, -1, SettingsManager::LINK_COLOR, -1);
	add(T_("Logs"), IDH_SETTINGS_STYLES_LOGS, GROUP_CHAT, -1, SettingsManager::LOG_COLOR, -1);

	update(globalData);

	handleSelectionChanged();

	table->onSelectionChanged([this] { handleSelectionChanged(); });

	table->setHelpId([this](unsigned& id) { handleTableHelpId(id); });
}

StylesPage::~StylesPage() {
}

void StylesPage::layout() {
	PropPage::layout();

	table->setColumnWidth(COLUMN_TEXT, table->getWindowSize().x - 30);
}

void StylesPage::write() {
	table->forEach(&StylesPage::Data::write);
}

void StylesPage::updateUserMatches(std::vector<UserMatch>& userMatches) {
	// remove previous user matching rows.
	for(size_t i = 0; i < table->size();) {
		if(dynamic_cast<UserMatchData*>(table->getData(i))) {
			table->erase(i);
		} else {
			++i;
		}
	}

	// add current user matching rows.
	for(auto& i: userMatches) {
		if(showGen->getChecked() || !i.isSet(UserMatch::GENERATED)) {
			table->insert(table->isGrouped() ? GROUP_USERS : -1, new UserMatchData(i));
		}
	}
}

StylesPage::Data::Data(tstring&& text, const unsigned helpId) :
Flags(),
text(forward<tstring>(text)),
helpId(helpId)
{
}

const tstring& StylesPage::Data::getText(int) const {
	return text;
}

int StylesPage::Data::getStyle(HFONT& font, COLORREF& textColor, COLORREF& bgColor, int) const {
	auto f = getFont();
	if(f.first) {
		font = f.first->handle();
	}

	auto color = getTextColor();
	if(color >= 0) {
		textColor = color;
	}

	color = getBgColor();
	if(color >= 0) {
		bgColor = color;
	}

	return CDRF_NEWFONT;
}

void StylesPage::Data::makeFont(Font& dest, const string& setting) {
	if(!setting.empty()) {
		WinUtil::decodeFont(Text::toT(setting), dest.second);
		dest.first.reset(new dwt::Font(dest.second));
	}
}

StylesPage::SettingsData::SettingsData(tstring&& text, unsigned helpId, int fontSetting, int textColorSetting, int bgColorSetting) :
Data(forward<tstring>(text), helpId),
fontSetting(fontSetting),
textColorSetting(textColorSetting),
bgColorSetting(bgColorSetting)
{
	if(fontSetting != -1) {
		setFlag(FONT_CHANGEABLE);
		customFont = !SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::StrSetting>(fontSetting));
		makeFont(font, SettingsManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(fontSetting)));
		makeFont(defaultFont, SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::StrSetting>(fontSetting)));
	} else {
		customFont = false;
	}

	if(textColorSetting != -1) {
		setFlag(TEXT_COLOR_CHANGEABLE);
		customTextColor = !SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::IntSetting>(textColorSetting));
		textColor = SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(textColorSetting));
	} else {
		customTextColor = false;
	}

	if(bgColorSetting != -1) {
		setFlag(BG_COLOR_CHANGEABLE);
		customBgColor = !SettingsManager::getInstance()->isDefault(static_cast<SettingsManager::IntSetting>(bgColorSetting));
		bgColor = SettingsManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(bgColorSetting));
	} else {
		customBgColor = false;
	}
}

const StylesPage::Data::Font& StylesPage::SettingsData::getFont() const {
	return customFont ? font : defaultFont;
}

int StylesPage::SettingsData::getTextColor() const {
	return customTextColor ? textColor :
		(textColorSetting != -1) ? SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(textColorSetting)) :
		-1;
}

int StylesPage::SettingsData::getBgColor() const {
	return customBgColor ? bgColor :
		(bgColorSetting != -1) ? SettingsManager::getInstance()->getDefault(static_cast<SettingsManager::IntSetting>(bgColorSetting)) :
		-1;
}

void StylesPage::SettingsData::write() {
	if(fontSetting != -1) {
		if(customFont) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(fontSetting), Text::fromT(WinUtil::encodeFont(font.second)));
		} else {
			SettingsManager::getInstance()->unset(static_cast<SettingsManager::StrSetting>(fontSetting));
		}
	}

	if(textColorSetting != -1) {
		if(customTextColor) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(textColorSetting), textColor);
		} else {
			SettingsManager::getInstance()->unset(static_cast<SettingsManager::IntSetting>(textColorSetting));
		}
	}

	if(bgColorSetting != -1) {
		if(customBgColor) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(bgColorSetting), bgColor);
		} else {
			SettingsManager::getInstance()->unset(static_cast<SettingsManager::IntSetting>(bgColorSetting));
		}
	}
}

StylesPage::UserMatchData::UserMatchData(UserMatch& matcher) :
Data(Text::toT(matcher.name), IDH_SETTINGS_STYLES_USER_MATCH),
matcher(matcher)
{
	setFlag(FONT_CHANGEABLE);
	customFont = !matcher.style.font.empty();
	makeFont(font, matcher.style.font);

	setFlag(TEXT_COLOR_CHANGEABLE);
	customTextColor = matcher.style.textColor >= 0;
	textColor = matcher.style.textColor;

	setFlag(BG_COLOR_CHANGEABLE);
	customBgColor = matcher.style.bgColor >= 0;
	bgColor = matcher.style.bgColor;
}

const StylesPage::Data::Font& StylesPage::UserMatchData::getFont() const {
	return customFont ? font : defaultFont;
}

int StylesPage::UserMatchData::getTextColor() const {
	return customTextColor ? textColor : -1;
}

int StylesPage::UserMatchData::getBgColor() const {
	return customBgColor ? bgColor : -1;
}

void StylesPage::UserMatchData::update() {
	matcher.style.font = customFont ? Text::fromT(WinUtil::encodeFont(font.second)) : Util::emptyString;
	matcher.style.textColor = customTextColor ? textColor : -1;
	matcher.style.bgColor = customBgColor ? bgColor : -1;
	matcher.unsetFlag(UserMatch::GENERATED);
}

void StylesPage::handleSelectionChanged() {
	auto data = table->getSelectedData();
	bool enable = data;

	if(data) {
		updatePreview(data);
	}
	preview->setText(data ? data->text : Util::emptyStringT);
	preview->setEnabled(enable);
	preview->setVisible(enable);
	preview->getParent()->layout();

	enable = data && data->customFont;
	customFont->setChecked(enable);
	customFont->setEnabled(data && data->isSet(Data::FONT_CHANGEABLE));
	font->setEnabled(enable);

	enable = data && data->customTextColor;
	customTextColor->setChecked(enable);
	customTextColor->setEnabled(data && data->isSet(Data::TEXT_COLOR_CHANGEABLE));
	textColor->setEnabled(enable);

	enable = data && data->customBgColor;
	customBgColor->setChecked(enable);
	customBgColor->setEnabled(data && data->isSet(Data::BG_COLOR_CHANGEABLE));
	bgColor->setEnabled(enable);
}

void StylesPage::handleTableHelpId(unsigned& id) {
	// same as PropPage::handleListHelpId
	int item = isAnyKeyPressed() ? table->getSelected() :
		table->hitTest(dwt::ScreenCoordinate(dwt::Point::fromLParam(::GetMessagePos()))).first;
	if(item >= 0)
		id = table->getData(item)->helpId;
}

void StylesPage::handleCustomFont() {
	auto data = table->getSelectedData();
	data->customFont = customFont->getChecked();
	if(data->customFont) {
		initFont(data);
	}
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleFont() {
	auto data = table->getSelectedData();
	initFont(data);
	FontDialog::Options options;
	options.strikeout = false;
	options.underline = false;
	options.color = false;
	options.bgColor = getBgColor(data);
	auto color = getTextColor(data);
	if(FontDialog(this).open(data->font.second, color, &options)) {
		data->font.first.reset(new dwt::Font(data->font.second));
		update(data);
	}
}

void StylesPage::handleCustomTextColor() {
	auto data = table->getSelectedData();
	data->customTextColor = customTextColor->getChecked();
	if(data->customTextColor) {
		data->textColor = getTextColor(data);
	}
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleTextColor() {
	auto data = table->getSelectedData();
	auto color = colorDialog(getTextColor(data));
	if(color >= 0) {
		data->textColor = color;
	}
	update(data);
}

void StylesPage::handleCustomBgColor() {
	auto data = table->getSelectedData();
	data->customBgColor = customBgColor->getChecked();
	if(data->customBgColor) {
		data->bgColor = getBgColor(data);
	}
	update(data);
	handleSelectionChanged();
}

void StylesPage::handleBgColor() {
	auto data = table->getSelectedData();
	auto color = colorDialog(getBgColor(data));
	if(color >= 0) {
		data->bgColor = color;
	}
	update(data);
}

int StylesPage::colorDialog(COLORREF color) {
	ColorDialog::ColorParams colorParams(color);
	if(ColorDialog(this).open(colorParams)) {
		return colorParams.getColor();
	}
	return -1;
}

void StylesPage::initFont(Data* const data) const {
	if(!data->font.first) {
		// initialize the LOGFONT structure.
		data->font.second = data->defaultFont.first ? data->defaultFont.second : globalData->getFont().second;
	}
}

COLORREF StylesPage::getTextColor(const Data* const data) const {
	auto color = data->getTextColor();
	return (color >= 0) ? color : globalData->getTextColor();
}

COLORREF StylesPage::getBgColor(const Data* const data) const {
	auto color = data->getBgColor();
	return (color >= 0) ? color : globalData->getBgColor();
}

void StylesPage::update(Data* const data) {
	data->update();

	if(data == globalData) {
		table->setFont(globalData->getFont().first);
		table->setColor(globalData->getTextColor(), globalData->getBgColor());
		table->Control::redraw(true);
	} else {
		table->update(data);
	}

	updatePreview(data);
	preview->getParent()->layout();

	if(dynamic_cast<UserMatchData* const>(data)) {
		static_cast<SettingsDialog*>(getRoot())->getPage<UserMatchPage>()->setDirty();
	}
}

void StylesPage::updatePreview(Data* const data) {
	const auto& font = data->getFont();
	preview->setFont(font.first ? font.first : globalData->getFont().first);
	preview->setColor(getTextColor(data), getBgColor(data));
	preview->redraw(true);
}
