/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

  SmartWin++

  Copyright (c) 2005 Thomas Hansen

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef WINCE

#include <dwt/widgets/Menu.h>

#include <dwt/resources/Brush.h>
#include <dwt/resources/Pen.h>
#include <dwt/DWTException.h>
#include <dwt/util/check.h>
#include <dwt/util/StringUtils.h>
#include <dwt/util/win32/Version.h>
#include <dwt/dwt_vsstyle.h>

#include <algorithm>
#include <boost/scoped_array.hpp>

#include <dwt/widgets/Control.h>

namespace dwt {

const int Menu::borderGap = 3;
const int Menu::pointerGap = 5;
const int Menu::textIconGap = 8;
const int Menu::textBorderGap = 4;
const unsigned Menu::minWidth = 100;

/// @todo menus should re-init the cached default colors on WM_SYSCOLORCHANGE

const COLORREF Menu::Colors::text = ::GetSysColor(COLOR_MENUTEXT);
const COLORREF Menu::Colors::gray = ::GetSysColor(COLOR_GRAYTEXT);

Menu::Colors::Colors() :
background(::GetSysColor(COLOR_MENU)),
menuBar(::GetSysColor(util::win32::ensureVersion(util::win32::VISTA) ? COLOR_MENUBAR : COLOR_3DFACE)),
stripBar(ColorUtilities::darkenColor(background, 0.06)),
highlightBackground(::GetSysColor(COLOR_HIGHLIGHT)),
highlightText(::GetSysColor(COLOR_HIGHLIGHTTEXT)),
titleText(::GetSysColor(COLOR_MENUTEXT))
{
}

Menu::Seed::Seed(bool ownerDrawn_,
				 const Colors& colors_,
				 const Point& iconSize_,
				 FontPtr font_) :
popup(true),
ownerDrawn(ownerDrawn_),
colors(colors_),
iconSize(iconSize_),
font(font_)
{
}

Menu::Menu(Widget* parent) :
parentMenu(0),
itsParent(parent),
ownerDrawn(true),
popup(true),
drawSidebar(false)
{
	dwtassert(dynamic_cast<Control*>(itsParent), _T("A Menu must have a parent derived from dwt::Control"));
}

void Menu::createHelper(const Seed& cs) {
	ownerDrawn = cs.ownerDrawn;
	popup = cs.popup;

	if(ownerDrawn) {
		colors = cs.colors;
		iconSize = cs.iconSize;

		if(cs.font)
			font = cs.font;
		else
			font = new Font(DefaultGuiFont);
		{
			LOGFONT lf;
			::GetObject(font->handle(), sizeof(lf), &lf);
			lf.lfWeight = FW_BOLD;
			itsTitleFont = boldFont = FontPtr(new Font(::CreateFontIndirect(&lf), true));
		}

		theme.load(VSCLASS_MENU, itsParent, !popup);
	}
}

void Menu::create(const Seed& cs) {
	createHelper(cs);

	if(popup)
		itsHandle = ::CreatePopupMenu();
	else
		itsHandle = ::CreateMenu();
	if ( !itsHandle ) {
		throw Win32Exception("CreateMenu in Menu::create failed");
	}

	if(!popup) {
		// set the MNS_NOTIFYBYPOS style to the whole menu
		MENUINFO mi = { sizeof(MENUINFO), MIM_STYLE, MNS_NOTIFYBYPOS };
		if(!::SetMenuInfo(handle(), &mi))
			throw Win32Exception("SetMenuInfo in Menu::create failed");
	}
}

void Menu::attach(HMENU hMenu, const Seed& cs) {
	createHelper(cs);

	itsHandle = hMenu;

	if(ownerDrawn) {
		unsigned defaultItem = ::GetMenuDefaultItem(itsHandle, TRUE, GMDI_USEDISABLED);

		// update all current items to be owner-drawn
		const unsigned count = getCount();
		for(size_t i = 0; i < count; ++i) {
			MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_FTYPE | MIIM_SUBMENU };
			if(::GetMenuItemInfo(itsHandle, i, TRUE, &info)) {
				if(info.hSubMenu) {
					ObjectType subMenu(new Menu(itsParent));
					subMenu->attach(info.hSubMenu, cs);
					itsChildren.push_back(subMenu);
				}

				info.fMask |= MIIM_DATA;
				info.fType |= MFT_OWNERDRAW;

				// create item data wrapper
				ItemDataWrapper * wrapper = new ItemDataWrapper( this, i );
				if(i == defaultItem)
					wrapper->isDefault = true;
				info.dwItemData = reinterpret_cast< ULONG_PTR >( wrapper );

				if(::SetMenuItemInfo(itsHandle, i, TRUE, &info)) {
					itsItemData.push_back( wrapper );
				} else {
					throw Win32Exception("SetMenuItemInfo in Menu::attach failed");
				}
			} else {
				throw Win32Exception("GetMenuItemInfo in Menu::attach failed");
			}
		}
	}
}

