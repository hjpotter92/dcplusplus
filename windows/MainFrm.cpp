/* 
 * Copyright (C) 2001 Jacek Sieka, j_s@telia.com
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

#include "MainFrm.h"
#include "AboutDlg.h"
#include "HubFrame.h"
#include "SearchFrm.h"
#include "PublicHubsFrm.h"
#include "PropertiesDlg.h"
#include "UsersFrame.h"
#include "DirectoryListingFrm.h"
#include "FavoritesFrm.h"
#include "NotepadFrame.h"
#include "QueueFrame.h"
#include "SpyFrame.h"
#include "FinishedFrame.h"

#include "../client/ConnectionManager.h"
#include "../client/DownloadManager.h"
#include "../client/UploadManager.h"
#include "../client/StringTokenizer.h"
#include "../client/SimpleXML.h"

int MainFrame::columnIndexes[MainFrame::COLUMN_LAST] = { COLUMN_USER, COLUMN_STATUS, COLUMN_SIZE, COLUMN_FILE };
int MainFrame::columnSizes[MainFrame::COLUMN_LAST] = { 200, 300, 400, 100 };
MainFrame::~MainFrame() {
	arrows.Destroy();
	images.Destroy();
	largeImages.Destroy();

	DeleteObject(WinUtil::bgBrush);
	DeleteObject(WinUtil::font);
	WinUtil::fileImages.Destroy();
}

DWORD WINAPI MainFrame::stopper(void* p) {
	MainFrame* mf = (MainFrame*)p;
	HWND wnd, wnd2 = NULL;

	while( (wnd=::GetWindow(mf->m_hWndClient, GW_CHILD)) != NULL) {
		if(wnd == wnd2) 
			Sleep(100);
		else { 
			::SendMessage(wnd, WM_CLOSE, 0, 0);
			wnd2 = wnd;
		}
	}

	shutdown();
	
	mf->PostMessage(WM_CLOSE);	
	return 0;
}

LRESULT MainFrame::onSpeaker(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
		
	if(wParam == ADD_DOWNLOAD_ITEM) {
		StringListInfo* i = (StringListInfo*)lParam;
		StringList l;
		for(int j = 0; j < COLUMN_LAST; j++) {
			l.push_back(i->columns[j]);
		}
		dcassert(ctrlTransfers.find(i->lParam) == -1);
		ctrlTransfers.insert(l, IMAGE_DOWNLOAD, i->lParam);
		delete i;
	} else if(wParam == ADD_UPLOAD_ITEM) {
		StringListInfo* i = (StringListInfo*)lParam;
		StringList l;
		for(int j = 0; j < COLUMN_LAST; j++) {
			l.push_back(i->columns[j]);
		}
		dcassert(ctrlTransfers.find(i->lParam) == -1);
		ctrlTransfers.insert(l, IMAGE_UPLOAD, i->lParam);
		delete i;
	} else if(wParam == REMOVE_ITEM) {
		dcassert(ctrlTransfers.find(lParam) != -1);
		ctrlTransfers.DeleteItem(ctrlTransfers.find(lParam));
	} else if(wParam == SET_TEXT) {
		StringListInfo* l = (StringListInfo*)lParam;
		int n = ctrlTransfers.find(l->lParam);
		if(n != -1) {
			ctrlTransfers.SetRedraw(FALSE);
			for(int i = 0; i < COLUMN_LAST; i++) {
				if(!l->columns[i].empty()) {
					ctrlTransfers.SetItemText(n, i, l->columns[i].c_str());
				}
			}
			ctrlTransfers.SetRedraw(TRUE);
		}
		
		if(ctrlTransfers.getSortColumn() != COLUMN_USER)
			ctrlTransfers.resort();

		delete l;
	} else if(wParam == SET_TEXTS) {
		vector<StringListInfo*>* v = (vector<StringListInfo*>*)lParam;
		ctrlTransfers.SetRedraw(FALSE);
		for(vector<StringListInfo*>::iterator j = v->begin(); j != v->end(); ++j) {
			StringListInfo* l = *j;
			int n = ctrlTransfers.find(l->lParam);
			if(n != -1) {
				for(int i = 0; i < COLUMN_LAST; i++) {
					if(!l->columns[i].empty()) {
						ctrlTransfers.SetItemText(n, i, l->columns[i].c_str());
					}
				}
			}
			delete l;
		}
		ctrlTransfers.SetRedraw(TRUE);
		ctrlTransfers.resort();
		delete v;

	} else if(wParam == DOWNLOAD_LISTING) {
		DirectoryListInfo* i = (DirectoryListInfo*)lParam;
		int n = ctrlTransfers.find(i->lParam);
		ctrlTransfers.SetItemText(n, COLUMN_STATUS, CSTRING(PREPARING_FILE_LIST));
		try {
			DirectoryListingFrame* pChild = new DirectoryListingFrame(i->file, i->user);
			pChild->setTab(&ctrlTab);
			pChild->CreateEx(m_hWndClient);
			pChild->setWindowTitle();
			delete i;
		} catch(FileException e) {
			// ...
		}
		ctrlTransfers.SetItemText(n, COLUMN_STATUS, CSTRING(DOWNLOAD_FINISHED_IDLE));
	} else if(wParam == STATS) {
		StringList* str = (StringList*)lParam;
		if(ctrlStatus.IsWindow()) {

			ctrlStatus.SetText(1, (*str)[0].c_str());
			ctrlStatus.SetText(2, (*str)[1].c_str());
			ctrlStatus.SetText(3, (*str)[2].c_str());
			ctrlStatus.SetText(4, (*str)[3].c_str());
			ctrlStatus.SetText(5, (*str)[4].c_str());
		}
		delete str;
	} else if(wParam == AUTO_CONNECT) {
		autoConnect(HubManager::getInstance()->lockFavoriteHubs());
		HubManager::getInstance()->unlockFavoriteHubs();
	}

	return 0;
}

void MainFrame::onConnectionAdded(ConnectionQueueItem* aCqi) {
	StringListInfo* i = new StringListInfo((LPARAM)aCqi);
	i->columns[COLUMN_USER] = aCqi->getUser()->getNick() + " (" + aCqi->getUser()->getClientName() + ")";
	i->columns[COLUMN_STATUS] = STRING(CONNECTING);

	if(aCqi->getConnection() && aCqi->getConnection()->isSet(UserConnection::FLAG_UPLOAD)) {
		PostMessage(WM_SPEAKER, ADD_UPLOAD_ITEM, (LPARAM)i);
	} else {
		PostMessage(WM_SPEAKER, ADD_DOWNLOAD_ITEM, (LPARAM)i);
	}
}

void MainFrame::onConnectionStatus(ConnectionQueueItem* aCqi) {
	StringListInfo* i = new StringListInfo((LPARAM)aCqi);
	i->columns[COLUMN_STATUS] = aCqi->getStatus() == ConnectionQueueItem::CONNECTING ? STRING(CONNECTING) : STRING(WAITING_TO_RETRY);
	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);
}

void MainFrame::onConnectionRemoved(ConnectionQueueItem* aCqi) {
	PostMessage(WM_SPEAKER, REMOVE_ITEM, (LPARAM)aCqi);
}

void MainFrame::onConnectionFailed(ConnectionQueueItem* aCqi, const string& aReason) {
	StringListInfo* i = new StringListInfo((LPARAM)aCqi);
	i->columns[COLUMN_STATUS] = aReason;
	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);
}
void MainFrame::onUploadStarting(Upload* aUpload) {
	StringListInfo* i = new StringListInfo((LPARAM)aUpload->getUserConnection()->getCQI());
	i->columns[COLUMN_FILE] = aUpload->getFileName();
	i->columns[COLUMN_STATUS] = STRING(UPLOAD_STARTING);
	i->columns[COLUMN_SIZE] = Util::formatBytes(aUpload->getSize());

	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);
}

void MainFrame::onUploadTick(const Upload::List& ul) {
	vector<StringListInfo*>* v = new vector<StringListInfo*>();
	v->reserve(ul.size());
	
	for(Upload::List::const_iterator j = ul.begin(); j != ul.end(); ++j) {
		Upload* u = *j;

		char* buf = new char[STRING(UPLOADED_LEFT).size() + 32];
		sprintf(buf, CSTRING(UPLOADED_LEFT), Util::formatBytes(u->getPos()).c_str(), 
			(double)u->getPos()*100.0/(double)u->getSize(), Util::formatBytes(u->getRunningAverage()).c_str(), Util::formatSeconds(u->getSecondsLeft()).c_str());

		StringListInfo* i = new StringListInfo((LPARAM)u->getUserConnection()->getCQI());
		i->columns[COLUMN_STATUS] = buf;

		delete[] buf;
		v->push_back(i);
	}
	
	PostMessage(WM_SPEAKER, SET_TEXTS, (LPARAM)v);
}

void MainFrame::onUploadComplete(Upload* aUpload) {
	StringListInfo* i = new StringListInfo((LPARAM)aUpload->getUserConnection()->getCQI());
	i->columns[COLUMN_STATUS] = STRING(UPLOAD_FINISHED_IDLE);
	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);	
}

void MainFrame::onDownloadComplete(Download* p) {
	if(p->isSet(Download::USER_LIST)) {
		// We have a new DC listing, show it...
		DirectoryListInfo* i = new DirectoryListInfo((LPARAM)p->getUserConnection()->getCQI());
		i->file = p->getTarget();
		i->user = p->getUserConnection()->getUser();
		
		PostMessage(WM_SPEAKER, DOWNLOAD_LISTING, (LPARAM)i);
	} else {
		StringListInfo* i = new StringListInfo((LPARAM)p->getUserConnection()->getCQI());
		i->columns[COLUMN_STATUS] = STRING(DOWNLOAD_FINISHED_IDLE);
		PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);	
	}
}

void MainFrame::onDownloadFailed(Download* aDownload, const string& aReason) {
	StringListInfo* i = new StringListInfo((LPARAM)aDownload->getUserConnection()->getCQI());
	i->columns[COLUMN_STATUS] = aReason;
	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);
}

void MainFrame::onDownloadStarting(Download* aDownload) {
	StringListInfo* i = new StringListInfo((LPARAM)aDownload->getUserConnection()->getCQI());
	i->columns[COLUMN_FILE] = aDownload->getTargetFileName();
	i->columns[COLUMN_STATUS] = STRING(DOWNLOAD_STARTING);
	i->columns[COLUMN_SIZE] = Util::formatBytes(aDownload->getSize());
	
	PostMessage(WM_SPEAKER, SET_TEXT, (LPARAM)i);
}

void MainFrame::onDownloadTick(const Download::List& dl) {
	vector<StringListInfo*>* v = new vector<StringListInfo*>();
	v->reserve(dl.size());

	for(Download::List::const_iterator j = dl.begin(); j != dl.end(); ++j) {
		Download* d = *j;
		char* buf = new char[STRING(DOWNLOADED_LEFT).size() + 32];
		sprintf(buf, CSTRING(DOWNLOADED_LEFT), Util::formatBytes(d->getPos()).c_str(), 
			(double)d->getPos()*100.0/(double)d->getSize(), Util::formatBytes(d->getRunningAverage()).c_str(), Util::formatSeconds(d->getSecondsLeft()).c_str());
		
		StringListInfo* i = new StringListInfo((LPARAM)d->getUserConnection()->getCQI());
		i->columns[COLUMN_STATUS] = buf;
		delete[] buf;

		v->push_back(i);
	}
	
	PostMessage(WM_SPEAKER, SET_TEXTS, (LPARAM)v);
}

HWND MainFrame::createToolbar() {
	
	CToolBarCtrl ctrl;
	ctrl.Create(m_hWnd, NULL, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS, 0, ATL_IDW_TOOLBAR);
	ctrl.SetImageList(largeImages);
	
	TBBUTTON tb[8];
	memset(tb, 0, sizeof(tb));
	int n = 0;
	tb[n].iBitmap = n;
	tb[n].idCommand = ID_FILE_CONNECT;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = ID_FILE_RECONNECT;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	
	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = IDC_FOLLOW;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = IDC_FAVORITES;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = IDC_QUEUE;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = ID_FILE_SEARCH;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
	
	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = ID_FILE_SETTINGS;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;

	n++;
	tb[n].iBitmap = n;
	tb[n].idCommand = IDC_NOTEPAD;
	tb[n].fsState = TBSTATE_ENABLED;
	tb[n].fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
		
	ctrl.SetButtonStructSize();
	ctrl.AddButtons(8, tb);
	ctrl.AutoSize();

	return ctrl.m_hWnd;
}

LRESULT MainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {

	TimerManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	UploadManager::getInstance()->addListener(this);
	ConnectionManager::getInstance()->addListener(this);
	
	LOGFONT lf;
	::GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	SettingsManager::getInstance()->setDefault(SettingsManager::TEXT_FONT, WinUtil::encodeFont(lf));
	WinUtil::decodeFont(SETTING(TEXT_FONT), lf);

	WinUtil::bgBrush = CreateSolidBrush(SETTING(BACKGROUND_COLOR));
	WinUtil::textColor = SETTING(TEXT_COLOR);
	WinUtil::bgColor = SETTING(BACKGROUND_COLOR);
	WinUtil::font = ::CreateFontIndirect(&lf);

	trayMessage = RegisterWindowMessage("TaskbarCreated");

	if(BOOLSETTING(USE_SYSTEM_ICONS)) {
		SHFILEINFO fi;
		WinUtil::fileImages = CImageList::Duplicate((HIMAGELIST)::SHGetFileInfo(".", FILE_ATTRIBUTE_DIRECTORY, &fi, sizeof(fi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES));
		WinUtil::fileImages.SetBkColor(SETTING(BACKGROUND_COLOR));
		WinUtil::dirIconIndex = fi.iIcon;	
	} else {
		WinUtil::fileImages.CreateFromImage(IDB_FOLDERS, 16, 3, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
		WinUtil::dirIconIndex = 0;
	}
	
	TimerManager::getInstance()->start();
	
	if(!SETTING(LANGUAGE_FILE).empty()) {
		ResourceManager::getInstance()->loadLanguage(SETTING(LANGUAGE_FILE));
	}

	// Set window name
	SetWindowText(APPNAME " " VERSIONSTRING);

	// Load images
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);

	WinUtil::buildMenu();
	m_hMenu = WinUtil::mainMenu;

	// attach menu
	m_CmdBar.AttachMenu(WinUtil::mainMenu);
	// load command bar images
	images.CreateFromImage(IDB_TOOLBAR, 16, 5, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	largeImages.CreateFromImage(IDB_TOOLBAR20, 20, 5, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	m_CmdBar.m_hImageList = images;
	
	m_CmdBar.m_arrCommand.Add(ID_FILE_CONNECT);
	m_CmdBar.m_arrCommand.Add(ID_FILE_RECONNECT);
	m_CmdBar.m_arrCommand.Add(IDC_FOLLOW);
	m_CmdBar.m_arrCommand.Add(IDC_FAVORITES);
	m_CmdBar.m_arrCommand.Add(IDC_QUEUE);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SEARCH);
	m_CmdBar.m_arrCommand.Add(ID_FILE_SETTINGS);
	m_CmdBar.m_arrCommand.Add(IDC_NOTEPAD);
	
	// remove old menu
	SetMenu(NULL);
	
	HWND hWndToolBar = createToolbar();
	
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	CreateSimpleStatusBar();
	
	ctrlStatus.Attach(m_hWndStatusBar);
	ctrlStatus.SetSimple(FALSE);
	
	CreateMDIClient();
	m_CmdBar.SetMDIClient(m_hWndMDIClient);
	
	arrows.CreateFromImage(IDB_ARROWS, 16, 2, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION | LR_SHARED);
	ctrlTransfers.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
		WS_HSCROLL | WS_VSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_CLIENTEDGE, IDC_TRANSFERS);

	if(BOOLSETTING(FULL_ROW_SELECT)) {
		ctrlTransfers.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
	} else {
		ctrlTransfers.SetExtendedListViewStyle(LVS_EX_HEADERDRAGDROP);
	}
	
	StringList l = StringTokenizer(SETTING(MAINFRAME_ORDER), ',').getTokens();

	{
		int k = 0;
		for(StringIter i = l.begin(); i != l.end(); ++i) {
			if(k >= COLUMN_LAST)
				break;
			columnIndexes[k++] = Util::toInt(*i);
		}
	}

	l = StringTokenizer(SETTING(MAINFRAME_WIDTHS), ',').getTokens();
	{
		int k = 0;
		for(StringIter i = l.begin(); i != l.end(); ++i) {
			if(k >= COLUMN_LAST)
				break;
			columnSizes[k++] = Util::toInt(*i);
		}
	}
	
	LV_COLUMN lvc;
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	
	lvc.pszText = const_cast<char*>(CSTRING(USER));
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = columnSizes[COLUMN_USER];
	lvc.iOrder = columnIndexes[COLUMN_USER];
	lvc.iSubItem = COLUMN_USER;
	ctrlTransfers.InsertColumn(0, &lvc);

	lvc.pszText = const_cast<char*>(CSTRING(STATUS));
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = columnSizes[COLUMN_STATUS];
	lvc.iOrder = columnIndexes[COLUMN_STATUS];
	lvc.iSubItem = COLUMN_STATUS;
	ctrlTransfers.InsertColumn(1, &lvc);

	lvc.pszText = const_cast<char*>(CSTRING(FILE));
	lvc.fmt = LVCFMT_LEFT;
	lvc.cx = columnSizes[COLUMN_FILE];
	lvc.iOrder = columnIndexes[COLUMN_FILE];
	lvc.iSubItem = COLUMN_FILE;
	ctrlTransfers.InsertColumn(2, &lvc);

	lvc.pszText = const_cast<char*>(CSTRING(SIZE));
	lvc.fmt = LVCFMT_RIGHT;
	lvc.cx = columnSizes[COLUMN_SIZE];
	lvc.iOrder = columnIndexes[COLUMN_SIZE];
	lvc.iSubItem = COLUMN_SIZE;
	ctrlTransfers.InsertColumn(3, &lvc);
	
	ctrlTransfers.SetBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextBkColor(WinUtil::bgColor);
	ctrlTransfers.SetTextColor(WinUtil::textColor);
	
	ctrlTransfers.SetImageList(arrows, LVSIL_SMALL);
	
	ctrlTab.Create(m_hWnd, rcDefault);
	
	SetSplitterPanes(m_hWndClient, ctrlTransfers.m_hWnd);
	SetSplitterExtendedStyle(SPLIT_PROPORTIONAL);
	m_nProportionalPos = 8000;
	
	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	
	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
		try {
			ConnectionManager::getInstance()->setPort((short)SETTING(PORT));
			SearchManager::getInstance()->setPort((short)SETTING(PORT));
		} catch(Exception e) {
			dcdebug("MainFrame::OnCreate caught %s\n", e.getError().c_str());
			char* buf = new char[STRING(PORT_IS_BUSY).size() + 8];
			sprintf(buf, CSTRING(PORT_IS_BUSY), SETTING(PORT));
			MessageBox(buf);
			delete[] buf;
		}
	}

	transferMenu.CreatePopupMenu();

	transferMenu.AppendMenu(MF_STRING, IDC_GETLIST, CSTRING(GET_FILE_LIST));
	transferMenu.AppendMenu(MF_STRING, IDC_PRIVATEMESSAGE, CSTRING(SEND_PRIVATE_MESSAGE));
	transferMenu.AppendMenu(MF_STRING, IDC_FORCE, CSTRING(FORCE_ATTEMPT));
	transferMenu.AppendMenu(MF_STRING, IDC_REMOVE, CSTRING(CLOSE_CONNECTION));

	c->addListener(this);
	c->downloadFile("http://dcplusplus.sourceforge.net/version.xml");

	if(BOOLSETTING(OPEN_PUBLIC))
		PostMessage(WM_COMMAND, ID_FILE_CONNECT);
	if(BOOLSETTING(OPEN_QUEUE))
		PostMessage(WM_COMMAND, IDC_QUEUE);
	
	PostMessage(WM_SPEAKER, AUTO_CONNECT);

	Util::ensureDirectory(SETTING(LOG_DIRECTORY));
	
	// We want to pass this one on to the splitter...hope it get's there...
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	RECT rc;                    // client area of window 
	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };        // location of mouse click 

	// Get the bounding rectangle of the client area. 
	ctrlTransfers.GetClientRect(&rc);
	ctrlTransfers.ScreenToClient(&pt); 
	if (PtInRect(&rc, pt) && ctrlTransfers.GetSelectedCount() > 0) 
	{ 
		ctrlTransfers.ClientToScreen(&pt);
		transferMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, m_hWnd);

		return TRUE; 
	}
	return FALSE; 
}

LRESULT MainFrame::OnFileSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	
	SearchFrame* pChild = new SearchFrame();
	pChild->setTab(&ctrlTab);
	pChild->CreateEx(m_hWndClient);
	return 0;
}	

LRESULT MainFrame::onFavorites(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(FavoriteHubsFrame::frame == NULL) {
		FavoriteHubsFrame* pChild = new FavoriteHubsFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(FavoriteHubsFrame::frame->m_hWnd);
	}
	return 0;
}

LRESULT MainFrame::onFavoriteUsers(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(UsersFrame::frame == NULL) {
		UsersFrame* pChild = new UsersFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(UsersFrame::frame->m_hWnd);
	}
	return 0;
}

LRESULT MainFrame::onNotepad(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(NotepadFrame::frame == NULL) {
		NotepadFrame* pChild = new NotepadFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(NotepadFrame::frame->m_hWnd);
	}
	return 0;
}

LRESULT MainFrame::onQueue(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(QueueFrame::frame == NULL) {
		QueueFrame* pChild = new QueueFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(QueueFrame::frame->m_hWnd);
	}
	return 0;
}

LRESULT MainFrame::OnFileConnect(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(PublicHubsFrame::frame == NULL) {
		PublicHubsFrame* pChild = new PublicHubsFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(PublicHubsFrame::frame->m_hWnd);
	}

	return 0;
}

LRESULT MainFrame::onSearchSpy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if(SpyFrame::frame == NULL) {
		SpyFrame* pChild = new SpyFrame();
		pChild->setTab(&ctrlTab);
		pChild->CreateEx(m_hWndClient);
	} else {
		MDIActivate(SpyFrame::frame->m_hWnd);
	}
	
	return 0;
}

LRESULT MainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT MainFrame::OnFileSettings(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PropertiesDlg dlg(SettingsManager::getInstance());
	
	if(dlg.DoModal(m_hWnd) == IDOK)
	{		
		SettingsManager::getInstance()->save();
		
		if(SETTING(CONNECTION_TYPE) == SettingsManager::CONNECTION_ACTIVE) {
			try {
				ConnectionManager::getInstance()->setPort((short)SETTING(PORT));
				SearchManager::getInstance()->setPort((short)SETTING(PORT));
			} catch(Exception e) {
				dcdebug("MainFrame::OnCreate caught %s\n", e.getError().c_str());
				char* buf = new char[STRING(PORT_IS_BUSY).size() + 8];
				sprintf(buf, CSTRING(PORT_IS_BUSY), SETTING(PORT));
				MessageBox(buf);
				delete[] buf;
			}
		}
		ClientManager::getInstance()->infoUpdated();
	}
	return 0;
}

void MainFrame::onHttpComplete(HttpConnection* /*aConn*/)  {
	try {
		SimpleXML xml;
		xml.fromXML(versionInfo);
		xml.stepIn();
		if(xml.findChild("Version")) {
			if(atof(xml.getChildData().c_str()) > VERSIONFLOAT) {
				xml.resetCurrentChild();
				if(xml.findChild("Message")) {
					const string& msg = xml.getChildData();
					xml.resetCurrentChild();
					if(xml.findChild("Title")) {
						MessageBox(msg.c_str(), xml.getChildData().c_str());
					}
				}
				xml.resetCurrentChild();
				if(xml.findChild("VeryOldVersion")) {
					if(atof(xml.getChildData().c_str()) >= VERSIONFLOAT) {
						xml.resetCurrentChild();
						if(xml.findChild("URL")) {
							MessageBox(("Your version of DC++ is very old and will be shut down. Please get a new one at \r\n" + xml.getChildData()).c_str());
							oldshutdown = true;
							PostMessage(WM_CLOSE);
						}
					}
				}
			}
		}
	} catch (Exception e) {
		// ...
	}
}

