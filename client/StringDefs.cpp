#include "stdinc.h"
#include "DCPlusPlus.h"
string ResourceManager::strings[] = {
	"Add To Favorites", "Address already in use", "Address not available", "All download slots taken", "All %d users offline", "Already getting that file list", "Any", "At least", "At most", "Audio", 
"Auto connect / Name", "Average/s: ", "Away mode off", "Away mode on: ", "B", "Both users offline", "Browse...", "Can't connect to passive user while in passive mode", "Choose folder", "Close connection", 
"Closing connection...", "Close", "Compressed", "Connect", "Connected", "Connecting...", "Connecting (forced)...", "Connecting to ", "Connection", "Connection closed", 
"Connection refused by target machine", "Connection reset by server", "Connection timeout", "Could not open target file: ", "Could not open file", "Count", "Description", "Directory already shared", "Disconnected", "Disk full(?)", 
"Document", "Done", "Download", "Download failed: ", "Download finished, idle...", "Download starting...", "Download to...", "Download queue", "Downloaded %s (%.01f%%), %s/s, %s left", " downloaded from ", 
"Downloading...", "Downloading public hub list...", "E-Mail", "Please enter a nickname in the settings dialog!", "Please enter a password", "Please enter a reason", "Enter search string", "Please enter a destination server", "Error opening file", "Executable", 
"Favorite Hubs", "Favorite Users", "File", "Files", "File list refreshed", "File not available", "File type", "A file with a different size already exists in the queue", "Filename", "Filter", 
"Find", "Finished downloads", "Force attempt", "GB", "Get file list", "Grant extra slot", "High", "Highest", "Hit Ratio: ", "Hits: ", 
"Host unreachable", "Hub", "Hubs", "Address", "Hub list downloaded...", "Name", "Hub password", "Users", "Ignored message: ", "Import queue from NMDC", 
"Invalid number of slots", "Joins: ", "Join/part showing off", "Join/part showing on", "kB", "Kick user(s)", "A file of equal or larger size already exists at the target location", "Hub (last seen on if offline)", "Loading DC++, please wait...", "Low", 
"Lowest", "Manual connect address", "MB", "&File", "&Download Queue\tCtrl+D", "&Exit", "&Favorite Hubs\tCtrl+F", "Favorite &Users\tCtrl+U", "Follow last redirec&t\tCtrl+T", "&Notepad\tCtrl+N", 
"&Public Hubs\tCtrl+P", "&Reconnect\tCtrl+R", "&Search\tCtrl+S", "Search spy", "Settings...", "&Help", "About DC++...", "DC++ discussion forum", "Downloads and translations", "Frequently asked questions", 
"Help forum", "DC++ Homepage", "Readme / Newbie help", "Request a feature", "Report a bug", "&View", "&Status bar", "&Toolbar", "&Window", "Minimize &All", 
"Arrange icons", "Cascade", "Tile", "Next", "New...", "Nick", "Your nick was already taken, please change to something else!", " (Nick unknown)", "Non-blocking operation still in progress", "Not connected", 
"Not a socket", "No directory specified", "You're trying to download from yourself!", "No matches", "No slots available", "No users", "No users to download from", "Normal", "Notepad", "Offline", 
"Online", "Only users with free slots", "Open file list", "Operation would block execution", "Out of buffer space", "Password", "Parts: ", "Path", "Paused", "Permission denied", 
"Picture", "Port %d is busy, please choose another one in the settings dialog, or disable any other application that might be using it and restart DC++", "Preparing file list...", "Press the follow redirect button to connect to ", "Priority", "Private message from ", "Properties", "Public Hubs", "Really exit?", "Redirect", 
"Redirect request received to a hub that's already connected", "Redirect user(s)", "Refresh", "Refresh user list", "Remove", "Remove all", "Remove all subdirectories before adding this one", "Remove source", "Rollback inconsistency, existing file does not match the one being downloaded", "Running...", 
"Search", "Search for", "Search for alternates", "Search for file", "Search options", "Search spam detected from ", "Search Spy", "Search string", "Searching for ", "Send private message", 
"Server", "Set priority", "Shared", "Shared Files", "Size", "Slot granted", "Slots", "Slots set", "Socket has been shut down", "Specify a server to connect to", 
"Specify a search string", "Speed", "Status", "TB", "Time", "Timestamps disabled", "Timestamps enabled", "Total: ", "Type", "Unable to create thread", 
"Unknown", "Unknown address", "Unknown error: 0x%x", "Upload finished, idle...", "Upload starting...", "Uploaded %s (%.01f%%), %s/s, %s left", " uploaded to ", "User", "User offline", "User went offline", 
"Users", "Video", "Waiting...", "Waiting (User online)", "Waiting (%d of %d users online)", "Waiting to retry...", "You are being redirected to "
};
string ResourceManager::names[] = {
	"AddToFavorites", "AddressAlreadyInUse", "AddressNotAvailable", "AllDownloadSlotsTaken", "AllUsersOffline", "AlreadyGettingThatList", "Any", "AtLeast", "AtMost", "Audio", 
"AutoConnect", "Average", "AwayModeOff", "AwayModeOn", "B", "BothUsersOffline", "Browse", "CantConnectInPassiveMode", "ChooseFolder", "CloseConnection", 
"ClosingConnection", "Close", "Compressed", "Connect", "Connected", "Connecting", "ConnectingForced", "ConnectingTo", "Connection", "ConnectionClosed", 
"ConnectionRefused", "ConnectionReset", "ConnectionTimeout", "CouldNotOpenTargetFile", "CouldNotOpenFile", "Count", "Description", "DirectoryAlreadyShared", "Disconnected", "DiscFull", 
"Document", "Done", "Download", "DownloadFailed", "DownloadFinishedIdle", "DownloadStarting", "DownloadTo", "DownloadQueue", "DownloadedLeft", "DownloadedFrom", 
"Downloading", "DownloadingHubList", "Email", "EnterNick", "EnterPassword", "EnterReason", "EnterSearchString", "EnterServer", "ErrorOpeningFile", "Executable", 
"FavoriteHubs", "FavoriteUsers", "File", "Files", "FileListRefreshed", "FileNotAvailable", "FileType", "FileWithDifferentSize", "Filename", "Filter", 
"Find", "FinishedDownloads", "ForceAttempt", "Gb", "GetFileList", "GrantExtraSlot", "High", "Highest", "HitRatio", "Hits", 
"HostUnreachable", "Hub", "Hubs", "HubAddress", "HubListDownloaded", "HubName", "HubPassword", "HubUsers", "IgnoredMessage", "ImportQueue", 
"InvalidNumberOfSlots", "Joins", "JoinShowingOff", "JoinShowingOn", "Kb", "KickUser", "LargerTargetFileExists", "LastHub", "Loading", "Low", 
"Lowest", "ManualAddress", "Mb", "MenuFile", "MenuFileDownloadQueue", "MenuFileExit", "MenuFileFavoriteHubs", "MenuFileFavoriteUsers", "MenuFileFollowRedirect", "MenuFileNotepad", 
"MenuFilePublicHubs", "MenuFileReconnect", "MenuFileSearch", "MenuFileSearchSpy", "MenuFileSettings", "MenuHelp", "MenuHelpAbout", "MenuHelpDiscuss", "MenuHelpDownloads", "MenuHelpFaq", 
"MenuHelpHelpForum", "MenuHelpHomepage", "MenuHelpReadme", "MenuHelpRequestFeature", "MenuHelpReportBug", "MenuView", "MenuViewStatusBar", "MenuViewToolbar", "MenuWindow", "MenuWindowMinimizeAll", 
"MenuWindowArrange", "MenuWindowCascade", "MenuWindowTile", "Next", "New", "Nick", "NickTaken", "NickUnknown", "NonBlockingOperation", "NotConnected", 
"NotSocket", "NoDirectorySpecified", "NoDownloadsFromSelf", "NoMatches", "NoSlotsAvailable", "NoUsers", "NoUsersToDownloadFrom", "Normal", "Notepad", "Offline", 
"Online", "OnlyFreeSlots", "OpenFileList", "OperationWouldBlockExecution", "OutOfBufferSpace", "Password", "Parts", "Path", "Paused", "PermissionDenied", 
"Picture", "PortIsBusy", "PreparingFileList", "PressFollow", "Priority", "PrivateMessageFrom", "Properties", "PublicHubs", "ReallyExit", "Redirect", 
"RedirectAlreadyConnected", "RedirectUser", "Refresh", "RefreshUserList", "Remove", "RemoveAll", "RemoveAllSubdirectories", "RemoveSource", "RollbackInconsistency", "Running", 
"Search", "SearchFor", "SearchForAlternates", "SearchForFile", "SearchOptions", "SearchSpamFrom", "SearchSpy", "SearchString", "SearchingFor", "SendPrivateMessage", 
"Server", "SetPriority", "Shared", "SharedFiles", "Size", "SlotGranted", "Slots", "SlotsSet", "SocketShutDown", "SpecifyServer", 
"SpecifySearchString", "Speed", "Status", "Tb", "Time", "TimestampsDisabled", "TimestampsEnabled", "Total", "Type", "UnableToCreateThread", 
"Unknown", "UnknownAddress", "UnknownError", "UploadFinishedIdle", "UploadStarting", "UploadedLeft", "UploadedTo", "User", "UserOffline", "UserWentOffline", 
"Users", "Video", "Waiting", "WaitingUserOnline", "WaitingUsersOnline", "WaitingToRetry", "YouAreBeingRedirected"
};
