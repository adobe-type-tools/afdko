#   File:       dlg68K.make
#   Target:     dlg68K
#   Sources:    automata.c
#               dlg_a.c
#               dlg_p.c
#               err.c
#               main.c
#               output.c
#               relabel.c
#               support.c
#				::support:set:set.c
#   Created:    Sunday, May 17, 1998 10:58:10 PM
#	Author:		Kenji Tanaka


MAKEFILE     = dlg68K.make
¥MondoBuild¥ = {MAKEFILE}  # Make blank to avoid rebuilds when makefile is modified
Includes     = ¶
		-i "::h:" ¶
		-i "::support:set:"
Sym¥68K      = 
ObjDir¥68K   = ":Obj:"

COptions     = {Includes} {Sym¥68K} -model far -mc68020 -w off -d MPW -d __STDC__=1 -d USER_ZZSYN

Objects¥68K  = ¶
		"{ObjDir¥68K}automata.c.o" ¶
		"{ObjDir¥68K}dlg_a.c.o" ¶
		"{ObjDir¥68K}dlg_p.c.o" ¶
		"{ObjDir¥68K}err.c.o" ¶
		"{ObjDir¥68K}main.c.o" ¶
		"{ObjDir¥68K}output.c.o" ¶
		"{ObjDir¥68K}relabel.c.o" ¶
		"{ObjDir¥68K}support.c.o" ¶
		"{ObjDir¥68K}set.c.o"


dlg68K ÄÄ {¥MondoBuild¥} {Objects¥68K}
	Link ¶
		-o {Targ} -d {Sym¥68K} ¶
		{Objects¥68K} ¶
		-t 'MPST' ¶
		-c 'MPS ' ¶
		-model far ¶
		-mf ¶
		-br ON ¶
		-srtsg ALL ¶
		"{Libraries}Stubs.o" ¶
		"{Libraries}MathLib.o" ¶
		#"{CLibraries}Complex.o" ¶
		"{CLibraries}StdCLib.o" ¶
		"{Libraries}MacRuntime.o" ¶
		"{Libraries}IntEnv.o" ¶
		"{Libraries}ToolLibs.o" ¶
		"{Libraries}Interface.o"


"{ObjDir¥68K}automata.c.o" Ä {¥MondoBuild¥} automata.c
	{C} automata.c -o {Targ} {COptions}

"{ObjDir¥68K}dlg_a.c.o" Ä {¥MondoBuild¥} dlg_a.c
	{C} dlg_a.c -o {Targ} {COptions}

"{ObjDir¥68K}dlg_p.c.o" Ä {¥MondoBuild¥} dlg_p.c
	{C} dlg_p.c -o {Targ} {COptions}

"{ObjDir¥68K}err.c.o" Ä {¥MondoBuild¥} err.c
	{C} err.c -o {Targ} {COptions}

"{ObjDir¥68K}main.c.o" Ä {¥MondoBuild¥} main.c
	{C} main.c -o {Targ} {COptions}

"{ObjDir¥68K}output.c.o" Ä {¥MondoBuild¥} output.c
	{C} output.c -o {Targ} {COptions}

"{ObjDir¥68K}relabel.c.o" Ä {¥MondoBuild¥} relabel.c
	{C} relabel.c -o {Targ} {COptions}

"{ObjDir¥68K}support.c.o" Ä {¥MondoBuild¥} support.c
	{C} support.c -o {Targ} {COptions}

"{ObjDir¥68K}set.c.o" Ä {¥MondoBuild¥} "::support:set:set.c"
	{C} "::support:set:set.c" -o {Targ} {COptions}


dlg68K ÄÄ dlg.r
	Rez dlg.r -o dlg68K -a

Install  Ä dlg68K
	Duplicate -y dlg68K "{MPW}"Tools:dlg