LRESULT MainFrame::onGetToolTip(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	LPNMTTDISPINFO pDispInfo = (LPNMTTDISPINFO)pnmh;
	pDispInfo->szText[0] = 0;

	if((idCtrl != 0) && !(pDispInfo->uFlags & TTF_IDISHWND))
	{
		int stringId = -1;
		switch(idCtrl) {
			case ID_FILE_CONNECT: stringId = ResourceManager::MENU_FILE_PUBLIC_HUBS; break;
			case ID_FILE_RECONNECT: stringId = ResourceManager::MENU_FILE_RECONNECT; break;
			case IDC_FOLLOW: stringId = ResourceManager::MENU_FILE_FOLLOW_REDIRECT; break;
			case IDC_FAVORITES: stringId = ResourceManager::MENU_FILE_FAVORITE_HUBS; break;
			case IDC_QUEUE: stringId = ResourceManager::MENU_FILE_DOWNLOAD_QUEUE; break;
			case ID_FILE_SEARCH: stringId = ResourceManager::MENU_FILE_SEARCH; break;
			case ID_FILE_SETTINGS: stringId = ResourceManager::MENU_FILE_SETTINGS; break;
			case IDC_NOTEPAD: stringId = ResourceManager::MENU_FILE_NOTEPAD; break;
		}
		if(stringId != -1) {
			strncpy(pDispInfo->lpszText, ResourceManager::getInstance()->getString((ResourceManager::Strings)stringId).c_str(), 79);
			pDispInfo->uFlags |= TTF_DI_SETITEM;
		}
	}
	return 0;
}

