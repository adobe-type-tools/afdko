# Microsoft Developer Studio Project File - Name="ANTLR" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=ANTLR - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AntlrMSVC50.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AntlrMSVC50.mak" CFG="ANTLR - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ANTLR - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "ANTLR - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ANTLR - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "."
# PROP Intermediate_Dir "."
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /O2 /I "." /I "..\h" /I "..\support\set" /D "__STDC__" /D "NDEBUG" /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D ZZLEXBUFSIZE=65536 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"./Antlr.exe"
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Copy antlr to ..\bin directory
PostBuild_Cmds=mkdir ..\bin	copy ..\bin\antlr.exe antlr_old.exe	copy antlr.exe\
       ..\bin\.
# End Special Build Tool

!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "."
# PROP Intermediate_Dir "."
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /Zi /Od /I "." /I "..\h" /I "..\support\set" /D "__STDC__" /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D ZZLEXBUFSIZE=65536 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo -DUSER_ZZSYN -D__STDC__
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"./Antlr.exe" /pdbtype:sept
# Begin Special Build Tool
SOURCE=$(InputPath)
PostBuild_Desc=Copy antlr to ..\bin
PostBuild_Cmds=mkdir ..\bin	copy ..\bin\antlr.exe antlr_old.exe	copy antlr.exe\
       ..\bin\.
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "ANTLR - Win32 Release"
# Name "ANTLR - Win32 Debug"
# Begin Source File

SOURCE=.\antlr.c
# End Source File
# Begin Source File

SOURCE=.\antlr.g

!IF  "$(CFG)" == "ANTLR - Win32 Release"

# Begin Custom Build - Building ANTLR Parser from ANTLR Grammar
InputPath=.\antlr.g
InputName=antlr

BuildCmds= \
	../bin/antlr -gh $(InputName).g \
	../bin/dlg -C2 parser.dlg scan.c \
	

"antlr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"err.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"mode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"scan.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"tokens.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.dlg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

# Begin Custom Build - Building ANTLR Parser from ANTLR Grammar
InputPath=.\antlr.g
InputName=antlr

BuildCmds= \
	..\bin\antlr -gh $(InputName).g \
	..\bin\dlg -C2 parser.dlg scan.c \
	

"antlr.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"err.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"mode.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"scan.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"tokens.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.dlg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bits.c
# End Source File
# Begin Source File

SOURCE=.\build.c
# End Source File
# Begin Source File

SOURCE=.\egman.c
# End Source File
# Begin Source File

SOURCE=.\err.c

!IF  "$(CFG)" == "ANTLR - Win32 Release"

!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

# SUBTRACT CPP /YX /Yc /Yu

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\fcache.c
# End Source File
# Begin Source File

SOURCE=.\fset.c
# End Source File
# Begin Source File

SOURCE=.\fset2.c
# End Source File
# Begin Source File

SOURCE=.\gen.c
# End Source File
# Begin Source File

SOURCE=.\globals.c
# End Source File
# Begin Source File

SOURCE=.\hash.c
# End Source File
# Begin Source File

SOURCE=.\lex.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\mrhoist.c
# End Source File
# Begin Source File

SOURCE=.\pred.c
# End Source File
# Begin Source File

SOURCE=.\scan.c
# End Source File
# Begin Source File

SOURCE=..\support\set\set.c
# End Source File
# End Target
# End Project
