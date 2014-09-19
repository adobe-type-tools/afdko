#   File:       sorPPC.make
#   Target:     sorPPC
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


MAKEFILE     = sorPPC.make
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i "::h:" ¶
		-i "::support:set:"
Sym¥PPC      = 
ObjDir¥PPC   = :Obj:

PPCCOptions  = {Includes} {Sym¥PPC} -w off -d  __STDC__  -d MPW -d USER_ZZSYN 

Objects¥PPC  = ¶
		"{ObjDir¥PPC}cpp.c.x" ¶
		"{ObjDir¥PPC}err.c.x" ¶
		"{ObjDir¥PPC}gen.c.x" ¶
		"{ObjDir¥PPC}globals.c.x" ¶
		"{ObjDir¥PPC}hash.c.x" ¶
		"{ObjDir¥PPC}look.c.x" ¶
		"{ObjDir¥PPC}main.c.x" ¶
		"{ObjDir¥PPC}scan.c.x" ¶
		"{ObjDir¥PPC}sor.c.x" ¶
		"{ObjDir¥PPC}set.c.x"

sorPPC ÄÄ {¥MondoBuild¥} {Objects¥PPC}
	PPCLink ¶
		-o {Targ} {Sym¥PPC} ¶
		{Objects¥PPC} ¶
		-t 'MPST' ¶
		-c 'MPS ' ¶
		"{SharedLibraries}InterfaceLib" ¶
		"{SharedLibraries}StdCLib" ¶
		"{SharedLibraries}MathLib" ¶
		"{PPCLibraries}StdCRuntime.o" ¶
		"{PPCLibraries}PPCCRuntime.o" ¶
		"{PPCLibraries}PPCToolLibs.o"

"{ObjDir¥PPC}cpp.c.x" Ä {¥MondoBuild¥} cpp.c
	{PPCC} cpp.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}err.c.x" Ä {¥MondoBuild¥} err.c
	{PPCC} err.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}gen.c.x" Ä {¥MondoBuild¥} gen.c
	{PPCC} gen.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}globals.c.x" Ä {¥MondoBuild¥} globals.c
	{PPCC} globals.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}hash.c.x" Ä {¥MondoBuild¥} hash.c
	{PPCC} hash.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}look.c.x" Ä {¥MondoBuild¥} look.c
	{PPCC} look.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}main.c.x" Ä {¥MondoBuild¥} main.c
	{PPCC} main.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}scan.c.x" Ä {¥MondoBuild¥} scan.c
	{PPCC} scan.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}sor.c.x" Ä {¥MondoBuild¥} sor.c
	{PPCC} sor.c -o {Targ} {PPCCOptions}

"{ObjDir¥PPC}set.c.x" Ä {¥MondoBuild¥} "::support:set:set.c"
	{PPCC} "::support:set:set.c" -o {Targ} {PPCCOptions}


sorPPC ÄÄ sor.r
	Rez sor.r -o sorPPC -a

Install  Ä sorPPC
	Duplicate -y sorPPC "{MPW}"Tools:sor