void MainFrame::autoConnect(const FavoriteHubEntry::List& fl) {
	for(FavoriteHubEntry::List::const_iterator i = fl.begin(); i != fl.end(); ++i) {
		FavoriteHubEntry* entry = *i;
		if(entry->getConnect())
			HubFrame::openWindow(m_hWndMDIClient, &ctrlTab, entry->getServer(), entry->getNick(), entry->getPassword());
	}
}

LRESULT MainFrame::onPrivateMessage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		PrivateFrame::openWindow(((ConnectionQueueItem*)ctrlTransfers.GetItemData(i))->getUser(), m_hWndMDIClient, &ctrlTab);
	}
	return 0;
}

LRESULT MainFrame::onForce(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		ctrlTransfers.SetItemText(i, COLUMN_STATUS, CSTRING(CONNECTING_FORCED));
		((ConnectionQueueItem*)ctrlTransfers.GetItemData(i))->getUser()->connect();
	}
	return 0;
}

void MainFrame::updateTray(bool add /* = true */) {
	if(add) {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		nid.uCallbackMessage = WM_APP + 242;
		nid.hIcon = (HICON)::LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		strncpy(nid.szTip, "DC++",64);
		nid.szTip[63] = '\0';
		lastMove = GET_TICK() - 1000;
		::Shell_NotifyIcon(NIM_ADD, &nid);
		trayIcon = true;
	} else {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = 0;
		::Shell_NotifyIcon(NIM_DELETE, &nid);
		ShowWindow(SW_SHOW);
		trayIcon = false;		
	}
}
LRESULT MainFrame::onSize(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if(wParam == SIZE_MINIMIZED && BOOLSETTING(MINIMIZE_TRAY)) {
		updateTray(true);
		ShowWindow(SW_HIDE);
	} else if( ((wParam == SIZE_RESTORED) || wParam == SIZE_MAXIMIZED) && trayIcon) {
		updateTray(false);
	}
	
	bHandled = FALSE;
	return 0;
}

