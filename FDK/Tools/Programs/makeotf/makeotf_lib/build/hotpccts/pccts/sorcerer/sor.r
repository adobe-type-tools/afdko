#include "cmdo.r"

resource 'cmdo' (128, "Sourcerer") {
	{	/* array dialogs: 2 elements */
		/* [1] */
		295,
		"Sorcerer -- Purdue Compiler Construction"
		" Tool Set (PCCTS) simple tree-parser gen"
		"erator.",
		{	/* array itemArray: 7 elements */
			/* [1] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{18, 23, 33, 223},
				"Read grammar from stdin",
				"-",
				"Read grammar from stdin."
			},
			/* [2] */
			NotDependent {

			},
			MultiFiles {
				"Grammar File(s)…",
				"Choose the grammar specification files y"
				"ou wish to have Sorcerer process.",
				{79, 22, 98, 152},
				"Grammar specification:",
				"",
				MultiInputFiles {
					{	/* array MultiTypesArray: 1 elements */
						/* [1] */
						text
					},
					".g",
					"Files ending in .g",
					"All text files"
				}
			},
			/* [3] */
			NotDependent {

			},
			Files {
				DirOnly,
				OptionalFile {
					{58, 168, 74, 298},
					{79, 169, 98, 299},
					"Output Directory",
					":",
					"-out-dir",
					"",
					"Choose the directory where SORCERER will"
					" put its output.",
					dim,
					"Output Directory…",
					"",
					""
				},
				NoMore {

				}
			},
			/* [4] */
			NotDependent {

			},
			Redirection {
				StandardOutput,
				{126, 27}
			},
			/* [5] */
			NotDependent {

			},
			Redirection {
				DiagnosticOutput,
				{126, 178}
			},
			/* [6] */
			NotDependent {

			},
			TextBox {
				gray,
				{117, 20, 167, 300},
				"Redirection"
			},
			/* [7] */
			NotDependent {

			},
			NestedDialog {
				2,
				{20, 324, 40, 460},
				"Options…",
				"Other options may be set with this butto"
				"n."
			}
		},
		/* [2] */
		295,
		"Use this dialog to specify command line "
		"Generate Options.",
		{	/* array itemArray: 10 elements */
			/* [1] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{18, 25, 33, 225},
				"Generate C++ code",
				"-CPP",
				"Turn on C++ output mode. You must define"
				" a class around your grammar rules. An \""
				".h\" and \".c\" file are created for the cl"
				"ass definition as well as the normal \".c"
				"\" file for the parser your grammar rules"
				"."
			},
			/* [2] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					1
				}
			},
			CheckOption {
				NotSet,
				{39, 25, 54, 354},
				"Define referenced tokens in class defini"
				"tion file",
				"-def-tokens",
				"For each token referenced in the grammar"
				", generate an enum STokenType definition"
				" in the class definition file. This shou"
				"ld not be used with the #tokedefs direct"
				"ive, which specifies token types you've "
				"already defined."
			},
			/* [3] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					-1
				}
			},
			RegularEntry {
				"Define referenced tokens in:",
				{63, 25, 78, 208},
				{63, 220, 79, 291},
				"",
				keepCase,
				"-def-tokens-file",
				"For each token referenced in the grammar"
				", generate a #define in the specified fi"
				"le This should not be used with the #tok"
				"defs  directive, which specifies token t"
				"ypes you've already defined."
			},
			/* [4] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					-1
				}
			},
			RadioButtons {
				{	/* array radioArray: 3 elements */
					/* [1] */
					{37, 374, 52, 434}, "ANSI", "-funcs ANSI", Set, "When this option is selected, SORCERER w"
					"ill generate ANSI style function headers"
					" and prototypes.",
					/* [2] */
					{55, 374, 70, 434}, "K&R", "-funcs KR", NotSet, "When this option is selected, SORCERER w"
					"ill generate K&R style function headers "
					"and prototypes.",
					/* [3] */
					{73, 374, 88, 434}, "Both", "-funcs both", NotSet, "When this option is selected, SORCERER w"
					"ill generate both ANSI and K&R style fun"
					"ction headers and prototypes."
				}
			},
			/* [5] */
			NotDependent {

			},
			TextBox {
				gray,
				{29, 359, 101, 450},
				"Style"
			},
			/* [6] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{90, 25, 105, 229},
				"Print out internal data structure",
				"-guts",
				"Print out a bunch of interna data struct"
				"ures."
			},
			/* [7] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{113, 25, 128, 226},
				"Transformation mode",
				"-transform",
				"Assume that a tree transformation will t"
				"ake place."
			},
			/* [8] */
			NotDependent {

			},
			CheckOption {
				NotSet,
				{137, 25, 152, 238},
				"Don't generate header info.. ",
				"-inline",
				"Only generate actions and functions for "
				"the given rules. Do not generate header "
				"information in the output. The usefulnes"
				"s of this options for anything but docum"
				"entation has not been established."
			},
			/* [9] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					-1
				}
			},
			RegularEntry {
				"Prefix:",
				{163, 25, 177, 83},
				{163, 98, 179, 193},
				" ",
				keepCase,
				"-prefix",
				"Prefix all globally visible symbols with"
				" this, including the error routines. Act"
				"ions that call rules must prefix the fun"
				"ction with this as well. "
			},
			/* [10] */
			Or {
				{	/* array OrArray: 1 elements */
					/* [1] */
					-1
				}
			},
			RegularEntry {
				"Proto file:",
				{163, 228, 178, 309},
				{163, 318, 179, 413},
				" ",
				keepCase,
				"-proto-file",
				"Put all prototypes for rule functions in"
				" this file. "
			}
		}
	}
};

