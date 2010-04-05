/*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
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

#include "resource.h"

#include "UploadPage.h"

#include <dwt/widgets/Spinner.h>

#include <dcpp/SettingsManager.h>
#include <dcpp/ShareManager.h>
#include <dcpp/version.h>
#include "ParamDlg.h"
#include "HashProgressDlg.h"
#include "WinUtil.h"

static const ColumnInfo columns[] = {
	{ N_("Virtual name"), 100, false },
	{ N_("Directory"), 100, false },
	{ N_("Size"), 100, true }
};

UploadPage::UploadPage(dwt::Widget* parent) :
PropPage(parent),
grid(0),
directories(0),
total(0),
rename(0),
remove(0)
{
	setHelpId(IDH_UPLOADPAGE);

	grid = addChild(Grid::Seed(2, 1));
	grid->column(0).mode = GridInfo::FILL;
	grid->row(0).mode = GridInfo::FILL;
	grid->row(0).align = GridInfo::STRETCH;

	{
		GroupBoxPtr group = grid->addChild(GroupBox::Seed(T_("Shared directories")));
		group->setHelpId(IDH_SETTINGS_UPLOAD_DIRECTORIES);

		GridPtr cur = group->addChild(Grid::Seed(4, 1));
		cur->column(0).mode = GridInfo::FILL;
		cur->row(0).mode = GridInfo::FILL;
		cur->row(0).align = GridInfo::STRETCH;
		cur->setSpacing(6);

		Table::Seed seed = WinUtil::Seeds::Dialog::table;
		seed.exStyle |= WS_EX_ACCEPTFILES;
		directories = cur->addChild(seed);

		{
			GridPtr row = cur->addChild(Grid::Seed(1, 5));
			row->column(1).mode = GridInfo::FILL;

			row->addChild(Label::Seed(T_("Total size:")));
			total = row->addChild(Label::Seed(Text::toT(Util::formatBytes(ShareManager::getInstance()->getShareSize()))));

			row->addChild(Button::Seed(T_("&Add folder")))->onClicked(std::tr1::bind(&UploadPage::handleAddClicked, this));

			rename = row->addChild(Button::Seed(T_("Re&name")));
			rename->onClicked(std::tr1::bind(&UploadPage::handleRenameClicked, this));

			remove = row->addChild(Button::Seed(T_("&Remove")));
			remove->onClicked(std::tr1::bind(&UploadPage::handleRemoveClicked, this));
		}

		cur->addChild(Label::Seed(T_("Note; Files appear in the share only after they've been hashed!")));

		// dummy grid so that the check-box doesn't fill the whole row.
		CheckBoxPtr shareHidden = cur->addChild(Grid::Seed(1, 1))->addChild(CheckBox::Seed(T_("Share hidden files")));
		items.push_back(Item(shareHidden, SettingsManager::SHARE_HIDDEN, PropPage::T_BOOL));
		shareHidden->onClicked(std::tr1::bind(&UploadPage::handleShareHiddenClicked, this, shareHidden, items.back()));
		shareHidden->setHelpId(IDH_SETTINGS_UPLOAD_SHAREHIDDEN);
	}

	{
		GridPtr cur = grid->addChild(Grid::Seed(2, 3));
		cur->column(0).mode = GridInfo::FILL;
		cur->column(0).align = GridInfo::BOTTOM_RIGHT;
		cur->column(1).size = 40;
		cur->column(1).mode = GridInfo::STATIC;

		cur->addChild(Label::Seed(T_("Automatically open an extra slot if speed is below (0 = disable)")))->setHelpId(IDH_SETTINGS_UPLOAD_MIN_UPLOAD_SPEED);

		TextBoxPtr box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::MIN_UPLOAD_SPEED, PropPage::T_INT_WITH_SPIN));
		box->setHelpId(IDH_SETTINGS_UPLOAD_MIN_UPLOAD_SPEED);

		SpinnerPtr spin = cur->addChild(Spinner::Seed(0, UD_MAXVAL, box));
		cur->setWidget(spin);
		spin->setHelpId(IDH_SETTINGS_UPLOAD_MIN_UPLOAD_SPEED);

		cur->addChild(Label::Seed(T_("KiB/s")))->setHelpId(IDH_SETTINGS_UPLOAD_MIN_UPLOAD_SPEED);

		cur->addChild(Label::Seed(T_("Upload slots")))->setHelpId(IDH_SETTINGS_UPLOAD_SLOTS);

		box = cur->addChild(WinUtil::Seeds::Dialog::intTextBox);
		items.push_back(Item(box, SettingsManager::SLOTS_PRIMARY, PropPage::T_INT_WITH_SPIN));
		box->setHelpId(IDH_SETTINGS_UPLOAD_SLOTS);

		spin = cur->addChild(Spinner::Seed(1, UD_MAXVAL, box));
		cur->setWidget(spin);
		spin->setHelpId(IDH_SETTINGS_UPLOAD_SLOTS);

		cur->addChild(Label::Seed(tstring()));
	}

	PropPage::read(items);

	WinUtil::makeColumns(directories, columns, 3);

	fillList();

	directories->onDblClicked(std::tr1::bind(&UploadPage::handleDoubleClick, this));
	directories->onKeyDown(std::tr1::bind(&UploadPage::handleKeyDown, this, _1));
	directories->onRaw(std::tr1::bind(&UploadPage::handleItemChanged, this), dwt::Message(WM_NOTIFY, LVN_ITEMCHANGED));
	directories->onDragDrop(std::tr1::bind(&UploadPage::handleDragDrop, this, _1));
}

UploadPage::~UploadPage() {
}

void UploadPage::layout(const dwt::Rectangle& rc) {
	PropPage::layout(rc);

	dwt::Point clientSize = getClientSize();
	grid->layout(dwt::Rectangle(7, 4, clientSize.x - 14, clientSize.y - 21));

	directories->setColumnWidth(1, directories->getWindowSize().x - 220);
}

void UploadPage::write()
{
	PropPage::write(items);

	if(SETTING(SLOTS) < 1)
		SettingsManager::getInstance()->set(SettingsManager::SLOTS, 1);

	ShareManager::getInstance()->refresh();
}

void UploadPage::handleDoubleClick() {
	if(directories->hasSelected()) {
		handleRenameClicked();
	} else {
		handleAddClicked();
	}
}

bool UploadPage::handleKeyDown(int c) {
	switch(c) {
	case VK_INSERT:
		handleAddClicked();
		return true;
	case VK_DELETE:
		handleRemoveClicked();
		return true;
	}
	return false;
}

LRESULT UploadPage::handleItemChanged() {
	bool enable = directories->hasSelected();
	rename->setEnabled(enable);
	remove->setEnabled(enable);
	return 0;
}

void UploadPage::handleDragDrop(const TStringList& files) {
	for(TStringIterC i = files.begin(); i != files.end(); ++i)
		if(PathIsDirectory(i->c_str()))
			addDirectory(*i);
}

void UploadPage::handleShareHiddenClicked(CheckBoxPtr checkBox, Item& item) {
	// Save the checkbox state so that ShareManager knows to include/exclude hidden files
	SettingsManager::getInstance()->set((SettingsManager::IntSetting)item.setting, checkBox->getChecked());

	// Refresh the share. This is a blocking refresh. Might cause problems?
	// Hopefully people won't click the checkbox enough for it to be an issue. :-)
	ShareManager::getInstance()->setDirty();
	ShareManager::getInstance()->refresh(true, false, true);

	// Clear the GUI list, for insertion of updated shares
	directories->clear();
	fillList();

	// Display the new total share size
	refreshTotalSize();
}

void UploadPage::handleAddClicked() {
	tstring target;
	if(FolderDialog(this).open(target)) {
		addDirectory(target);
		HashProgressDlg(this, true).run();
	}
}

void UploadPage::handleRenameClicked() {
	bool setDirty = false;

	int i = -1;
	while((i = directories->getNext(i, LVNI_SELECTED)) != -1) {
		tstring vName = directories->getText(i, 0);
		tstring rPath = directories->getText(i, 1);
		try {
			ParamDlg dlg(this, T_("Virtual name"), T_("Name under which the others see the directory"), vName);
			if(dlg.run() == IDOK) {
				tstring line = dlg.getValue();
				if (Util::stricmp(vName, line) != 0) {
					ShareManager::getInstance()->renameDirectory(Text::fromT(rPath), Text::fromT(line));
					directories->setText(i, 0, line);

					setDirty = true;
				} else {
					dwt::MessageBox(this).show(T_("New virtual name matches old name, skipping..."), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
						dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONINFORMATION);
				}
			}
		} catch(const ShareException& e) {
			dwt::MessageBox(this).show(Text::toT(e.getError()), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
				dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
		}
	}

	if(setDirty)
		ShareManager::getInstance()->setDirty();
}

void UploadPage::handleRemoveClicked() {
	int i;
	while((i = directories->getNext(-1, LVNI_SELECTED)) != -1) {
		ShareManager::getInstance()->removeDirectory(Text::fromT(directories->getText(i, 1)));
		directories->erase(i);
		refreshTotalSize();
	}
}

void UploadPage::addRow(const string& virtualPath, const string& realPath) {
	TStringList row;
	row.push_back(Text::toT(virtualPath));
	row.push_back(Text::toT(realPath));
	row.push_back(Text::toT(Util::formatBytes(ShareManager::getInstance()->getShareSize(realPath))));
	directories->insert(row);
}

void UploadPage::fillList() {
	const StringPairList dirs = ShareManager::getInstance()->getDirectories();
	for(StringPairList::const_iterator i = dirs.begin(), iend = dirs.end(); i != iend; ++i)
		addRow(i->first, i->second);
}

void UploadPage::refreshTotalSize() {
	total->setText(Text::toT(Util::formatBytes(ShareManager::getInstance()->getShareSize())));
}

void UploadPage::addDirectory(const tstring& aPath) {
	tstring path = aPath;
	if( path[ path.length() -1 ] != _T('\\') )
		path += _T('\\');

	ShareManager* sm = ShareManager::getInstance();
	try {
		while(true) {
			ParamDlg dlg(this, T_("Virtual name"), T_("Name under which the others see the directory"), Text::toT(sm->validateVirtual(Util::getLastDir(Text::fromT(path)))));
			if(dlg.run() == IDOK) {
				const tstring& line = dlg.getValue();
				const string aLine = Text::fromT(line);
				if(sm->hasVirtual(sm->validateVirtual(aLine))) {
					if(dwt::MessageBox(this).show(str(TF_("A virtual directory named %1% already exists, do you wish to merge the contents?") % line),
						_T(APPNAME) _T(" ") _T(VERSIONSTRING), dwt::MessageBox::BOX_YESNO, dwt::MessageBox::BOX_ICONQUESTION) == IDNO) {
						continue;
					}
				}
				const string aPath = Text::fromT(path);
				ShareManager::getInstance()->addDirectory(aPath, aLine);
				addRow(aLine, aPath);
				refreshTotalSize();
			}
			break;
		}
	} catch(const ShareException& e) {
		dwt::MessageBox(this).show(Text::toT(e.getError()), _T(APPNAME) _T(" ") _T(VERSIONSTRING),
			dwt::MessageBox::BOX_OK, dwt::MessageBox::BOX_ICONSTOP);
	}
}
