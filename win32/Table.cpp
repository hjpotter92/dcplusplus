/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#include "Table.h"

Table::Seed::Seed() :
	BaseType::Seed()
{
}

Table::Table( dwt::Widget * parent ) : BaseType(parent) {
	this->onKeyDown(std::tr1::bind(&Table::handleKeyDown, this, _1));
}

bool Table::handleKeyDown(int c) {
	if(c == 'A' && isControlPressed()) {
		const size_t itemCount = size();
		for(size_t i = 0; i < itemCount; ++i)
			select(i);
		return true;
	}
	return false;
}