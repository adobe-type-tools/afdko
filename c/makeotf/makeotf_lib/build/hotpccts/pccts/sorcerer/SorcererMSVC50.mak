# Microsoft Developer Studio Generated NMAKE File, Based on SorcererMSVC50.dsp
!IF "$(CFG)" == ""
CFG=Sorcerer - Win32 Debug
!MESSAGE No configuration specified. Defaulting to Sorcerer - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Sorcerer - Win32 Release" && "$(CFG)" !=\
 "Sorcerer - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "SorcererMSVC50.mak" CFG="Sorcerer - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Sorcerer - Win32 Release" (based on\
 "Win32 (x86) Console Application")
!MESSAGE "Sorcerer - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "Sorcerer - Win32 Release"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "stdpccts.h" "sor.c" "scan.c" "parser.dlg" "mode.h" "err.c"\
 "$(OUTDIR)\Sorcerer.exe"

!ELSE 

ALL : "DLG - Win32 Release" "ANTLR - Win32 Release" "stdpccts.h" "sor.c"\
 "scan.c" "parser.dlg" "mode.h" "err.c" "$(OUTDIR)\Sorcerer.exe"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ANTLR - Win32 ReleaseCLEAN" "DLG - Win32 ReleaseCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\cpp.obj"
	-@erase "$(INTDIR)\err.obj"
	-@erase "$(INTDIR)\gen.obj"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\look.obj"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\scan.obj"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\sor.obj"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(OUTDIR)\Sorcerer.exe"
	-@erase "err.c"
	-@erase "mode.h"
	-@erase "parser.dlg"
	-@erase "scan.c"
	-@erase "sor.c"
	-@erase "stdpccts.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "." /I "..\h" /I "..\support\set" /D\
 "NDEBUG" /D "LONGFILENAMES" /D "PC" /D ZZLEXBUFSIZE=65536 /D "_DEBUG" /D\
 "WIN32" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\SorcererMSVC50.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
CPP_OBJS=./
CPP_SBRS=.

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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SorcererMSVC50.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:no\
 /pdb:"$(OUTDIR)\Sorcerer.pdb" /machine:I386 /out:"$(OUTDIR)\Sorcerer.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cpp.obj" \
	"$(INTDIR)\err.obj" \
	"$(INTDIR)\gen.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\look.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sor.obj"

"$(OUTDIR)\Sorcerer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Update executables in BIN directory and remove intermediate\
         files
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.
# End Custom Macros

$(DS_POSTBUILD_DEP) : "DLG - Win32 Release" "ANTLR - Win32 Release"\
 "stdpccts.h" "sor.c" "scan.c" "parser.dlg" "mode.h" "err.c"\
 "$(OUTDIR)\Sorcerer.exe"
   del ..\bin\Win32\Antlr.old
	rename      ..\bin\Win32\Antlr.exe   Antlr.old
	copy ..\Antlr\Antlr.exe ..\bin\Win32
	del      ..\bin\Win32\Dlg.old
	rename ..\bin\Win32\Dlg.exe Dlg.old
	copy ..\Dlg\dlg.exe      ..\bin\Win32
	del   ..\bin\Win32\Sorcerer.old
	rename ..\bin\Win32\Sorcerer.exe      Sorcerer.old
	copy ..\Sorcerer\Sorcerer.exe ..\bin\Win32
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"

OUTDIR=.\.
INTDIR=.\.
# Begin Custom Macros
OutDir=.\.
# End Custom Macros

!IF "$(RECURSE)" == "0" 

ALL : "stdpccts.h" "sor.c" "scan.c" "parser.dlg" "mode.h" "err.c"\
 "$(OUTDIR)\Sorcerer.exe" "$(OUTDIR)\SorcererMSVC50.bsc"

!ELSE 

ALL : "DLG - Win32 Debug" "ANTLR - Win32 Debug" "stdpccts.h" "sor.c" "scan.c"\
 "parser.dlg" "mode.h" "err.c" "$(OUTDIR)\Sorcerer.exe"\
 "$(OUTDIR)\SorcererMSVC50.bsc"