LRESULT Menu::handleNCPaint(UINT message, WPARAM wParam, long menuWidth) {
	// forward to ::DefWindowProc
	const MSG msg = { getParent()->handle(), message, wParam, 0 };
	getParent()->getDispatcher().chain(msg);

	if(!theme)
		return TRUE;

	MENUBARINFO info = { sizeof(MENUBARINFO) };
	if(::GetMenuBarInfo(getParent()->handle(), OBJID_MENU, 0, &info)) {
		Rectangle rect(info.rcBar);
		rect.pos -= getParent()->getWindowRect().pos; // convert to client coords

		rect.pos.x += menuWidth;
		rect.size.x -= menuWidth;

		BufferedCanvas<WindowUpdateCanvas> canvas(getParent(), rect.left(), rect.top());

		// avoid non-drawn left edge
		Rectangle rect_bg = rect;
		rect_bg.pos.x -= 1;
		rect_bg.size.x += 1;
		theme.drawBackground(canvas, MENU_BARBACKGROUND, MB_ACTIVE, rect_bg, false);

		canvas.blast(rect);
	}

	return TRUE;
}

void Menu::setMenu() {
	dwtassert(!popup, _T("Only non-popup menus may call setMenu (change the Seed accordingly)"));

	if(!::SetMenu(getParent()->handle(), handle()))
		throw Win32Exception("SetMenu in Menu::setMenu failed");

	if(theme) {
		// get the width that current items will occupy
		long menuWidth = 0;
		const unsigned count = getCount();
		for(size_t i = 0; i < count; ++i) {
			::RECT rect;
			::GetMenuItemRect(getParent()->handle(), handle(), i, &rect);
			menuWidth += Rectangle(rect).width();
		}

		Control* control = static_cast<Control*>(getParent());
		control->onRaw([this, menuWidth](WPARAM wParam, LPARAM) { return handleNCPaint(WM_NCPAINT, wParam, menuWidth); }, Message(WM_NCPAINT));
		control->onRaw([this, menuWidth](WPARAM wParam, LPARAM) { return handleNCPaint(WM_NCACTIVATE, wParam, menuWidth); }, Message(WM_NCACTIVATE));
		::DrawMenuBar(control->handle());
	}
}

Menu::ObjectType Menu::appendPopup(const tstring& text, const IconPtr& icon) {
	// create the sub-menu
	ObjectType sub(new Menu(getParent()));
	sub->create(Seed(ownerDrawn, colors, iconSize, font));
	sub->parentMenu = this;

	// init structure for new item
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STRING };

	// set item text
	info.dwTypeData = const_cast< LPTSTR >( text.c_str() );

	// set sub menu
	info.hSubMenu = sub->handle();

	// get position to insert
	unsigned position = getCount();

	ItemDataWrapper * wrapper = NULL;
	if(ownerDrawn) {
		info.fMask |= MIIM_DATA | MIIM_FTYPE;

		info.fType = MFT_OWNERDRAW;

		// create item data
		wrapper = new ItemDataWrapper( this, position, false, icon );
		info.dwItemData = reinterpret_cast< ULONG_PTR >( wrapper );
	}

	// append to this menu at the end
	if ( ::InsertMenuItem( itsHandle, position, TRUE, & info ) )
	{
		if(ownerDrawn)
			itsItemData.push_back( wrapper );
		itsChildren.push_back( sub );
	}
	return sub;
}