LRESULT MainFrame::onGetList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) { 
	int i = -1;
	while( (i = ctrlTransfers.GetNextItem(i, LVNI_SELECTED)) != -1) {
		try {
			QueueManager::getInstance()->addList(((ConnectionQueueItem*)ctrlTransfers.GetItemData(i))->getUser());
		} catch(QueueException e) {
			ctrlStatus.SetText(0, e.getError().c_str());
		} catch(FileException e) {
			dcdebug("MainFonGetList caught %s\n", e.getError().c_str());
		}
	}
	return 0;
}

LRESULT MainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	if(c != NULL) {
		c->removeListener(this);
		delete c;
		c = NULL;
	}

	DWORD id;
	if(stopperThread) {
		if(WaitForSingleObject(stopperThread, 0) == WAIT_TIMEOUT) {

			// Hm, the thread's not finished stopping the client yet...post a close message and continue processing...
			Thread::yield();
			PostMessage(WM_CLOSE);
			return 0;
		}
		CloseHandle(stopperThread);
		stopperThread = NULL;
		bHandled = FALSE;
	} else {
		if( oldshutdown ||(!BOOLSETTING(CONFIRM_EXIT)) || (MessageBox(CSTRING(REALLY_EXIT), "", MB_YESNO) == IDYES) ) {
			string tmp1;
			string tmp2;

			ctrlTransfers.GetColumnOrderArray(COLUMN_LAST, columnIndexes);
			for(int i = COLUMN_FIRST; i != COLUMN_LAST; i++) {
				columnSizes[i] = ctrlTransfers.GetColumnWidth(i);
				tmp1 += Util::toString(columnIndexes[i]) + ",";
				tmp2 += Util::toString(columnSizes[i]) + ",";
			}
			tmp1.erase(tmp1.size()-1, 1);
			tmp2.erase(tmp2.size()-1, 1);
			
			SettingsManager::getInstance()->set(SettingsManager::MAINFRAME_ORDER, tmp1);
			SettingsManager::getInstance()->set(SettingsManager::MAINFRAME_WIDTHS, tmp2);

			ShowWindow(SW_HIDE);
			stopperThread = CreateThread(NULL, 0, stopper, this, 0, &id);
		}
	}
	return 0;
}

