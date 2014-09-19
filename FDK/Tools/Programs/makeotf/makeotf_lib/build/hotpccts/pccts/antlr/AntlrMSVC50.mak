# Microsoft Developer Studio Generated NMAKE File, Based on AntlrMSVC50.dsp
!IF "$(CFG)" == ""
CFG=ANTLR - Win32 Debug
!MESSAGE No configuration specified. Defaulting to ANTLR - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "ANTLR - Win32 Release" && "$(CFG)" != "ANTLR - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "ANTLR - Win32 Release"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c" "antlr.c"\
 "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"

!ELSE 

ALL : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c" "antlr.c"\
 "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\antlr.obj"
	-@erase "$(INTDIR)\antlr.sbr"
	-@erase "$(INTDIR)\bits.obj"
	-@erase "$(INTDIR)\bits.sbr"
	-@erase "$(INTDIR)\build.obj"
	-@erase "$(INTDIR)\build.sbr"
	-@erase "$(INTDIR)\egman.obj"
	-@erase "$(INTDIR)\egman.sbr"
	-@erase "$(INTDIR)\err.obj"
	-@erase "$(INTDIR)\err.sbr"
	-@erase "$(INTDIR)\fcache.obj"
	-@erase "$(INTDIR)\fcache.sbr"
	-@erase "$(INTDIR)\fset.obj"
	-@erase "$(INTDIR)\fset.sbr"
	-@erase "$(INTDIR)\fset2.obj"
	-@erase "$(INTDIR)\fset2.sbr"
	-@erase "$(INTDIR)\gen.obj"
	-@erase "$(INTDIR)\gen.sbr"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\globals.sbr"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\hash.sbr"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\lex.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\misc.sbr"
	-@erase "$(INTDIR)\mrhoist.obj"
	-@erase "$(INTDIR)\mrhoist.sbr"
	-@erase "$(INTDIR)\pred.obj"
	-@erase "$(INTDIR)\pred.sbr"
	-@erase "$(INTDIR)\scan.obj"
	-@erase "$(INTDIR)\scan.sbr"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\set.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\Antlr.exe"
	-@erase "$(OUTDIR)\AntlrMSVC50.bsc"
	-@erase "antlr.c"
	-@erase "err.c"
	-@erase "mode.h"
	-@erase "parser.dlg"
	-@erase "scan.c"
	-@erase "tokens.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /I "..\h" /I "..\support\set" /D\
 "NDEBUG" /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D ZZLEXBUFSIZE=65536 /D\
 "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=./
CPP_SBRS=./

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\AntlrMSVC50.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\antlr.sbr" \
	"$(INTDIR)\bits.sbr" \
	"$(INTDIR)\build.sbr" \
	"$(INTDIR)\egman.sbr" \
	"$(INTDIR)\err.sbr" \
	"$(INTDIR)\fcache.sbr" \
	"$(INTDIR)\fset.sbr" \
	"$(INTDIR)\fset2.sbr" \
	"$(INTDIR)\gen.sbr" \
	"$(INTDIR)\globals.sbr" \
	"$(INTDIR)\hash.sbr" \
	"$(INTDIR)\lex.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\misc.sbr" \
	"$(INTDIR)\mrhoist.sbr" \
	"$(INTDIR)\pred.sbr" \
	"$(INTDIR)\scan.sbr" \
	"$(INTDIR)\set.sbr"

"$(OUTDIR)\AntlrMSVC50.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\Antlr.pdb" /machine:I386 /out:"$(OUTDIR)\Antlr.exe" 
LINK32_OBJS= \
	"$(INTDIR)\antlr.obj" \
	"$(INTDIR)\bits.obj" \
	"$(INTDIR)\build.obj" \
	"$(INTDIR)\egman.obj" \
	"$(INTDIR)\err.obj" \
	"$(INTDIR)\fcache.obj" \
	"$(INTDIR)\fset.obj" \
	"$(INTDIR)\fset2.obj" \
	"$(INTDIR)\gen.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mrhoist.obj" \
	"$(INTDIR)\pred.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\set.obj"