#ifdef PORT_ME
Menu::ObjectType Menu::getSystemMenu()
{
	// get system menu for the utmost parent
	HMENU handle = ::GetSystemMenu( itsParent->handle(), FALSE );

	// create pointer to system menu
	ObjectType sysMenu( new Menu( itsParent->handle() ) );

	// create(take) system menu
	sysMenu->create( handle, false );

	// We're assuming that the system menu has the same lifespan as the "this" menu, we must keep a reference to te system menu
	// otherwise it will be "lost", therefore we add it up as a child to the "this" menu...
	itsChildren.push_back( sysMenu );

	return sysMenu;
}
#endif

Menu::~Menu()
{
	// Destroy this menu
	::DestroyMenu( handle() );
	std::for_each( itsItemData.begin(), itsItemData.end(), destroyItemDataWrapper );
}

void Menu::destroyItemDataWrapper( ItemDataWrapper * wrapper )
{
	if ( 0 != wrapper )
		delete wrapper;

	wrapper = 0;
}

void Menu::setTitleFont( FontPtr font )
{
	itsTitleFont = font;
}

void Menu::clearTitle( bool clearSidebar /* = false */)
{
	if(!ownerDrawn)
		return;

	if ( !clearSidebar && !itsTitle.empty() )
		removeItem( 0 );

	// clear title text
	itsTitle.clear();
}

void Menu::checkItem(unsigned index, bool value) {
	::CheckMenuItem( handle(), index, MF_BYPOSITION | (value ? MF_CHECKED : MF_UNCHECKED) );
}

void Menu::setItemEnabled(unsigned index, bool value) {
	if ( ::EnableMenuItem( handle(), index, MF_BYPOSITION | (value ? MF_ENABLED : MF_GRAYED) ) == - 1 )
	{
		dwtWin32DebugFail("Couldn't enable/disable the menu item, item doesn't exist" );
	}
}

UINT Menu::getMenuState(unsigned index) {
	return ::GetMenuState(handle(), index, MF_BYPOSITION);
}

bool Menu::isSeparator(unsigned index) {
	return (getMenuState(index) & MF_SEPARATOR) == MF_SEPARATOR;
}

bool Menu::isChecked(unsigned index) {
	return (getMenuState(index) & MF_CHECKED) == MF_CHECKED;
}

bool Menu::isPopup(unsigned index) {
	return (getMenuState(index) & MF_POPUP) == MF_POPUP;
}

bool Menu::isEnabled(unsigned index) {
	return !(getMenuState(index) & (MF_DISABLED | MF_GRAYED));
}

void Menu::setDefaultItem(unsigned index) {
	if(!::SetMenuDefaultItem(handle(), index, TRUE))
		throw Win32Exception("SetMenuDefaultItem in Menu::setDefaultItem fizzled...");
}

tstring Menu::getText(unsigned index) const {
	MENUITEMINFO mi = { sizeof(MENUITEMINFO), MIIM_STRING };
	if ( ::GetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE )
		throw Win32Exception( "Couldn't get item info 1 in Menu::getText" );
	boost::scoped_array< TCHAR > buffer( new TCHAR[++mi.cch] );
	mi.dwTypeData = buffer.get();
	if ( ::GetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE )
		throw Win32Exception( "Couldn't get item info 2 in Menu::getText" );
	return mi.dwTypeData;
}

void Menu::setText(unsigned index, const tstring& text) {
	MENUITEMINFO mi = { sizeof(MENUITEMINFO), MIIM_STRING };
	mi.dwTypeData = (TCHAR*) text.c_str();
	if ( ::SetMenuItemInfo( itsHandle, index, TRUE, & mi ) == FALSE )
		dwtWin32DebugFail("Couldn't set item info in Menu::setText");
}