LRESULT MainFrame::onLink(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {

	string site;

	switch(wID) {
	case IDC_HELP_README: site = Util::getAppPath() + "README.txt"; break;
	case IDC_HELP_HOMEPAGE: site = "http://dcplusplus.sourceforge.net"; break;
	case IDC_HELP_DOWNLOADS: site = "http://dcplusplus.sourceforge.net/index.php?page=download"; break;
	case IDC_HELP_FAQ: site = "http://dcplusplus.sourceforge.net/faq/faq.php?list=all&prog=1"; break;
	case IDC_HELP_HELP_FORUM: site = "http://dcpp.lichlord.org/forum"; break;
	case IDC_HELP_DISCUSS: site = "http://dcpp.lichlord.org/forum"; break;
	case IDC_HELP_REQUEST_FEATURE: site = "http://sourceforge.net/tracker/?atid=427635&group_id=40287&func=browse"; break;
	case IDC_HELP_REPORT_BUG: site = "http://sourceforge.net/tracker/?atid=427632&group_id=40287&func=browse"; break;
	default: dcassert(0);
	}

	ShellExecute(NULL, NULL, site.c_str(), NULL, NULL, SW_SHOWNORMAL);

	return 0;
}

LRESULT MainFrame::onImport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string file = Util::getAppPath() + "queue.config";
 	if(WinUtil::browseFile(file, m_hWnd, false) == IDOK) {
		try {
			QueueManager::getInstance()->importNMQueue(file);
 		} catch(FileException e) {
			ctrlStatus.SetText(0, CSTRING(ERROR_OPENING_FILE));
 		}
 	} 
 
 	return 0;
}
 