!ENDIF 

!IF "$(RECURSE)" == "1" 
CLEAN :"ANTLR - Win32 DebugCLEAN" "DLG - Win32 DebugCLEAN" 
!ELSE 
CLEAN :
!ENDIF 
	-@erase "$(INTDIR)\cpp.obj"
	-@erase "$(INTDIR)\cpp.sbr"
	-@erase "$(INTDIR)\err.obj"
	-@erase "$(INTDIR)\err.sbr"
	-@erase "$(INTDIR)\gen.obj"
	-@erase "$(INTDIR)\gen.sbr"
	-@erase "$(INTDIR)\globals.obj"
	-@erase "$(INTDIR)\globals.sbr"
	-@erase "$(INTDIR)\hash.obj"
	-@erase "$(INTDIR)\hash.sbr"
	-@erase "$(INTDIR)\look.obj"
	-@erase "$(INTDIR)\look.sbr"
	-@erase "$(INTDIR)\main.obj"
	-@erase "$(INTDIR)\main.sbr"
	-@erase "$(INTDIR)\scan.obj"
	-@erase "$(INTDIR)\scan.sbr"
	-@erase "$(INTDIR)\set.obj"
	-@erase "$(INTDIR)\set.sbr"
	-@erase "$(INTDIR)\sor.obj"
	-@erase "$(INTDIR)\sor.sbr"
	-@erase "$(INTDIR)\vc50.idb"
	-@erase "$(INTDIR)\vc50.pdb"
	-@erase "$(OUTDIR)\Sorcerer.exe"
	-@erase "$(OUTDIR)\Sorcerer.ilk"
	-@erase "$(OUTDIR)\Sorcerer.pdb"
	-@erase "$(OUTDIR)\SorcererMSVC50.bsc"
	-@erase "err.c"
	-@erase "mode.h"
	-@erase "parser.dlg"
	-@erase "scan.c"
	-@erase "sor.c"
	-@erase "stdpccts.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /I "." /I "..\h" /I "..\support\set"\
 /D "LONGFILENAMES" /D "PC" /D ZZLEXBUFSIZE=65536 /D "_DEBUG" /D "WIN32" /D\
 "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\SorcererMSVC50.pch" /YX\
 /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
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
BSC32_FLAGS=/nologo /o"$(OUTDIR)\SorcererMSVC50.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cpp.sbr" \
	"$(INTDIR)\err.sbr" \
	"$(INTDIR)\gen.sbr" \
	"$(INTDIR)\globals.sbr" \
	"$(INTDIR)\hash.sbr" \
	"$(INTDIR)\look.sbr" \
	"$(INTDIR)\main.sbr" \
	"$(INTDIR)\scan.sbr" \
	"$(INTDIR)\set.sbr" \
	"$(INTDIR)\sor.sbr"

"$(OUTDIR)\SorcererMSVC50.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /nologo /subsystem:console /incremental:yes\
 /pdb:"$(OUTDIR)\Sorcerer.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)\Sorcerer.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cpp.obj" \
	"$(INTDIR)\err.obj" \
	"$(INTDIR)\gen.obj" \
	"$(INTDIR)\globals.obj" \
	"$(INTDIR)\hash.obj" \
	"$(INTDIR)\look.obj" \
	"$(INTDIR)\main.obj" \
	"$(INTDIR)\scan.obj" \
	"$(INTDIR)\set.obj" \
	"$(INTDIR)\sor.obj"

"$(OUTDIR)\Sorcerer.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

SOURCE=$(InputPath)
PostBuild_Desc=Update executables in BIN directory and remove intermediate\
         files
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\.
# End Custom Macros