void Menu::setTitle(const tstring& title, const IconPtr& icon, bool drawSidebar /* = false */) {
	if(!ownerDrawn)
		return;

	this->drawSidebar = drawSidebar;

	const bool hasTitle = !itsTitle.empty();
	// set the new title
	itsTitle = util::escapeMenu(title);

	if(!drawSidebar) {
		// init struct for title info
		MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_STATE | MIIM_STRING | MIIM_FTYPE | MIIM_DATA, MFT_OWNERDRAW, MF_DISABLED };

		// set title text
		info.dwTypeData = const_cast< LPTSTR >( title.c_str() );

		// create wrapper for title item
		ItemDataWrapper * wrapper = new ItemDataWrapper(this, 0, true, icon);

		// set item data
		info.dwItemData = reinterpret_cast< ULONG_PTR >( wrapper );

		if ( ( !hasTitle && ::InsertMenuItem( itsHandle, 0, TRUE, & info ) ) ||
			( hasTitle && ::SetMenuItemInfo( itsHandle, 0, TRUE, & info ) ) )
		{
			// adjust item data wrappers for all existing items
			for(size_t i = 0; i < itsItemData.size(); ++i)
				if(itsItemData[i])
					++itsItemData[i]->index;

			// push back title
			itsItemData.push_back( wrapper );
		}
	}
}

