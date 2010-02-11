/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor the names of its contributors
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
#ifndef DWT_NOTIFICATION_H_
#define DWT_NOTIFICATION_H_

#include "../resources/Icon.h"
#include "Window.h"

namespace dwt {

/** A notification object represents a tray icon and a short message notification service */
class Notification {
public:
	Notification(WindowPtr parent_) : parent(parent_), lastTick(0) { }
	~Notification();

	struct Seed {
		Seed(const IconPtr& icon_ = IconPtr(), const tstring& tip_ = tstring()) : icon(icon_), tip(tip_) { }

		IconPtr icon;
		tstring tip;
	};

	void create(const Seed& seed = Seed());

	void setIcon(const IconPtr& icon_);

	void setVisible(bool visible_);
	bool isVisible() const { return visible; }

	void setTooltip(const tstring& tip);

	void addNotification(const tstring& message);

	// TODO Fix callback parameters
	typedef std::tr1::function<void ()> Callback;

	void onContextMenu(const Callback& callback_) { contextMenu = callback_; }

	/// The icon was left-clicked / selected
	void onIconClicked(const Callback& callback_) { iconClicked = callback_; }

	/// The message added by addNotification was clicked
	void onNotificationClicked(const Callback& callback_) { notificationClicked = callback_; }

	/// This is sent when the tooltip text should be updated
	void onUpdateTip(const Callback& callback_) { updateTip = callback_; }

private:
	WindowPtr parent;
	IconPtr icon;

	bool visible;
	tstring tip;
	/** List of messages to display */
	std::list<tstring> messages;

	Callback contextMenu;
	Callback iconClicked;
	Callback notificationClicked;
	Callback updateTip;

	/// Last tick that tip was updated
	DWORD lastTick;
	bool trayHandler(const MSG& msg);
	bool redisplay();

	static const UINT message;
};
}

#endif /* NOTIFICATION_H_ */
