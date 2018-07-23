# Microsoft Developer Studio Project File - Name="support" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=support - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "support.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "support.mak" CFG="support - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "support - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "support - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""$/pccts/support", YGABAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe

!IF  "$(CFG)" == "support - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\h" /I "..\support\set" /I "..\support" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "USER_ZZSYN" /D "PC" /D "__STDC__" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "support - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Z7 /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /Z7 /Od /I "..\h" /I "..\support\set" /I "..\support" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "USER_ZZSYN" /D "PC" /D "__STDC__" /YX /FD /c
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "support - Win32 Release"
# Name "support - Win32 Debug"
# Begin Group "Headers"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\errout.h
# End Source File
# Begin Source File

SOURCE=.\set\set.h
# End Source File
# End Group
# Begin Group "Source"

# PROP Default_Filter "c,cpp"
# Begin Source File

SOURCE=.\errout.c
# End Source File
# Begin Source File

SOURCE=.\set\set.c
# End Source File
# End Group
# Begin Group "PCCTS' clients files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\h\antlr.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\AParser.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\AParser.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ast.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ast.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ASTBase.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ASTBase.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\AToken.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ATokenBuffer.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ATokenBuffer.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ATokenStream.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ATokPtr.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\ATokPtr.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\charbuf.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\charptr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\charptr.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\pcctscfg.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\DLexer.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\DLexerBase.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\DLexerBase.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\dlgauto.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\dlgdef.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\err.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\int.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\PBlackBox.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\PCCTSAST.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\PCCTSAST.h
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\SList.cpp
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\h\SList.h
# PROP Exclude_From_Build 1
# End Source File
# End Group
# End Target
# End Project