bool Menu::handlePainting(LPDRAWITEMSTRUCT drawInfo, ItemDataWrapper* wrapper) {
	// init struct for menu item info
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_CHECKMARKS | MIIM_FTYPE | MIIM_DATA | MIIM_STATE | MIIM_STRING };

	if(::GetMenuItemInfo(handle(), wrapper->index, TRUE, &info) == FALSE)
		throw Win32Exception ( "Couldn't get menu item info in Menu::handleDrawItem" );

	// check if item is owner drawn
	dwtassert( ( info.fType & MFT_OWNERDRAW ) != 0, _T( "Not an owner-drawn item in Menu::handleDrawItem" ) );

	// get state info
	bool isGrayed = ( drawInfo->itemState & ODS_GRAYED ) == ODS_GRAYED;
	bool isChecked = ( drawInfo->itemState & ODS_CHECKED ) == ODS_CHECKED;
	bool isDisabled = ( drawInfo->itemState & ODS_DISABLED ) == ODS_DISABLED;
	bool isSelected = ( drawInfo->itemState & ODS_SELECTED ) == ODS_SELECTED;
	bool isHighlighted = ( drawInfo->itemState & ODS_HOTLIGHT ) == ODS_HOTLIGHT;

	// find item icon
	/// @todo add support for HBITMAPs embedded in MENUITEMINFOs (hbmpChecked, hbmpUnchecked, hbmpItem)
	const IconPtr& icon = wrapper->icon;

	// compute strip width
	int stripWidth = iconSize.x + textIconGap;

	// prepare item rectangle
	Rectangle itemRectangle(drawInfo->rcItem);

	// setup buffered canvas
	BufferedCanvas<FreeCanvas> canvas(drawInfo->hDC, itemRectangle.left(), itemRectangle.top());

	// this will contain adjusted sidebar width
	int sidebarWidth = 0;

	if(drawSidebar) {
		// get logical info for title font
		LOGFONT lf;
		::GetObject(itsTitleFont->handle(), sizeof(lf), &lf);

		// 90 degree rotation and bold
		lf.lfOrientation = lf.lfEscapement = 900;

		// create title font from logical info and select it
		Canvas::Selector select(canvas, *FontPtr(new Font(::CreateFontIndirect(&lf), true)));

		// set sidebar width to text height
		sidebarWidth = canvas.getTextExtent(itsTitle).y;

		// adjust item rectangle and item background
		itemRectangle.pos.x += sidebarWidth;
		itemRectangle.size.x -= sidebarWidth;

		if((drawInfo->itemAction & ODA_DRAWENTIRE) && !itsTitle.empty()) {
			// draw sidebar with menu title

			// select title color
			COLORREF oldColor = canvas.setTextColor( colors.titleText );

			// set background mode to transparent
			bool oldMode = canvas.setBkMode( true );

			// get rect for sidebar
			RECT rect;
			::GetClipBox( drawInfo->hDC, & rect );
			//rect.left -= borderGap;

			// set title rectangle
			Rectangle textRectangle( 0, 0, sidebarWidth, rect.bottom - rect.top );

			// draw background
			canvas.fill(textRectangle, Brush(colors.stripBar));

			// draw title
			textRectangle.pos.y += 10;
			canvas.drawText(itsTitle, textRectangle, DT_BOTTOM | DT_SINGLELINE);

			// clear
			canvas.setTextColor( oldColor );
			canvas.setBkMode( oldMode );
		}
	}

	bool highlight = (isSelected || isHighlighted) && !isDisabled;

	int part, state;
	if(theme) {
		int part_bg, state_bg;
		if(popup && !wrapper->isTitle) {
			part = MENU_POPUPITEM;
			state = (isDisabled || isGrayed) ? ((isSelected || isHighlighted) ? MPI_DISABLEDHOT : MPI_DISABLED) :
				highlight ? MPI_HOT : MPI_NORMAL;
			part_bg = MENU_POPUPBACKGROUND;
			state_bg = 0;
		} else {
			part = MENU_BARITEM;
			state = wrapper->isTitle ? MBI_NORMAL : isSelected ? MBI_PUSHED : isHighlighted ? MBI_HOT : MBI_NORMAL;
			part_bg = MENU_BARBACKGROUND;
			state_bg = MB_ACTIVE;
		}

		if(theme.isBackgroundPartiallyTransparent(part, state)) {
			Rectangle rect = itemRectangle;
			if(part_bg == MENU_BARBACKGROUND) {
				// avoid non-drawn edges
				rect.pos.x -= 1;
				rect.size.x += 2;
			}
			theme.drawBackground(canvas, part_bg, state_bg, rect, false);
		}
		theme.drawBackground(canvas, part, state, itemRectangle, false);

	} else {
		canvas.fill(itemRectangle, Brush(
			highlight ? colors.highlightBackground :
			!popup ? colors.menuBar :
			wrapper->isTitle ? colors.stripBar :
			colors.background));
	}

	Rectangle stripRectangle(itemRectangle);
	stripRectangle.size.x = stripWidth;

	if(!(theme ? (isSelected || isHighlighted) : highlight) && popup && !wrapper->isTitle) {
		// paint the strip bar (on the left, where icons go)
		canvas.fill(stripRectangle, Brush(colors.stripBar));
	}

	if(isChecked && theme) {
		/// @todo we should also support bullets
		theme.drawBackground(canvas, MENU_POPUPCHECKBACKGROUND, icon ? MCB_BITMAP : MCB_NORMAL, stripRectangle, false);
		if(!icon)
			theme.drawBackground(canvas, MENU_POPUPCHECK, MC_CHECKMARKNORMAL, stripRectangle, false);
	}

	if(popup && info.fType & MFT_SEPARATOR) {
		// separator
		Rectangle rectangle(itemRectangle);
		rectangle.pos.x += stripWidth + textIconGap;

		if(theme) {
			Point pt;
			if(theme.getPartSize(canvas, MENU_POPUPSEPARATOR, 0, pt)) {
				rectangle.size.y = std::min(rectangle.size.y, pt.y);
			}
			theme.drawBackground(canvas, MENU_POPUPSEPARATOR, 0, rectangle, false);

		} else {
			// center in the item rectangle
			rectangle.pos.y += rectangle.height() / 2 - 1;

			// select color
			Canvas::Selector select(canvas, *PenPtr(new Pen(Colors::gray)));

			// draw separator
			canvas.moveTo( rectangle.left(), rectangle.top() );
			canvas.lineTo( rectangle.width(), rectangle.top() );
		}

	} else {
		// not a seperator, then draw item text and icon

		// get item text
		const int length = info.cch + 1;
		std::vector< TCHAR > buffer( length );
		int count = ::GetMenuString(handle(), wrapper->index, &buffer[0], length, MF_BYPOSITION);
		tstring itemText( buffer.begin(), buffer.begin() + count );

		if(!itemText.empty()) {
			// index will contain accelerator position
			size_t index = itemText.find_last_of( _T( '\t' ) );

			// split item text to draw accelerator correctly
			tstring text = itemText.substr( 0, index );

			// get accelerator
			tstring accelerator;

			if ( index != itemText.npos )
				accelerator = itemText.substr( index + 1 );

			// Select item font
			Canvas::Selector select(canvas, *(
				wrapper->isTitle ? itsTitleFont :
				wrapper->isDefault ? boldFont :
				font));

			// compute text rectangle
			Rectangle textRectangle(itemRectangle);
			if(popup) {
				if(!wrapper->isTitle || icon) {
					textRectangle.pos.x += stripWidth + textIconGap;
					textRectangle.size.x -= stripWidth + textIconGap;
				}
				textRectangle.size.x -= borderGap;
			}

			unsigned drawTextFormat = DT_VCENTER | DT_SINGLELINE;
			if((drawInfo->itemState & ODS_NOACCEL) == ODS_NOACCEL)
				drawTextFormat |= DT_HIDEPREFIX;
			unsigned drawAccelFormat = drawTextFormat | DT_RIGHT;
			drawTextFormat |= (popup && !wrapper->isTitle) ? DT_LEFT : DT_CENTER;
			if(wrapper->isTitle)
				drawTextFormat |= DT_WORD_ELLIPSIS;

			if(theme) {
				theme.drawText(canvas, part, state, text, drawTextFormat, textRectangle);
				if(!accelerator.empty())
					theme.drawText(canvas, part, state, accelerator, drawAccelFormat, textRectangle);

			} else {
				bool oldMode = canvas.setBkMode(true);

				canvas.setTextColor(
					isGrayed ? Colors::gray :
					wrapper->isTitle ? colors.titleText :
					highlight ? colors.highlightText :
					wrapper->textColor);

				canvas.drawText(text, textRectangle, drawTextFormat);
				if(!accelerator.empty())
					canvas.drawText(accelerator, textRectangle, drawAccelFormat);

				canvas.setBkMode(oldMode);
			}
		}

		// set up icon rectangle
		Rectangle iconRectangle(itemRectangle.pos, iconSize);
		iconRectangle.pos.x += (stripWidth - iconSize.x) / 2;
		iconRectangle.pos.y += (itemRectangle.height() - iconSize.y) / 2;
		if(isSelected && !isDisabled && !isGrayed) {
			// selected and active item; adjust icon position
			iconRectangle.pos.x--;
			iconRectangle.pos.y--;
		}

		if(icon) {
			// the item has an icon; draw it
			canvas.drawIcon(icon, iconRectangle);

		} else if(isChecked && !theme) {
			// the item has no icon set but it is checked; draw the check mark or radio bullet

			// background color
			canvas.fill(iconRectangle, Brush(highlight ? colors.highlightBackground : colors.stripBar));

			// create memory DC and set bitmap on it
			HDC memoryDC = ::CreateCompatibleDC( canvas.handle() );
			HGDIOBJ old = ::SelectObject( memoryDC, ::CreateCompatibleBitmap( canvas.handle(), iconSize.x, iconSize.y ) );

			// draw into memory
			RECT rc( Rectangle( 0, 0, iconSize.x, iconSize.y ) );
			::DrawFrameControl( memoryDC, & rc, DFC_MENU, ( info.fType & MFT_RADIOCHECK ) == 0 ? DFCS_MENUCHECK : DFCS_MENUBULLET );

			const int adjustment = 2; // adjustment for mark to be in the center

			// bit - blast into out canvas
			::BitBlt( canvas.handle(), iconRectangle.left() + adjustment, iconRectangle.top(), iconSize.x, iconSize.y, memoryDC, 0, 0, SRCAND );

			// delete memory dc
			::DeleteObject( ::SelectObject( memoryDC, old ) );
			::DeleteDC( memoryDC );
		}

		if(isChecked && !theme && popup && !wrapper->isTitle) {
			// must draw the surrounding rect after the check-mark
			stripRectangle.pos.x += 1;
			stripRectangle.pos.y += 1;
			stripRectangle.size.x -= 2;
			stripRectangle.size.y -= 2;
			Canvas::Selector select(canvas, *PenPtr(new Pen(Colors::gray)));
			canvas.line(stripRectangle);
		}
	}

	// blast buffer into screen
	if((drawInfo->itemAction & ODA_DRAWENTIRE) && drawSidebar) {
		// adjust for sidebar
		itemRectangle.pos.x -= sidebarWidth;
		itemRectangle.size.x += sidebarWidth;
	}

	canvas.blast(itemRectangle);
	return true;
}