void MainFrame::UpdateLayout(BOOL bResizeBars /* = TRUE */)
{
	RECT rect;
	GetClientRect(&rect);
	// position bars and offset their dimensions
	UpdateBarsPosition(rect, bResizeBars);
	
	if(ctrlStatus.IsWindow()) {
		CRect sr;
		int w[6];
		ctrlStatus.GetClientRect(sr);
		int tmp = (sr.Width()) > 596 ? 496 : ((sr.Width() > 116) ? sr.Width()-100 : 16);
		
		w[0] = sr.right - tmp;
		w[1] = w[0] + (tmp-16)*1/5;
		w[2] = w[0] + (tmp-16)*2/5;
		w[3] = w[0] + (tmp-16)*3/5;
		w[4] = w[0] + (tmp-16)*4/5;
		w[5] = w[0] + (tmp-16)*5/5;
		
		ctrlStatus.SetParts(6, w);
	}
	CRect rc = rect;
	rc.top = rc.bottom - ctrlTab.getHeight();
	if(ctrlTab.IsWindow())
		ctrlTab.MoveWindow(rc);
	
	CRect rc2 = rect;
	rc2.bottom = rc.top;
	SetSplitterRect(rc2);
}

LRESULT MainFrame::onOpenFileList(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	string file;
	if(WinUtil::browseFile(file, m_hWnd, false, Util::getAppPath() + "FileLists\\")) {
		string username;
		if(file.rfind('\\') != string::npos) {
			username = file.substr(file.rfind('\\') + 1);
			if(username.rfind('.') != string::npos) {
				username = username.substr(0, username.rfind('.'));
			}
			User::Ptr& u = ClientManager::getInstance()->getUser(username);

			DirectoryListingFrame* pChild = new DirectoryListingFrame(file, u);
			pChild->setTab(&ctrlTab);
			pChild->CreateEx(m_hWndClient);
			pChild->setWindowTitle();
		}
	}
	return 0;
}