$(DS_POSTBUILD_DEP) : "DLG - Win32 Debug" "ANTLR - Win32 Debug" "stdpccts.h"\
 "sor.c" "scan.c" "parser.dlg" "mode.h" "err.c" "$(OUTDIR)\Sorcerer.exe"\
 "$(OUTDIR)\SorcererMSVC50.bsc"
   del ..\bin\Win32\Antlr.old
	rename      ..\bin\Win32\Antlr.exe   Antlr.old
	copy ..\Antlr\Antlr.exe ..\bin\Win32
	del      ..\bin\Win32\Dlg.old
	rename ..\bin\Win32\Dlg.exe Dlg.old
	copy ..\Dlg\dlg.exe      ..\bin\Win32
	del   ..\bin\Win32\Sorcerer.old
	rename ..\bin\Win32\Sorcerer.exe      Sorcerer.old
	copy ..\Sorcerer\Sorcerer.exe ..\bin\Win32
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(CFG)" == "Sorcerer - Win32 Release" || "$(CFG)" ==\
 "Sorcerer - Win32 Debug"

!IF  "$(CFG)" == "Sorcerer - Win32 Release"

"ANTLR - Win32 Release" : 
   cd "\SDK\pccts\antlr"
   $(MAKE) /$(MAKEFLAGS) /F .\AntlrMSVC50.mak CFG="ANTLR - Win32 Release" 
   cd "..\sorcerer"

"ANTLR - Win32 ReleaseCLEAN" : 
   cd "\SDK\pccts\antlr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\AntlrMSVC50.mak CFG="ANTLR - Win32 Release"\
 RECURSE=1 
   cd "..\sorcerer"

!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"

"ANTLR - Win32 Debug" : 
   cd "\SDK\pccts\antlr"
   $(MAKE) /$(MAKEFLAGS) /F .\AntlrMSVC50.mak CFG="ANTLR - Win32 Debug" 
   cd "..\sorcerer"

"ANTLR - Win32 DebugCLEAN" : 
   cd "\SDK\pccts\antlr"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\AntlrMSVC50.mak CFG="ANTLR - Win32 Debug"\
 RECURSE=1 
   cd "..\sorcerer"

!ENDIF 

!IF  "$(CFG)" == "Sorcerer - Win32 Release"

"DLG - Win32 Release" : 
   cd "\SDK\pccts\dlg"
   $(MAKE) /$(MAKEFLAGS) /F .\DlgMSVC50.mak CFG="DLG - Win32 Release" 
   cd "..\sorcerer"

"DLG - Win32 ReleaseCLEAN" : 
   cd "\SDK\pccts\dlg"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\DlgMSVC50.mak CFG="DLG - Win32 Release"\
 RECURSE=1 
   cd "..\sorcerer"

!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"

"DLG - Win32 Debug" : 
   cd "\SDK\pccts\dlg"
   $(MAKE) /$(MAKEFLAGS) /F .\DlgMSVC50.mak CFG="DLG - Win32 Debug" 
   cd "..\sorcerer"

"DLG - Win32 DebugCLEAN" : 
   cd "\SDK\pccts\dlg"
   $(MAKE) /$(MAKEFLAGS) CLEAN /F .\DlgMSVC50.mak CFG="DLG - Win32 Debug"\
 RECURSE=1 
   cd "..\sorcerer"

!ENDIF 

SOURCE=.\cpp.c
DEP_CPP_CPP_C=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\stdpccts.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\cpp.obj" : $(SOURCE) $(DEP_CPP_CPP_C) "$(INTDIR)"\
 "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\cpp.obj"	"$(INTDIR)\cpp.sbr" : $(SOURCE) $(DEP_CPP_CPP_C)\
 "$(INTDIR)" "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\err.c
DEP_CPP_ERR_C=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\h\err.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\sor.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\err.obj" : $(SOURCE) $(DEP_CPP_ERR_C) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\err.obj"	"$(INTDIR)\err.sbr" : $(SOURCE) $(DEP_CPP_ERR_C)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\gen.c
DEP_CPP_GEN_C=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\stdpccts.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\gen.obj" : $(SOURCE) $(DEP_CPP_GEN_C) "$(INTDIR)"\
 "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\gen.obj"	"$(INTDIR)\gen.sbr" : $(SOURCE) $(DEP_CPP_GEN_C)\
 "$(INTDIR)" "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\globals.c