bool Menu::handlePainting(LPMEASUREITEMSTRUCT measureInfo, ItemDataWrapper* wrapper) {
	// this will contain item size
	UINT & itemWidth = measureInfo->itemWidth;
	UINT & itemHeight = measureInfo->itemHeight;

	// init struct for item info
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_FTYPE | MIIM_DATA | MIIM_CHECKMARKS | MIIM_STRING };

	// try to get item info
	if(::GetMenuItemInfo(handle(), wrapper->index, TRUE, &info) == FALSE)
		throw DWTException ( "Couldn't get item info in Menu::handleMeasureItem" );

	// check if item is owner drawn
	dwtassert( ( info.fType & MFT_OWNERDRAW ) != 0, _T( "Not an owner-drawn item in Menu::handleMeasureItem" ) );

	// check if separator
	if(info.fType & MFT_SEPARATOR) {
		if(theme) {
			UpdateCanvas canvas(getParent());
			Point pt;
			if(theme.getPartSize(canvas, MENU_POPUPSEPARATOR, 0, pt)) {
				itemWidth = pt.x;
				itemHeight = pt.y;
				return true;
			}
		}

		itemWidth = 60;
		itemHeight = 8;
		return true;
	}

	Point textSize = getTextSize(getText(wrapper->index),
		wrapper->isTitle ? itsTitleFont :
		wrapper->isDefault ? boldFont :
		font);
	if(!wrapper->isTitle) // the title will adjust its hor size per others and add ellipsis if needed
		itemWidth = textSize.x;
	itemHeight = textSize.y;

	// find item icon
	/// @todo add support for HBITMAPs embedded in MENUITEMINFOs (hbmpChecked, hbmpUnchecked, hbmpItem)
	const IconPtr& icon = wrapper->icon;

	// adjust width
	if(popup || (!popup && icon)) {
		// adjust item width
		itemWidth += iconSize.x + textIconGap + pointerGap;

		// adjust item height
		itemHeight = (std::max)( itemHeight, ( UINT ) iconSize.y + borderGap );
	}

	// adjust width for sidebar
	if(drawSidebar)
		itemWidth += getTextSize(getText(0), itsTitleFont).y; // 0 is the title index

	// make sure the calculated size is not too small
	if(popup) {
		itemWidth = std::max(itemWidth, minWidth);
	}
	itemHeight = std::max(itemHeight, static_cast<UINT>(::GetSystemMetrics(SM_CYMENU)));
	return true;
}