LRESULT MainFrame::onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam == WM_LBUTTONUP) {
		ShowWindow(SW_SHOW);
		ShowWindow(SW_RESTORE);
	} else if(lParam == WM_MOUSEMOVE && ((lastMove + 1000) < GET_TICK()) ) {
		NOTIFYICONDATA nid;
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = m_hWnd;
		nid.uID = 0;
		nid.uFlags = NIF_TIP;
		strncpy(nid.szTip, ("D: " + Util::formatBytes(DownloadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(DownloadManager::getInstance()->getDownloads()) + ")\r\nU: " +
			Util::formatBytes(UploadManager::getInstance()->getAverageSpeed()) + "/s (" + 
			Util::toString(UploadManager::getInstance()->getUploads()) + ")").c_str(), 64);
		
		::Shell_NotifyIcon(NIM_MODIFY, &nid);
		lastMove = GET_TICK();
	}
	return 0;
}

LRESULT MainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT MainFrame::onFinished(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
 	if(FinishedFrame::frame == NULL) {
 		FinishedFrame* pChild = new FinishedFrame();
 		pChild->setTab(&ctrlTab);
 		pChild->CreateEx(m_hWndClient);
 	} else {
 		MDIActivate(FinishedFrame::frame->m_hWnd);
	}
	return 0;
}

