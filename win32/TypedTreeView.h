/*
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_WIN32_TYPED_TREE_VIEW_H
#define DCPLUSPLUS_WIN32_TYPED_TREE_VIEW_H


template<class T, class ContentType>
class TypedTreeView : public T::WidgetTreeView
{
private:
	typedef typename T::WidgetTreeView BaseType;
	typedef TypedTreeView<T, ContentType> ThisType;
	
public:
	typedef ThisType* ObjectType;

	explicit TypedTreeView( SmartWin::Widget* parent ) : SmartWin::Widget(parent), BaseType(parent) { }
	
	HTREEITEM insert(HTREEITEM parent, ContentType* data) {
		TVINSERTSTRUCT item = { parent, TVI_SORT, {TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT} };
		item.itemex.pszText = LPSTR_TEXTCALLBACK;
		item.itemex.iImage = I_IMAGECALLBACK;
		item.itemex.iSelectedImage = I_IMAGECALLBACK;
		item.itemex.lParam = reinterpret_cast<LPARAM>(data);
		return this->insert(&item);
	}
	
	HTREEITEM insert(TVINSERTSTRUCT* tvis) {
		return TreeView_InsertItem(this->handle(), tvis);
	}
	
	ContentType* getData(HTREEITEM item) {
		TVITEM tvitem = { TVIF_PARAM | TVIF_HANDLE };
		tvitem.hItem = item;
		if(!TreeView_GetItem(this->handle(), &tvitem)) {
			return 0;
		}
		return reinterpret_cast<ContentType*>(tvitem.lParam);
	}
	
	void getItem(TVITEMEX* item) {
		TreeView_GetItem(this->handle(), item);
	}
	
	ContentType* getSelectedData() {
		HTREEITEM item = getSelected();
		return item == NULL ? 0 : getData(item);
	}

	HTREEITEM getRoot() {
		return TreeView_GetRoot(this->handle());
	}
	
	HTREEITEM getSelected() {
		return TreeView_GetSelection(this->handle());
	}
	
	HTREEITEM getNextSibling(HTREEITEM item) {
		return TreeView_GetNextSibling(this->handle(), item);
	}

	HTREEITEM getChild(HTREEITEM item) {
		return TreeView_GetChild(this->handle(), item);
	}
	
	HTREEITEM getParent(HTREEITEM item) {
		return TreeView_GetParent(this->handle(), item);
	}
	
	void deleteItem(HTREEITEM item) {
		TreeView_DeleteItem(this->handle(), item);
	}
};

#endif // !defined(TYPED_LIST_VIEW_CTRL_H)
