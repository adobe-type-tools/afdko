"""
AFDKO Unified Command Invoker

This module provides the command registry, help system, and dispatch logic
for the unified 'afdko' command.
"""

import sys
from typing import NoReturn

# Complete command registry with abbreviations
# Format: name -> (module:function, description, category)
# Categories: 'primary', 'secondary', 'plot'

ALL_COMMANDS = {
    # PRIMARY COMMANDS - Most commonly used
    'tx': ('afdko._internal:tx', 'Font converter and analyzer',
           'primary'),
    'spot': ('afdko._internal:spot', 'SFNT font inspector', 'primary'),
    'sfntedit': ('afdko._internal:sfntedit', 'SFNT table editor',
                 'primary'),
    'se': ('afdko._internal:sfntedit', None, 'primary'),  # abbrev
    'mergefonts': ('afdko._internal:mergefonts', 'Merge font files',
                   'primary'),
    'mf': ('afdko._internal:mergefonts', None, 'primary'),  # abbrev
    'rotatefont': ('afdko._internal:rotatefont', 'Rotate font glyphs',
                   'primary'),
    'rf': ('afdko._internal:rotatefont', None, 'primary'),  # abbrev
    'makeotf': ('afdko.makeotf:main', 'Build OpenType font', 'primary'),
    'mo': ('afdko.makeotf:main', None, 'primary'),  # abbrev
    'buildcff2vf': ('afdko.buildcff2vf:main', 'Build CFF2 variable font',
                    'primary'),
    'bvf': ('afdko.buildcff2vf:main', None, 'primary'),  # abbrev
    'buildmasterotfs': ('afdko.buildmasterotfs:main',
                        'Build master OTF fonts', 'primary'),
    'bmo': ('afdko.buildmasterotfs:main', None, 'primary'),  # abbrev
    'makeinstancesufo': ('afdko.makeinstancesufo:main',
                         'Generate UFO instances', 'primary'),
    'miu': ('afdko.makeinstancesufo:main', None, 'primary'),  # abbrev
    'checkoutlinesufo': ('afdko.checkoutlinesufo:main',
                         'Check UFO outlines', 'primary'),
    'cou': ('afdko.checkoutlinesufo:main', None, 'primary'),  # abbrev
    'otfautohint': ('afdko.otfautohint.__main__:main', 'Auto-hint fonts',
                    'primary'),
    'autohint': ('afdko.otfautohint.__main__:main', None,
                 'primary'),  # abbrev
    'ah': ('afdko.otfautohint.__main__:main', None, 'primary'),  # abbrev
    'otfstemhist': ('afdko.otfautohint.__main__:stemhist',
                    'Generate stem histogram', 'primary'),
    'stemhist': ('afdko.otfautohint.__main__:stemhist', None,
                 'primary'),  # abbrev
    'sh': ('afdko.otfautohint.__main__:stemhist', None,
           'primary'),  # abbrev
    'addfeatures': ('afdko._internal:addfeatures',
                    'Feature file compiler', 'primary'),
    'af': ('afdko._internal:addfeatures', None, 'primary'),  # abbrev

    # SECONDARY COMMANDS - Less commonly used
    'type1': ('afdko._internal:type1', 'Type 1 font tool', 'secondary'),
    't1': ('afdko._internal:type1', None, 'secondary'),  # abbrev
    'detype1': ('afdko._internal:detype1', 'Type 1 font decompiler',
                'secondary'),
    'dt1': ('afdko._internal:detype1', None, 'secondary'),  # abbrev
    'sfntdiff': ('afdko._internal:sfntdiff', 'Compare SFNT fonts',
                 'secondary'),
    'comparefamily': ('afdko.comparefamily:main', 'Compare font family',
                      'secondary'),
    'cf': ('afdko.comparefamily:main', None, 'secondary'),  # abbrev
    'otc2otf': ('afdko.otc2otf:main', 'Extract OTFs from OTC',
                'secondary'),
    'otf2otc': ('afdko.otf2otc:main', 'Combine OTFs into OTC',
                'secondary'),
    'otf2ttf': ('afdko.otf2ttf:main', 'Convert OTF to TTF', 'secondary'),
    'ttfcomponentizer': ('afdko.ttfcomponentizer:main',
                         'Add TTF components', 'secondary'),
    'ttfdecomponentizer': ('afdko.ttfdecomponentizer:main',
                           'Remove TTF components', 'secondary'),
    'ttxn': ('afdko.ttxn:main', 'TTX wrapper', 'secondary'),

    # CONFIG COMMANDS - Configuration and setup
    'completion': ('afdko.completion:main', 'Generate shell completion script',
                   'config'),

    # PLOT COMMANDS - Proofing tools
    'charplot': ('afdko.proofpdf:charplot', 'Generate character proof',
                 'plot'),
    'digiplot': ('afdko.proofpdf:digiplot', 'Generate digitization proof',
                 'plot'),
    'fontplot': ('afdko.proofpdf:fontplot', 'Generate font proof', 'plot'),
    'fontplot2': ('afdko.proofpdf:fontplot2', 'Generate font proof (v2)',
                  'plot'),
    'fontsetplot': ('afdko.proofpdf:fontsetplot',
                    'Generate font set proof', 'plot'),
    'hintplot': ('afdko.proofpdf:hintplot', 'Generate hint proof', 'plot'),
    'waterfallplot': ('afdko.proofpdf:waterfallplot',
                      'Generate waterfall proof', 'plot'),
}