void MainFrame::onAction(UploadManagerListener::Types type, Upload* aUpload) {
	switch(type) {
		case UploadManagerListener::COMPLETE: onUploadComplete(aUpload); break;
		case UploadManagerListener::STARTING: onUploadStarting(aUpload); break;
		default: dcassert(0);
	}
}
void MainFrame::onAction(UploadManagerListener::Types type, const Upload::List& ul) {
	switch(type) {	
		case UploadManagerListener::TICK: onUploadTick(ul); break;
	}
}
void MainFrame::onAction(DownloadManagerListener::Types type, Download* aDownload) {
	switch(type) {
	case DownloadManagerListener::COMPLETE: onDownloadComplete(aDownload); break;
	case DownloadManagerListener::STARTING: onDownloadStarting(aDownload); break;
	default: dcassert(0); break;
	}
}
void MainFrame::onAction(DownloadManagerListener::Types type, const Download::List& dl) {
	switch(type) {	
	case DownloadManagerListener::TICK: onDownloadTick(dl); break;
	}
}
void MainFrame::onAction(DownloadManagerListener::Types type, Download* aDownload, const string& aReason) {
	switch(type) {
	case DownloadManagerListener::FAILED: onDownloadFailed(aDownload, aReason); break;
	default: dcassert(0); break;
	}
}
void MainFrame::onAction(ConnectionManagerListener::Types type, ConnectionQueueItem* aCqi) { 
	switch(type) {
		case ConnectionManagerListener::ADDED: onConnectionAdded(aCqi); break;
		case ConnectionManagerListener::CONNECTED: onConnectionConnected(aCqi); break;
		case ConnectionManagerListener::REMOVED: onConnectionRemoved(aCqi); break;
		case ConnectionManagerListener::STATUS_CHANGED: onConnectionStatus(aCqi); break;
		default: dcassert(0); break;
	}
};
void MainFrame::onAction(ConnectionManagerListener::Types type, ConnectionQueueItem* aCqi, const string& aLine) { 
	switch(type) {
		case ConnectionManagerListener::FAILED: onConnectionFailed(aCqi, aLine); break;
		default: dcassert(0); break;
	}
}

void MainFrame::onAction(TimerManagerListener::Types type, u_int32_t aTick) {
	if(type == TimerManagerListener::SECOND) {
		u_int32_t diff = (lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate;

		StringList* str = new StringList();
		str->push_back("Slots: " + Util::toString(UploadManager::getInstance()->getFreeSlots()) + '/' + Util::toString(SETTING(SLOTS)));
		str->push_back("D: " + Util::formatBytes(Socket::getTotalDown()));
		str->push_back("U: " + Util::formatBytes(Socket::getTotalUp()));
		str->push_back("D: " + Util::formatBytes(Socket::getDown()*1000/diff) + "/s (" + Util::toString(DownloadManager::getInstance()->getDownloads()) + ")");
		str->push_back("U: " + Util::formatBytes(Socket::getUp()*1000/diff) + "/s (" + Util::toString(UploadManager::getInstance()->getUploads()) + ")");
		PostMessage(WM_SPEAKER, STATS, (LPARAM)str);
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + Socket::getUp());
		SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + Socket::getDown());
		lastUpdate = aTick;
		Socket::resetStats();
	}
}

// HttpConnectionListener
void MainFrame::onAction(HttpConnectionListener::Types type, HttpConnection* conn) {
	switch(type) {
		case HttpConnectionListener::COMPLETE: onHttpComplete(conn); break;
	}
}
void MainFrame::onAction(HttpConnectionListener::Types type, HttpConnection* /*conn*/, const BYTE* buf, int len) {
	switch(type) {
		case HttpConnectionListener::DATA: versionInfo += string((const char*)buf, len); break;
	}
}

/**
 * @file MainFrm.cpp
 * $Id: MainFrm.cpp,v 1.14 2002/06/13 17:50:38 arnetheduck Exp $
 */