void Menu::appendSeparator() {
	// init structure for new item
	MENUITEMINFO itemInfo = { sizeof(MENUITEMINFO), MIIM_FTYPE, MFT_SEPARATOR };

	// get position to insert
	unsigned position = getCount();

	ItemDataWrapper * wrapper = NULL;
	if(ownerDrawn) {
		itemInfo.fMask |= MIIM_DATA;
		itemInfo.fType |= MFT_OWNERDRAW;

		// create item data wrapper
		wrapper = new ItemDataWrapper( this, position );
		itemInfo.dwItemData = reinterpret_cast< ULONG_PTR >( wrapper );
	}

	if ( ::InsertMenuItem( itsHandle, position, TRUE, & itemInfo ) && ownerDrawn )
		itsItemData.push_back( wrapper );
}

void Menu::removeItem(unsigned index) {
	// has sub menus ?
	HMENU popup = ::GetSubMenu( itsHandle, index );

	// try to remove item
	if ( ::RemoveMenu( itsHandle, index, MF_BYPOSITION ) )
	{
		if(ownerDrawn) {
			ItemDataWrapper * wrapper = 0;
			int itemRemoved = -1;

			for(size_t i = 0; i < itsItemData.size(); ++i) {
				// get current data wrapper
				wrapper = itsItemData[i];

				if ( wrapper->index == index ) // if found
				{
					itemRemoved = int(i);
					delete wrapper;
					itsItemData[i] = 0;
				}
				else if ( wrapper->index > index )
					--wrapper->index; // adjust succeeding item indices
			}

			if( itemRemoved != -1 )
				itsItemData.erase( itsItemData.begin() + itemRemoved );
		}

		// remove sub menus if any
		if(popup)
			for(size_t i = 0; i < itsChildren.size(); ++i)
				if(itsChildren[i]->handle() == popup)
					itsChildren[i].reset();
	} else {
		dwtWin32DebugFail("Couldn't remove item in removeItem()");
	}
}

