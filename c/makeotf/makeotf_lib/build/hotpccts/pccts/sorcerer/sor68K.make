#   File:       sor68K.make
#   Target:     sor68K
#   Sources:    cpp.c
#               err.c
#               gen.c
#               globals.c
#               hash.c
#               look.c
#               main.c
#               scan.c
#               sor.c
#               ::support:set:set.c
#   Created:    Monday, May 18, 1998 12:20:50 AM
#	Author:		Kenji Tanaka


MAKEFILE     = sor68K.make
�MondoBuild� = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = �
		-i "::h:" �
		-i "::support:set:"
Sym�68K      = 
ObjDir�68K   = :Obj:

COptions     = {Includes} {Sym�68K} -w off -model far -mc68020  -d  __STDC__  -d MPW -d USER_ZZSYN

Objects�68K  = �
		"{ObjDir�68K}cpp.c.o" �
		"{ObjDir�68K}err.c.o" �
		"{ObjDir�68K}gen.c.o" �
		"{ObjDir�68K}globals.c.o" �
		"{ObjDir�68K}hash.c.o" �
		"{ObjDir�68K}look.c.o" �
		"{ObjDir�68K}main.c.o" �
		"{ObjDir�68K}scan.c.o" �
		"{ObjDir�68K}sor.c.o" �
		"{ObjDir�68K}set.c.o"


sor68K �� {�MondoBuild�} {Objects�68K}
	Link �
		-o {Targ} -d {Sym�68K} �
		{Objects�68K} �
		-t 'MPST' �
		-c 'MPS ' �
		-mf �
		-model far �
		-br ON �
		-srtsg ALL �
		"{Libraries}Stubs.o" �
		#"{Libraries}MathLib.o" �
		#"{CLibraries}Complex.o" �
		"{CLibraries}StdCLib.o" �
		"{Libraries}MacRuntime.o" �
		"{Libraries}IntEnv.o" �
		#"{Libraries}ToolLibs.o" �
		"{Libraries}Interface.o"


"{ObjDir�68K}cpp.c.o" � {�MondoBuild�} cpp.c
	{C} cpp.c -o {Targ} {COptions}

"{ObjDir�68K}err.c.o" � {�MondoBuild�} err.c
	{C} err.c -o {Targ} {COptions}

"{ObjDir�68K}gen.c.o" � {�MondoBuild�} gen.c
	{C} gen.c -o {Targ} {COptions}

"{ObjDir�68K}globals.c.o" � {�MondoBuild�} globals.c
	{C} globals.c -o {Targ} {COptions}

"{ObjDir�68K}hash.c.o" � {�MondoBuild�} hash.c
	{C} hash.c -o {Targ} {COptions}

"{ObjDir�68K}look.c.o" � {�MondoBuild�} look.c
	{C} look.c -o {Targ} {COptions}

"{ObjDir�68K}main.c.o" � {�MondoBuild�} main.c
	{C} main.c -o {Targ} {COptions}

"{ObjDir�68K}scan.c.o" � {�MondoBuild�} scan.c
	{C} scan.c -o {Targ} {COptions}

"{ObjDir�68K}sor.c.o" � {�MondoBuild�} sor.c
	{C} sor.c -o {Targ} {COptions}

"{ObjDir�68K}set.c.o" � {�MondoBuild�} "::support:set:set.c"
	{C} "::support:set:set.c" -o {Targ} {COptions}

sor68K �� sor.r
	Rez sor.r -o sor68K -a

Install  � sor68K
	Duplicate -y sor68K "{MPW}"Tools:sor