"$(OUTDIR)\Antlr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Copy antlr to ..\bin directory
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.
# End Custom Macros

$(DS_POSTBUILD_DEP) : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c"\
 "antlr.c" "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"
   mkdir ..\bin
	copy ..\bin\antlr.exe antlr_old.exe
	copy antlr.exe   ..\bin\.
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c" "antlr.c"\
 "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"

!ELSE 

ALL : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c" "antlr.c"\
 "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"

!ENDIF 

CLEAN :
	-@erase "$(INTDIR)\antlr.obj"
	-@erase "$(INTDIR)\antlr.sbr"
	-@erase "$(INTDIR)\bits.obj"
	-@erase "$(INTDIR)\bits.sbr"
	-@erase "$(INTDIR)\build.obj"
	-@erase "$(INTDIR)\build.sbr"
	-@erase "$(INTDIR)\egman.obj"
	-@erase "$(INTDIR)\egman.sbr"
	-@erase "$(INTDIR)\err.obj"
	-@erase "$(INTDIR)\err.sbr"
	-@erase "$(INTDIR)\fcache.obj"
	-@erase "$(INTDIR)\fcache.sbr"
	-@erase "$(INTDIR)\fset.obj"
	-@erase "$(INTDIR)\fset.sbr"
	-@erase "$(INTDIR)\fset2.obj"
	-@erase "$(INTDIR)\fset2.sbr"
	-@erase "$(INTDIR)\gen.obj"
	-@erase "$(INTDIR)\gen.sbr"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\globals.sbr"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\hash.sbr"
	-@erase "$(INTDIR)\lex.obj"
	-@erase "$(INTDIR)\lex.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\misc.sbr"
	-@erase "$(INTDIR)\mrhoist.obj"
	-@erase "$(INTDIR)\mrhoist.sbr"
	-@erase "$(INTDIR)\pred.obj"
	-@erase "$(INTDIR)\pred.sbr"
	-@erase "$(INTDIR)\scan.obj"
	-@erase "$(INTDIR)\scan.sbr"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\set.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Antlr.exe"
	-@erase "$(OUTDIR)\Antlr.ilk"
	-@erase "$(OUTDIR)\Antlr.pdb"
	-@erase "$(OUTDIR)\AntlrMSVC50.bsc"
	-@erase "antlr.c"
	-@erase "err.c"
	-@erase "mode.h"
	-@erase "parser.dlg"
	-@erase "scan.c"
	-@erase "tokens.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /I "..\h" /I "..\support\set"\
 /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D ZZLEXBUFSIZE=65536 /D "_DEBUG" /D\
 "WIN32" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=./
CPP_SBRS=./

.c{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_OBJS)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(CPP_SBRS)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\AntlrMSVC50.bsc" -DUSER_ZZSYN -D__STDC__ 
BSC32_SBRS= \
	"$(INTDIR)\antlr.sbr" \
	"$(INTDIR)\bits.sbr" \
	"$(INTDIR)\build.sbr" \
	"$(INTDIR)\egman.sbr" \
	"$(INTDIR)\err.sbr" \
	"$(INTDIR)\fcache.sbr" \
	"$(INTDIR)\fset.sbr" \
	"$(INTDIR)\fset2.sbr" \
	"$(INTDIR)\gen.sbr" \
	"$(INTDIR)\globals.sbr" \
	"$(INTDIR)\hash.sbr" \
	"$(INTDIR)\lex.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\misc.sbr" \
	"$(INTDIR)\mrhoist.sbr" \
	"$(INTDIR)\pred.sbr" \
	"$(INTDIR)\scan.sbr" \
	"$(INTDIR)\set.sbr"

