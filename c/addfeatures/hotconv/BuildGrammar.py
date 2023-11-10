#!/usr/bin/env python

import subprocess
import argparse
import re
import sys
import os

antlr_program = "antlr4"
antlr_version = "4.13.1"

antlr_args = ['-no-listener', '-Dlanguage=Cpp']

lex_in = ['FeatLexer.g4', 'FeatLexerBase.g4']
lex_out = ['FeatLexer.cpp', 'FeatLexer.h', 'FeatLexer.interp',
           'FeatLexer.tokens']

parse_in = ['FeatParser.g4']
parse_out = ['FeatParser.cpp', 'FeatParser.h',
             'FeatParserVisitor.h', 'FeatParserVisitor.cpp',
             'FeatParser.interp', 'FeatParserBaseVisitor.h',
             'FeatParserBaseVisitor.cpp', 'FeatParser.tokens']

parser = argparse.ArgumentParser()
parser.add_argument('-c', '--clean',
                    help="remove generated files",
                    action="store_true")
parser.add_argument('-n', '--dry-run',
                    help="just print what the program would do",
                    action="store_true")
parser.add_argument('-f', '--force',
                    help="Regenerate even if current files are newer",
                    action="store_true")
args = parser.parse_args()


def q(r, *m, **kwargs):
    print(*m, file=sys.stderr, **kwargs)
    sys.exit(r)


def file_newer(p1, p2):
    return os.path.exists(p1) and os.path.getmtime(p1) > os.path.getmtime(p2)


def check_version():
    try:
        command = subprocess.run([antlr_program], capture_output=True)
    except (subprocess.CalledProcessError, OSError):
        q(1, "ERROR: Did not find '" + antlr_program + "' program in path")
    v = re.search(b'([0-9.]+)\n', command.stdout).group(1).decode('ascii')
    if v != antlr_version:
        q(1, "ERROR: Program version is " + v + " should be " + antlr_version)


def run_program(prog):
    print(' '.join(prog))
    if not args.dry_run:
        a = subprocess.run(prog, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
        print(a.stdout.decode('ascii'), end='')


check_version()

os.chdir(os.path.dirname(os.path.abspath(__file__)))

if args.clean:
    d = []
    for fp in lex_out + parse_out:
        if os.path.exists(fp):
            d.append(fp)
    if len(d) == 0:
        print("No files to remove")
    if args.dry_run:
        print('Remove files ' + ' '.join(d))
    else:
        for fp in d:
            os.remove(fp)
else:
    lexer_ok = all([file_newer(lex_out[0], p) for p in lex_in])
    parser_ok = all([file_newer(parse_out[0], p) for p in lex_in + parse_in])

    if lexer_ok and parser_ok and not args.force:
        q(0, "Generated files are newer than sources")

    if not lexer_ok or args.force:
        run_program([antlr_program] + antlr_args + [lex_in[0]])
    if not parser_ok or args.force:
        run_program([antlr_program] + antlr_args + ['-visitor'] +
                    [parse_in[0]])
