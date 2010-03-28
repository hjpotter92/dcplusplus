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

#ifndef DCPLUSPLUS_WIN32_STDAFX_H
#define DCPLUSPLUS_WIN32_STDAFX_H

#include <dcpp/stdinc.h>
#include <dcpp/DCPlusPlus.h>

#include <shlobj.h>
#include <malloc.h>
#include <htmlhelp.h>
#include <libintl.h>

#include <dwt/Application.h>
#include <dwt/resources/Pen.h>
#include <dwt/widgets/Button.h>
#include <dwt/widgets/CheckBox.h>
#include <dwt/widgets/ComboBox.h>
#include <dwt/widgets/ColorDialog.h>
#include <dwt/widgets/Container.h>
#include <dwt/widgets/FolderDialog.h>
#include <dwt/widgets/FontDialog.h>
#include <dwt/widgets/Grid.h>
#include <dwt/widgets/GroupBox.h>
#include <dwt/widgets/Label.h>
#include <dwt/widgets/LoadDialog.h>
#include <dwt/widgets/MessageBox.h>
#include <dwt/widgets/Menu.h>
#include <dwt/widgets/ModalDialog.h>
#include <dwt/widgets/ModelessDialog.h>
#include <dwt/widgets/RadioButton.h>
#include <dwt/widgets/RichTextBox.h>
#include <dwt/widgets/SaveDialog.h>
#include <dwt/widgets/Table.h>
#include <dwt/widgets/TabView.h>
#include <dwt/widgets/TextBox.h>
#include <dwt/widgets/Tree.h>
#include <dwt/widgets/ToolTip.h>
#include <dwt/widgets/Window.h>

using namespace dcpp;

using dwt::Button;
using dwt::ButtonPtr;
using dwt::CheckBox;
using dwt::CheckBoxPtr;
using dwt::ColorDialog;
using dwt::Container;
using dwt::ContainerPtr;
using dwt::FolderDialog;
using dwt::FontDialog;
using dwt::Label;
using dwt::LabelPtr;
using dwt::Menu;
using dwt::MenuPtr;
using dwt::Grid;
using dwt::GridInfo;
using dwt::GridPtr;
using dwt::GroupBox;
using dwt::GroupBoxPtr;
using dwt::LoadDialog;
using dwt::ProgressBar;
using dwt::ProgressBarPtr;
using dwt::RadioButton;
using dwt::RadioButtonPtr;
using dwt::SaveDialog;
using dwt::Spinner;
using dwt::SpinnerPtr;
using dwt::TabView;
using dwt::TabViewPtr;
using dwt::TextBox;
using dwt::TextBoxPtr;
using dwt::ToolBar;
using dwt::ToolBarPtr;
using dwt::Tree;
using dwt::TreePtr;

using std::tr1::placeholders::_1;
using std::tr1::placeholders::_2;

#define LOCALEDIR dcpp::Util::getPath(Util::PATH_LOCALE).c_str()
#define PACKAGE "dcpp-win32"
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)
#define T_(String) dcpp::Text::toT(gettext(String))
#define CT_(String) T_(String).c_str()
#define F_(String) dcpp_fmt(gettext(String))
#define FN_(String1,String2, N) dcpp_fmt(ngettext(String1, String2, N))
#ifdef UNICODE
#define TF_(String) dcpp_fmt(dcpp::Text::toT(gettext(String)))
#define TFN_(String1,String2, N) dcpp_fmt(dcpp::Text::toT(ngettext(String1, String2, N)))
#else
#define TF_(String) dcpp_fmt(dcpp::Text::toT(gettext(String)))
#define TFN_(String1,String2, N) dcpp_fmt(dcpp::Text::toT(ngettext(String1, String2, N)))
#endif
#endif

#include "ComboBox.h"
#include "RichTextBox.h"
#include "ShellMenu.h"
#include "Table.h"
#include "WidgetPaned.h"