"$(OUTDIR)\AntlrMSVC50.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\Antlr.pdb" /debug /machine:I386 /out:"$(OUTDIR)\Antlr.exe"\
 /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\antlr.obj" \
	"$(INTDIR)\bits.obj" \
	"$(INTDIR)\build.obj" \
	"$(INTDIR)\egman.obj" \
	"$(INTDIR)\err.obj" \
	"$(INTDIR)\fcache.obj" \
	"$(INTDIR)\fset.obj" \
	"$(INTDIR)\fset2.obj" \
	"$(INTDIR)\gen.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\lex.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\mrhoist.obj" \
	"$(INTDIR)\pred.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\set.obj"

"$(OUTDIR)\Antlr.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Copy antlr to ..\bin
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.
# End Custom Macros

$(DS_POSTBUILD_DEP) : "tokens.h" "scan.c" "parser.dlg" "mode.h" "err.c"\
 "antlr.c" "$(OUTDIR)\Antlr.exe" "$(OUTDIR)\AntlrMSVC50.bsc"
   mkdir ..\bin
	copy ..\bin\antlr.exe antlr_old.exe
	copy antlr.exe   ..\bin\.
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(CFG)" == "ANTLR - Win32 Release" || "$(CFG)" == "ANTLR - Win32 Debug"
SOURCE=.\antlr.c
DEP_CPP_ANTLR=\
	"..\h\antlr.h"\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\syn.h"\
	".\tokens.h"\
	

"$(INTDIR)\antlr.obj"	"$(INTDIR)\antlr.sbr" : $(SOURCE) $(DEP_CPP_ANTLR)\
 "$(INTDIR)" "$(INTDIR)\tokens.h" "$(INTDIR)\mode.h"


SOURCE=.\antlr.g

!IF  "$(CFG)" == "ANTLR - Win32 Release"

InputPath=.\antlr.g
InputName=antlr

"antlr.c"	"err.c"	"mode.h"	"scan.c"	"tokens.h"	"parser.dlg"	 : $(SOURCE)\
 "$(INTDIR)" "$(OUTDIR)"
	../bin/antlr -gh $(InputName).g 
	../bin/dlg -C2 parser.dlg scan.c 
	

!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

InputPath=.\antlr.g
InputName=antlr

"antlr.c"	"err.c"	"mode.h"	"scan.c"	"tokens.h"	"parser.dlg"	 : $(SOURCE)\
 "$(INTDIR)" "$(OUTDIR)"
	..\bin\antlr -gh $(InputName).g 
	..\bin\dlg -C2 parser.dlg scan.c 
	

!ENDIF 

SOURCE=.\bits.c
DEP_CPP_BITS_=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\bits.obj"	"$(INTDIR)\bits.sbr" : $(SOURCE) $(DEP_CPP_BITS_)\
 "$(INTDIR)"


SOURCE=.\build.c
DEP_CPP_BUILD=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\build.obj"	"$(INTDIR)\build.sbr" : $(SOURCE) $(DEP_CPP_BUILD)\
 "$(INTDIR)"


SOURCE=.\egman.c
DEP_CPP_EGMAN=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\egman.obj"	"$(INTDIR)\egman.sbr" : $(SOURCE) $(DEP_CPP_EGMAN)\
 "$(INTDIR)"


SOURCE=.\err.c
DEP_CPP_ERR_C=\
	"..\h\antlr.h"\
	"..\h\dlgdef.h"\
	"..\h\err.h"\
	"..\h\pccts_stdarg.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "ANTLR - Win32 Release"

CPP_SWITCHES=/nologo /ML /W3 /GX /O2 /I "." /I "..\h" /I "..\support\set" /D\
 "NDEBUG" /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D ZZLEXBUFSIZE=65536 /D\
 "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\"\
 /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\err.obj"	"$(INTDIR)\err.sbr" : $(SOURCE) $(DEP_CPP_ERR_C)\
 "$(INTDIR)" "$(INTDIR)\tokens.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "ANTLR - Win32 Debug"

CPP_SWITCHES=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /I "..\h" /I\
 "..\support\set" /D "LONGFILENAMES" /D "PC" /D "USER_ZZSYN" /D\
 ZZLEXBUFSIZE=65536 /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS"\
 /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

