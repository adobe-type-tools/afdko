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
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i "::h:" ¶
		-i "::support:set:"
Sym¥68K      = 
ObjDir¥68K   = :Obj:

COptions     = {Includes} {Sym¥68K} -w off -model far -mc68020  -d  __STDC__  -d MPW -d USER_ZZSYN

Objects¥68K  = ¶
		"{ObjDir¥68K}cpp.c.o" ¶
		"{ObjDir¥68K}err.c.o" ¶
		"{ObjDir¥68K}gen.c.o" ¶
		"{ObjDir¥68K}globals.c.o" ¶
		"{ObjDir¥68K}hash.c.o" ¶
		"{ObjDir¥68K}look.c.o" ¶
		"{ObjDir¥68K}main.c.o" ¶
		"{ObjDir¥68K}scan.c.o" ¶
		"{ObjDir¥68K}sor.c.o" ¶
		"{ObjDir¥68K}set.c.o"


sor68K ÄÄ {¥MondoBuild¥} {Objects¥68K}
	Link ¶
		-o {Targ} -d {Sym¥68K} ¶
		{Objects¥68K} ¶
		-t 'MPST' ¶
		-c 'MPS ' ¶
		-mf ¶
		-model far ¶
		-br ON ¶
		-srtsg ALL ¶
		"{Libraries}Stubs.o" ¶
		#"{Libraries}MathLib.o" ¶
		#"{CLibraries}Complex.o" ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}IntEnv.o" ¶
		#"{Libraries}ToolLibs.o" ¶
		"{Libraries}Interface.o"


"{ObjDir¥68K}cpp.c.o" Ä {¥MondoBuild¥} cpp.c
	{C} cpp.c -o {Targ} {COptions}

"{ObjDir¥68K}err.c.o" Ä {¥MondoBuild¥} err.c
	{C} err.c -o {Targ} {COptions}

"{ObjDir¥68K}gen.c.o" Ä {¥MondoBuild¥} gen.c
	{C} gen.c -o {Targ} {COptions}

"{ObjDir¥68K}globals.c.o" Ä {¥MondoBuild¥} globals.c
	{C} globals.c -o {Targ} {COptions}

"{ObjDir¥68K}hash.c.o" Ä {¥MondoBuild¥} hash.c
	{C} hash.c -o {Targ} {COptions}

"{ObjDir¥68K}look.c.o" Ä {¥MondoBuild¥} look.c
	{C} look.c -o {Targ} {COptions}

"{ObjDir¥68K}main.c.o" Ä {¥MondoBuild¥} main.c
	{C} main.c -o {Targ} {COptions}

"{ObjDir¥68K}scan.c.o" Ä {¥MondoBuild¥} scan.c
	{C} scan.c -o {Targ} {COptions}

"{ObjDir¥68K}sor.c.o" Ä {¥MondoBuild¥} sor.c
	{C} sor.c -o {Targ} {COptions}

"{ObjDir¥68K}set.c.o" Ä {¥MondoBuild¥} "::support:set:set.c"
	{C} "::support:set:set.c" -o {Targ} {COptions}

sor68K ÄÄ sor.r
	Rez sor.r -o sor68K -a

Install  Ä sor68K
	Duplicate -y sor68K "{MPW}"Tools:sor
