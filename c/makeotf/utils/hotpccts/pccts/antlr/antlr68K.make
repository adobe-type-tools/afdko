#   File:       antlr68K.make
#   Target:     antlr68K
#   Sources:    ::support:set:set.c
#               antlr.c
#               bits.c
#               build.c
#               egman.c
#               err.c
#               fcache.c
#               fset2.c
#               fset.c
#               gen.c
#               globals.c
#               hash.c
#               lex.c
#               main.c
#               misc.c
#               mrhoist.c
#               pred.c
#               scan.c
#   Created:    Sunday, May 17, 1998 10:12:02 PM
#	Author:		Kenji Tanaka


MAKEFILE     = antlr68K.make
�MondoBuild� = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = �
		-i "::h:" �
		-i "::support:set:"
Sym�68K      = 
ObjDir�68K   = :Obj:

COptions     = {Includes} {Sym�68K} -model far -w off  -d MPW -d __STDC__=1 -d USER_ZZSYN

Objects�68K  = �
		"{ObjDir�68K}set.c.o" �
		"{ObjDir�68K}antlr.c.o" �
		"{ObjDir�68K}bits.c.o" �
		"{ObjDir�68K}build.c.o" �
		"{ObjDir�68K}egman.c.o" �
		"{ObjDir�68K}err.c.o" �
		"{ObjDir�68K}fcache.c.o" �
		"{ObjDir�68K}fset2.c.o" �
		"{ObjDir�68K}fset.c.o" �
		"{ObjDir�68K}gen.c.o" �
		"{ObjDir�68K}globals.c.o" �
		"{ObjDir�68K}hash.c.o" �
		"{ObjDir�68K}lex.c.o" �
		"{ObjDir�68K}main.c.o" �
		"{ObjDir�68K}misc.c.o" �
		"{ObjDir�68K}mrhoist.c.o" �
		"{ObjDir�68K}pred.c.o" �
		"{ObjDir�68K}scan.c.o"


antlr68K �� {�MondoBuild�} {Objects�68K}
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
		"{Libraries}ToolLibs.o" �
		"{Libraries}Interface.o"


"{ObjDir�68K}set.c.o" � {�MondoBuild�} "::support:set:set.c"
	{C} "::support:set:set.c" -o {Targ} {COptions}

"{ObjDir�68K}antlr.c.o" � {�MondoBuild�} antlr.c
	{C} antlr.c -o {Targ} {COptions}

"{ObjDir�68K}bits.c.o" � {�MondoBuild�} bits.c
	{C} bits.c -o {Targ} {COptions}

"{ObjDir�68K}build.c.o" � {�MondoBuild�} build.c
	{C} build.c -o {Targ} {COptions}

"{ObjDir�68K}egman.c.o" � {�MondoBuild�} egman.c
	{C} egman.c -o {Targ} {COptions}

"{ObjDir�68K}err.c.o" � {�MondoBuild�} err.c
	{C} err.c -o {Targ} {COptions}

"{ObjDir�68K}fcache.c.o" � {�MondoBuild�} fcache.c
	{C} fcache.c -o {Targ} {COptions}

"{ObjDir�68K}fset2.c.o" � {�MondoBuild�} fset2.c
	{C} fset2.c -o {Targ} {COptions}

"{ObjDir�68K}fset.c.o" � {�MondoBuild�} fset.c
	{C} fset.c -o {Targ} {COptions}

"{ObjDir�68K}gen.c.o" � {�MondoBuild�} gen.c
	{C} gen.c -o {Targ} {COptions}

"{ObjDir�68K}globals.c.o" � {�MondoBuild�} globals.c
	{C} globals.c -o {Targ} {COptions}

"{ObjDir�68K}hash.c.o" � {�MondoBuild�} hash.c
	{C} hash.c -o {Targ} {COptions}

"{ObjDir�68K}lex.c.o" � {�MondoBuild�} lex.c
	{C} lex.c -o {Targ} {COptions}

"{ObjDir�68K}main.c.o" � {�MondoBuild�} main.c
	{C} main.c -o {Targ} {COptions}

"{ObjDir�68K}misc.c.o" � {�MondoBuild�} misc.c
	{C} misc.c -o {Targ} {COptions}

"{ObjDir�68K}mrhoist.c.o" � {�MondoBuild�} mrhoist.c
	{C} mrhoist.c -o {Targ} {COptions}

"{ObjDir�68K}pred.c.o" � {�MondoBuild�} pred.c
	{C} pred.c -o {Targ} {COptions}

"{ObjDir�68K}scan.c.o" � {�MondoBuild�} scan.c
	{C} scan.c -o {Targ} {COptions}


antlr68K �� antlr.r
	Rez antlr.r -o antlr68K -a

Install  � antlr68K
	Duplicate -y antlr68K "{MPW}"Tools:antlr