"$(INTDIR)\err.obj"	"$(INTDIR)\err.sbr" : $(SOURCE) $(DEP_CPP_ERR_C)\
 "$(INTDIR)" "$(INTDIR)\tokens.h"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF 

SOURCE=.\fcache.c
DEP_CPP_FCACH=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\fcache.obj"	"$(INTDIR)\fcache.sbr" : $(SOURCE) $(DEP_CPP_FCACH)\
 "$(INTDIR)"


SOURCE=.\fset.c
DEP_CPP_FSET_=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\fset.obj"	"$(INTDIR)\fset.sbr" : $(SOURCE) $(DEP_CPP_FSET_)\
 "$(INTDIR)"


SOURCE=.\fset2.c
DEP_CPP_FSET2=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\fset2.obj"	"$(INTDIR)\fset2.sbr" : $(SOURCE) $(DEP_CPP_FSET2)\
 "$(INTDIR)"


SOURCE=.\gen.c
DEP_CPP_GEN_C=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\gen.obj"	"$(INTDIR)\gen.sbr" : $(SOURCE) $(DEP_CPP_GEN_C)\
 "$(INTDIR)"


SOURCE=.\globals.c
DEP_CPP_GLOBA=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\globals.obj"	"$(INTDIR)\globals.sbr" : $(SOURCE) $(DEP_CPP_GLOBA)\
 "$(INTDIR)"


SOURCE=.\hash.c
DEP_CPP_HASH_=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	".\hash.h"\
	

"$(INTDIR)\hash.obj"	"$(INTDIR)\hash.sbr" : $(SOURCE) $(DEP_CPP_HASH_)\
 "$(INTDIR)"


SOURCE=.\lex.c
DEP_CPP_LEX_C=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\lex.obj"	"$(INTDIR)\lex.sbr" : $(SOURCE) $(DEP_CPP_LEX_C)\
 "$(INTDIR)"


SOURCE=.\main.c
DEP_CPP_MAIN_=\
	"..\h\antlr.h"\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\stdpccts.h"\
	".\syn.h"\
	".\tokens.h"\
	

"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_)\
 "$(INTDIR)" "$(INTDIR)\tokens.h" "$(INTDIR)\mode.h"


SOURCE=.\misc.c
DEP_CPP_MISC_=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\misc.obj"	"$(INTDIR)\misc.sbr" : $(SOURCE) $(DEP_CPP_MISC_)\
 "$(INTDIR)"


SOURCE=.\mrhoist.c
DEP_CPP_MRHOI=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\mrhoist.obj"	"$(INTDIR)\mrhoist.sbr" : $(SOURCE) $(DEP_CPP_MRHOI)\
 "$(INTDIR)"


SOURCE=.\pred.c
DEP_CPP_PRED_=\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\proto.h"\
	".\syn.h"\
	

"$(INTDIR)\pred.obj"	"$(INTDIR)\pred.sbr" : $(SOURCE) $(DEP_CPP_PRED_)\
 "$(INTDIR)"


SOURCE=.\scan.c
DEP_CPP_SCAN_=\
	"..\h\antlr.h"\
	"..\h\dlgauto.h"\
	"..\h\dlgdef.h"\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	".\generic.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\syn.h"\
	".\tokens.h"\
	

"$(INTDIR)\scan.obj"	"$(INTDIR)\scan.sbr" : $(SOURCE) $(DEP_CPP_SCAN_)\
 "$(INTDIR)" "$(INTDIR)\tokens.h" "$(INTDIR)\mode.h"


SOURCE=..\support\set\set.c
DEP_CPP_SET_C=\
	"..\h\pccts_stdio.h"\
	"..\h\pccts_stdlib.h"\
	"..\h\pccts_string.h"\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	

"$(INTDIR)\set.obj"	"$(INTDIR)\set.sbr" : $(SOURCE) $(DEP_CPP_SET_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

