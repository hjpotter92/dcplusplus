# Microsoft Developer Studio Project File - Name="client" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=client - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "client.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "client.mak" CFG="client - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "client - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "client - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "client - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "client___Win32_Release"
# PROP BASE Intermediate_Dir "client___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "vc6\Release\client"
# PROP Intermediate_Dir "vc6\Release\client"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G6 /Gr /MT /W4 /Gm /GX /Zi /Og /Oi /Os /Oy /Ob2 /Gf /Gy /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_REENTRANT" /D "BZ_NO_STDIO" /FAs /Yu"stdinc.h" /FD /c
# ADD BASE RSC /l 0x41d /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "client - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "client___Win32_Debug"
# PROP BASE Intermediate_Dir "client___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "vc6\Debug\client"
# PROP Intermediate_Dir "vc6\Debug\client"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W4 /Gm /Gi /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_REENTRANT" /D "BZ_NO_STDIO" /Yu"stdinc.h" /FD /c
# ADD BASE RSC /l 0x41d /d "_DEBUG"
# ADD RSC /l 0x41d /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "client - Win32 Release"
# Name "client - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\client\ADLSearch.cpp
# End Source File
# Begin Source File

SOURCE=.\client\BufferedSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\client\Client.cpp
# End Source File
# Begin Source File

SOURCE=.\client\ClientManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\ConnectionManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\CryptoManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\DCPlusPlus.cpp
# End Source File
# Begin Source File

SOURCE=.\client\DirectoryListing.cpp
# End Source File
# Begin Source File

SOURCE=.\client\DownloadManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\FinishedManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\HttpConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\client\HubManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\LogManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\QueueManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\ResourceManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\SearchManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\ServerSocket.cpp
# End Source File
# Begin Source File

SOURCE=.\client\SettingsManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\SFVReader.cpp
# End Source File
# Begin Source File

SOURCE=.\client\ShareManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\SimpleXML.cpp
# End Source File
# Begin Source File

SOURCE=.\client\Socket.cpp
# End Source File
# Begin Source File

SOURCE=.\client\stdinc.cpp
# ADD CPP /Yc"stdinc.h"
# End Source File
# Begin Source File

SOURCE=.\client\StringDefs.cpp
# End Source File
# Begin Source File

SOURCE=.\client\StringTokenizer.cpp
# End Source File
# Begin Source File

SOURCE=.\client\Thread.cpp
# End Source File
# Begin Source File

SOURCE=.\client\TimerManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\UploadManager.cpp
# End Source File
# Begin Source File

SOURCE=.\client\User.cpp
# End Source File
# Begin Source File

SOURCE=.\client\UserConnection.cpp
# End Source File
# Begin Source File

SOURCE=.\client\Util.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\client\ADLSearch.h
# End Source File
# Begin Source File

SOURCE=.\client\BitInputStream.h
# End Source File
# Begin Source File

SOURCE=.\client\BitOutputStream.h
# End Source File
# Begin Source File

SOURCE=.\client\BufferedSocket.h
# End Source File
# Begin Source File

SOURCE=.\client\Client.h
# End Source File
# Begin Source File

SOURCE=.\client\ClientManager.h
# End Source File
# Begin Source File

SOURCE=.\client\ClientManagerListener.h
# End Source File
# Begin Source File

SOURCE=.\client\config.h
# End Source File
# Begin Source File

SOURCE=.\client\ConnectionManager.h
# End Source File
# Begin Source File

SOURCE=.\client\CriticalSection.h
# End Source File
# Begin Source File

SOURCE=.\client\CryptoManager.h
# End Source File
# Begin Source File

SOURCE=.\client\DCPlusPlus.h
# End Source File
# Begin Source File

SOURCE=.\client\DirectoryListing.h
# End Source File
# Begin Source File

SOURCE=.\client\DownloadManager.h
# End Source File
# Begin Source File

SOURCE=.\client\Exception.h
# End Source File
# Begin Source File

SOURCE=.\client\File.h
# End Source File
# Begin Source File

SOURCE=.\client\FinishedManager.h
# End Source File
# Begin Source File

SOURCE=.\client\HttpConnection.h
# End Source File
# Begin Source File

SOURCE=.\client\HubManager.h
# End Source File
# Begin Source File

SOURCE=.\client\LogManager.h
# End Source File
# Begin Source File

SOURCE=.\client\Pointer.h
# End Source File
# Begin Source File

SOURCE=.\client\QueueManager.h
# End Source File
# Begin Source File

SOURCE=.\client\QueueManagerListener.h
# End Source File
# Begin Source File

SOURCE=.\client\ResourceManager.h
# End Source File
# Begin Source File

SOURCE=.\client\SearchManager.h
# End Source File
# Begin Source File

SOURCE=.\client\SearchManagerListener.h
# End Source File
# Begin Source File

SOURCE=.\client\Semaphore.h
# End Source File
# Begin Source File

SOURCE=.\client\ServerSocket.h
# End Source File
# Begin Source File

SOURCE=.\client\SettingsManager.h
# End Source File
# Begin Source File

SOURCE=.\client\SFVReader.h
# End Source File
# Begin Source File

SOURCE=.\client\ShareManager.h
# End Source File
# Begin Source File

SOURCE=.\client\SimpleXML.h
# End Source File
# Begin Source File

SOURCE=.\client\Singleton.h
# End Source File
# Begin Source File

SOURCE=.\client\Socket.h
# End Source File
# Begin Source File

SOURCE=.\client\stdinc.h
# End Source File
# Begin Source File

SOURCE=.\client\StringDefs.h
# End Source File
# Begin Source File

SOURCE=.\client\StringTokenizer.h
# End Source File
# Begin Source File

SOURCE=.\client\Thread.h
# End Source File
# Begin Source File

SOURCE=.\client\TimerManager.h
# End Source File
# Begin Source File

SOURCE=.\client\UploadManager.h
# End Source File
# Begin Source File

SOURCE=.\client\User.h
# End Source File
# Begin Source File

SOURCE=.\client\UserConnection.h
# End Source File
# Begin Source File

SOURCE=.\client\Util.h
# End Source File
# Begin Source File

SOURCE=.\client\version.h
# End Source File
# End Group
# End Target
# End Project