void Menu::removeAllItems() {
	//must be backwards, since bigger indexes change on remove
	for(int i = getCount() - 1; i >= 0; i--) {
		removeItem( i );
	}
}

unsigned Menu::getCount() const {
	int count = ::GetMenuItemCount(itsHandle);
	if(count < 0)
		throw Win32Exception("GetMenuItemCount in Menu::getCount fizzled...");
	return count;
}

unsigned Menu::appendItem(const tstring& text, const Dispatcher::F& f, const IconPtr& icon, bool enabled, bool defaultItem) {
	// init structure for new item
	MENUITEMINFO info = { sizeof(MENUITEMINFO), MIIM_ID | MIIM_STRING };

	if(!enabled) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DISABLED;
	}
	if(defaultItem) {
		info.fMask |= MIIM_STATE;
		info.fState |= MFS_DEFAULT;
	}

	const unsigned index = getCount();

	if(f) {
		Widget *parent = getParent();
		Dispatcher::F async_f = [this, parent, f] { parent->callAsync(f); };
		if(getRootMenu()->popup) {
			commands_type& commands_ref = getRootMenu()->commands;
			if(!commands_ref.get())
				commands_ref.reset(new commands_type::element_type);
			info.wID = id_offset + commands_ref->size();
			commands_ref->push_back(async_f);
		} else {
			// sub-menus of a menu bar operate via WM_MENUCOMMAND
			getParent()->addCallback(Message(WM_MENUCOMMAND, index * 31 + reinterpret_cast<LPARAM>(handle())),
				Dispatcher(async_f));
		}
	}

	// set text
	info.dwTypeData = const_cast< LPTSTR >( text.c_str() );

	ItemDataWrapper * wrapper = NULL;
	if(ownerDrawn) {
		info.fMask |= MIIM_FTYPE | MIIM_DATA;
		info.fType = MFT_OWNERDRAW;

		// set item data
		wrapper = new ItemDataWrapper(this, index, false, icon);
		if(defaultItem)
			wrapper->isDefault = true;
		info.dwItemData = reinterpret_cast< ULONG_PTR >( wrapper );
	}

	if(!::InsertMenuItem(itsHandle, index, TRUE, &info))
		throw Win32Exception("Couldn't insert item in Menu::appendItem");

	if(ownerDrawn)
		itsItemData.push_back(wrapper);

	return index;
}

void Menu::open(const ScreenCoordinate& sc, unsigned flags) {
	long x = sc.getPoint().x, y = sc.getPoint().y;

	if(x == - 1 || y == - 1) {
		DWORD pos = ::GetMessagePos();
		x = LOWORD(pos);
		y = HIWORD(pos);
	}

	if(!itsTitle.empty()) {
		// adjust "y" so that the first command ends up in front of the cursor, not the title
		/// @todo fix for menus that open upwards
		y -= std::max(static_cast<unsigned>(getTextSize(getText(0), itsTitleFont).y), // 0 is the title index
			static_cast<unsigned>(::GetSystemMetrics(SM_CYMENU)));
	}

	// sub-menus of the menu bar send WM_MENUCOMMAND; however, commands from ephemeral menus are handled right here.
	if(getRootMenu()->popup && (flags & TPM_RETURNCMD) != TPM_RETURNCMD)
		flags |= TPM_RETURNCMD;
	unsigned ret = ::TrackPopupMenu(handle(), flags, x, y, 0, getParent()->handle(), 0);
	if(ret >= id_offset) {
		commands_type& commands_ref = getRootMenu()->commands;
		if(commands_ref.get() && ret - id_offset < commands_ref->size())
			(*commands_ref)[ret - id_offset]();
	}
}

Menu::ObjectType Menu::getChild( unsigned position ) {
	HMENU h = ::GetSubMenu(handle(), position);
	for(size_t i = 0; i < itsChildren.size(); ++i) {
		ObjectType& menu = itsChildren[i];
		if(menu->handle() == h) {
			return menu;
		}
	}
	return ObjectType();
}

Point Menu::getTextSize(const tstring& text, const FontPtr& font_) const {
	UpdateCanvas canvas(getParent());
	Canvas::Selector select(canvas, *font_);
	return canvas.getTextExtent(text) + Point(borderGap, borderGap);
}

}

#endif
