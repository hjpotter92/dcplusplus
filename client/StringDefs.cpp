#include "stdinc.h"
#include "DCPlusPlus.h"
string ResourceManager::strings[] = {
	"Add To Favorites", "Address already in use", "Address not available", "All download slots taken", "All %d users offline", "Already getting that file list", "Any", "At least", "At most", "Audio", 
"Auto connect / Name", "Away mode off", "Away mode on: ", "B", "Both users offline", "Browse...", "Can't connect to passive user while in passive mode", "Close connection", "Close", "Compressed", 
"Connect", "Connected", "Connecting...", "Connecting (forced)...", "Connecting to ", "Connection", "Connection closed", "Connection refused by target machine", "Connection reset by server", "Connection timeout", 
"Could not open target file: ", "Could not open file", "Count", "Description", "Directory already shared", "Disconnected", "Disk full(?)", "Document", "Done", "Download", 
"Download failed: ", "Download finished, idle...", "Download starting...", "Download to...", "Download queue", "Downloaded %s (%.01f%%), %s/s, %s left", " downloaded from ", "Downloading public hub list...", "E-Mail", "Please enter a nickname in the settings dialog!", 
"Please enter a password", "Please enter a reason", "Please enter a destination server", "Error opening file", "Executable", "Favorite Hubs", "Favorite Users", "File", "Files", "File list refreshed", 
"File type", "A file with a different size already exists in the queue", "Filename", "Filter", "Force attempt", "GB", "Get file list", "Grant extra slot", "High", "Host unreachable", 
"Hub", "Hubs", "Address", "Hub list downloaded...", "Name", "Hub password", "Ignored message: ", "Import queue from NMDC", "Invalid number of slots", "Joins: ", 
"Join/part showing off", "Join/part showing on", "kB", "Kick user(s)", "A file of equal or larger size already exists at the target location", "Hub (last seen on if offline)", "Low", "Manual connect address", "MB", "&File", 
"&Download Queue\tCtrl+D", "&Exit", "&Favorite Hubs\tCtrl+F", "Favorite &Users\tCtrl+U", "Follow last redirec&t\tCtrl+T", "&Notepad\tCtrl+N", "&Public Hubs\tCtrl+P", "&Reconnect\tCtrl+R", "&Search\tCtrl+S", "Search spy", 
"Settings...", "&Help", "About DC++...", "DC++ discussion forum", "Downloads and translations", "Frequently asked questions", "Help forum", "DC++ Homepage", "Request a feature", "Report a bug", 
"&View", "&Status bar", "&Toolbar", "&Window", "Arrange icons", "Cascade", "Tile", "New...", "Nick", "Your nick was already taken, please change to something else!", 
" (Nick unknown)", "Non-blocking operation still in progress", "Not connected", "Not a socket", "No directory specified", "No slots available", "No users to download from", "Normal", "Offline", "Online", 
"Only users with free slots", "Operation would block execution", "Out of buffer space", "Password", "Parts: ", "Path", "Paused", "Permission denied", "Picture", "Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++", 
"Preparing file list...", "Priority", "Private message from ", "Properties", "Public Hubs", "Really exit?", "Redirect", "Redirect user(s)", "Refresh", "Refresh user list", 
"Remove", "Remove all subdirectories before adding this one", "Remove source", "Rollback inconsistency, existing file does not match the one being downloaded", "Running...", "Search", "Search for", "Search for alternates", "Search options", "Search Spy", 
"Search string", "Searching for ", "Send private message", "Server", "Set priority", "Shared", "Size", "Slots", "Slots set", "Socket has been shut down", 
"Specify a server to connect to", "Specify a search string", "Status", "TB", "Timestamps disabled", "Timestamps enabled", "Type", "Unable to create thread", "Unknown", "Unknown address", 
"Unknown error: 0x%x", "Upload finished, idle...", "Upload starting...", "Uploaded %s (%.01f%%), %s/s, %s left", " uploaded to ", "User", "User offline", "User went offline", "Users", "Video", 
"Waiting...", "Waiting (User online)", "Waiting (%d of %d users online)", "Waiting to retry...", "You are being redirected to "
};
string ResourceManager::names[] = {
	"AddToFavorites", "AddressAlreadyInUse", "AddressNotAvailable", "AllDownloadSlotsTaken", "AllUsersOffline", "AlreadyGettingThatList", "Any", "AtLeast", "AtMost", "Audio", 
"AutoConnect", "AwayModeOff", "AwayModeOn", "B", "BothUsersOffline", "Browse", "CantConnectInPassiveMode", "CloseConnection", "Close", "Compressed", 
"Connect", "Connected", "Connecting", "ConnectingForced", "ConnectingTo", "Connection", "ConnectionClosed", "ConnectionRefused", "ConnectionReset", "ConnectionTimeout", 
"CouldNotOpenTargetFile", "CouldNotOpenFile", "Count", "Description", "DirectoryAlreadyShared", "Disconnected", "DiscFull", "Document", "Done", "Download", 
"DownloadFailed", "DownloadFinishedIdle", "DownloadStarting", "DownloadTo", "DownloadQueue", "DownloadedLeft", "DownloadedFrom", "DownloadingHubList", "Email", "EnterNick", 
"EnterPassword", "EnterReason", "EnterServer", "ErrorOpeningFile", "Executable", "FavoriteHubs", "FavoriteUsers", "File", "Files", "FileListRefreshed", 
"FileType", "FileWithDifferentSize", "Filename", "Filter", "ForceAttempt", "Gb", "GetFileList", "GrantExtraSlot", "High", "HostUnreachable", 
"Hub", "Hubs", "HubAddress", "HubListDownloaded", "HubName", "HubPassword", "IgnoredMessage", "ImportQueue", "InvalidNumberOfSlots", "Joins", 
"JoinShowingOff", "JoinShowingOn", "Kb", "KickUser", "LargerTargetFileExists", "LastHub", "Low", "ManualAddress", "Mb", "MenuFile", 
"MenuFileDownloadQueue", "MenuFileExit", "MenuFileFavoriteHubs", "MenuFileFavoriteUsers", "MenuFileFollowRedirect", "MenuFileNotepad", "MenuFilePublicHubs", "MenuFileReconnect", "MenuFileSearch", "MenuFileSearchSpy", 
"MenuFileSettings", "MenuHelp", "MenuHelpAbout", "MenuHelpDiscuss", "MenuHelpDownloads", "MenuHelpFaq", "MenuHelpHelpForum", "MenuHelpHomepage", "MenuHelpRequestFeature", "MenuHelpReportBug", 
"MenuView", "MenuViewStatusBar", "MenuViewToolbar", "MenuWindow", "MenuWindowArrange", "MenuWindowCascade", "MenuWindowTile", "New", "Nick", "NickTaken", 
"NickUnknown", "NonBlockingOperation", "NotConnected", "NotSocket", "NoDirectorySpecified", "NoSlotsAvailable", "NoUsersToDownload", "Normal", "Offline", "Online", 
"OnlyFreeSlots", "OperationWouldBlockExecution", "OutOfBufferSpace", "Password", "Parts", "Path", "Paused", "PermissionDenied", "Picture", "PortIsBusy", 
"PreparingFileList", "Priority", "PrivateMessageFrom", "Properties", "PublicHubs", "ReallyExit", "Redirect", "RedirectUser", "Refresh", "RefreshUserList", 
"Remove", "RemoveAllSubdirectories", "RemoveSource", "RollbackInconsistency", "Running", "Search", "SearchFor", "SearchForAlternates", "SearchOptions", "SearchSpy", 
"SearchString", "SearchingFor", "SendPrivateMessage", "Server", "SetPriority", "Shared", "Size", "Slots", "SlotsSet", "SocketShutDown", 
"SpecifyServer", "SpecifySearchString", "Status", "Tb", "TimestampsDisabled", "TimestampsEnabled", "Type", "UnableToCreateThread", "Unknown", "UnknownAddress", 
"UnknownError", "UploadFinishedIdle", "UploadStarting", "UploadedLeft", "UploadedTo", "User", "UserOffline", "UserWentOffline", "Users", "Video", 
"Waiting", "WaitingUserOnline", "WaitingUsersOnline", "WaitingToRetry", "YouAreBeingRedirected"
};