def print_category_commands(category: str) -> None:
    """Print commands for a specific category."""
    seen = set()
    for name, (target, desc, cat) in ALL_COMMANDS.items():
        if cat == category and desc is not None and target not in seen:
            # Find abbreviations for this command
            abbrevs = [k for k, (t, d, c) in ALL_COMMANDS.items()
                       if t == target and d is None and c == category]
            if abbrevs:
                abbrev_str = f" ({', '.join(abbrevs)})"
            else:
                abbrev_str = ""
            print(f"  {name:20} {desc}{abbrev_str}")
            seen.add(target)


def print_help(show_category: str = 'primary') -> None:
    """Print help message.

    Args:
        show_category: 'primary', 'secondary', 'plot', or 'all'
    """
    print("Usage: afdko <command> [options]")
    print("   or: afdko -h <command>  (get help for specific command)")
    print()
    print("AFDKO (Adobe Font Development Kit for OpenType) is "
          "Adobe's open source")
    print("toolkit for building, editing, validating, and proofing "
          "OpenType fonts")
    print("with emphasis on PostScript-lineage CFF and CFF2 output. "
          "It includes over")
    print("25 specialized tools for tasks ranging from compiling fonts "
          "with OpenType")
    print("layout features (makeotf), auto-hinting (otfautohint), and "
          "format conversion")
    print("(tx, otf2ttf), to quality validation (checkoutlinesufo, "
          "comparefamily) and")
    print("generating detailed proofs (spot, charplot).")
    print()

    if show_category in ('primary', 'all'):
        print("Primary Commands:")
        print_category_commands('primary')
        print()

    # Config commands shown with primary (distinctive display)
    if show_category in ('primary', 'all'):
        print("Configuration:")
        print_category_commands('config')
        print()

    if show_category in ('secondary', 'all'):
        print("Secondary Commands:")
        print_category_commands('secondary')
        print()

    if show_category in ('plot', 'all'):
        print("Proofing Commands:")
        print_category_commands('plot')
        print()

    if show_category == 'primary':
        print("Additional commands:")
        print("  afdko -s         List secondary commands")
        print("  afdko -p         List proofing commands")
        print("  afdko -a         List all commands")
        print()

    print("Run 'afdko <command> -h' or 'afdko -h <command>' for "
          "command-specific help.")


def dispatch_command(cmd: str) -> NoReturn:
    """Dispatch to the appropriate command implementation."""
    if cmd not in ALL_COMMANDS:
        print(f"Error: Unknown command '{cmd}'", file=sys.stderr)
        print("Run 'afdko --help' for usage.", file=sys.stderr)
        sys.exit(1)

    target, _, _ = ALL_COMMANDS[cmd]  # Unpack 3-tuple
    module_name, func_name = target.split(':')

    # Import and call the function
    try:
        module = __import__(module_name, fromlist=[func_name])
        func = getattr(module, func_name)
    except (ImportError, AttributeError) as e:
        print(f"Error loading command '{cmd}': {e}", file=sys.stderr)
        sys.exit(1)

    # Adjust sys.argv to look like the command was called directly
    # "afdko makeotf -f font.pfa" becomes "makeotf -f font.pfa"
    sys.argv = sys.argv[1:]

    # Call the function - it will handle sys.exit() internally
    result = func()

    # Handle return value (some commands return exit code)
    if result is None:
        sys.exit(0)
    elif isinstance(result, int):
        sys.exit(result)
    else:
        sys.exit(0)


def main() -> NoReturn:
    """Main entry point for the unified afdko command."""
    # No subcommand provided
    if len(sys.argv) < 2:
        print_help('primary')
        sys.exit(1)

    subcmd = sys.argv[1]

    # Top-level help flags
    if subcmd in ('-s', '--secondary'):
        print_help('secondary')
        sys.exit(0)
    elif subcmd in ('-p', '--plot'):
        print_help('plot')
        sys.exit(0)
    elif subcmd in ('-a', '--all'):
        print_help('all')
        sys.exit(0)

    # Help requested
    if subcmd in ('-h', '--help', 'help', '-u'):
        # Check for command name after -h/-u
        if len(sys.argv) > 2:
            arg = sys.argv[2]
            # afdko -h <command> -> afdko <command> -h
            # afdko -u <command> -> afdko <command> -h
            # Check if it's a valid command
            if arg in ALL_COMMANDS:
                sys.argv = ['afdko', arg, '-h']
                dispatch_command(arg)
            else:
                print(f"Error: Unknown command '{arg}'", file=sys.stderr)
                print("Run 'afdko --help' for usage.", file=sys.stderr)
                sys.exit(1)
        else:
            print_help('primary')
            sys.exit(0)

    # Dispatch to command
    dispatch_command(subcmd)


if __name__ == '__main__':
    main()