DEP_CPP_GLOBA=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\sor.h"\
	".\stdpccts.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\globals.obj" : $(SOURCE) $(DEP_CPP_GLOBA) "$(INTDIR)"\
 "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\globals.obj"	"$(INTDIR)\globals.sbr" : $(SOURCE) $(DEP_CPP_GLOBA)\
 "$(INTDIR)" "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\hash.c
DEP_CPP_HASH_=\
	".\hash.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\hash.obj" : $(SOURCE) $(DEP_CPP_HASH_) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\hash.obj"	"$(INTDIR)\hash.sbr" : $(SOURCE) $(DEP_CPP_HASH_)\
 "$(INTDIR)"


!ENDIF 

SOURCE=.\look.c
DEP_CPP_LOOK_=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\stdpccts.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\look.obj" : $(SOURCE) $(DEP_CPP_LOOK_) "$(INTDIR)"\
 "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\look.obj"	"$(INTDIR)\look.sbr" : $(SOURCE) $(DEP_CPP_LOOK_)\
 "$(INTDIR)" "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\main.c
DEP_CPP_MAIN_=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\stdpccts.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\main.obj" : $(SOURCE) $(DEP_CPP_MAIN_) "$(INTDIR)"\
 "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\main.obj"	"$(INTDIR)\main.sbr" : $(SOURCE) $(DEP_CPP_MAIN_)\
 "$(INTDIR)" "$(INTDIR)\stdpccts.h" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\scan.c
DEP_CPP_SCAN_=\
	"..\h\antlr.h"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgauto.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\scan.obj" : $(SOURCE) $(DEP_CPP_SCAN_) "$(INTDIR)"\
 "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\scan.obj"	"$(INTDIR)\scan.sbr" : $(SOURCE) $(DEP_CPP_SCAN_)\
 "$(INTDIR)" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=..\support\set\set.c
DEP_CPP_SET_C=\
	"..\h\pcctscfg.h"\
	"..\support\set\set.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\set.obj" : $(SOURCE) $(DEP_CPP_SET_C) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\set.obj"	"$(INTDIR)\set.sbr" : $(SOURCE) $(DEP_CPP_SET_C)\
 "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\sor.c
DEP_CPP_SOR_C=\
	"..\h\antlr.h"\
	"..\h\ast.c"\
	"..\h\ast.h"\
	"..\h\charbuf.h"\
	"..\h\pcctscfg.h"\
	"..\h\dlgdef.h"\
	"..\support\set\set.h"\
	".\hash.h"\
	".\mode.h"\
	".\proto.h"\
	".\sor.h"\
	".\sym.h"\
	".\tokens.h"\
	

!IF  "$(CFG)" == "Sorcerer - Win32 Release"


"$(INTDIR)\sor.obj" : $(SOURCE) $(DEP_CPP_SOR_C) "$(INTDIR)" "$(INTDIR)\mode.h"


!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"


"$(INTDIR)\sor.obj"	"$(INTDIR)\sor.sbr" : $(SOURCE) $(DEP_CPP_SOR_C)\
 "$(INTDIR)" "$(INTDIR)\mode.h"


!ENDIF 

SOURCE=.\sor.g

!IF  "$(CFG)" == "Sorcerer - Win32 Release"

InputPath=.\sor.g
InputName=sor

"parser.dlg"	"mode.h"	"stdpccts.h"	"sor.c"	"scan.c"	"err.c"	 : $(SOURCE)\
 "$(INTDIR)" "$(OUTDIR)"
	..\bin\Win32\antlr -gh -k 2 -gt $(InputName).g 
	..\bin\Win32\dlg -C2 parser.dlg scan.c 
	

!ELSEIF  "$(CFG)" == "Sorcerer - Win32 Debug"

InputPath=.\sor.g
InputName=sor

"parser.dlg"	"mode.h"	"stdpccts.h"	"sor.c"	"scan.c"	"err.c"	 : $(SOURCE)\
 "$(INTDIR)" "$(OUTDIR)"
	..\bin\Win32\antlr -gh -k 2 -gt $(InputName).g 
	..\bin\Win32\dlg -C2 parser.dlg scan.c 
	

!ENDIF 


!ENDIF 

