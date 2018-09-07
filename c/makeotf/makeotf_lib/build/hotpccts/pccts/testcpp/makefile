#
# Makefile for C++ test examples
# Do a "make CCC=g++" or whatever if your make doesn't define CCC
# as the C++ compile macro.
#
# For SGI machines, we recommend using "-woff 3262" in your CFLAGS
#
#CCC = g++
CCC = CC
make:
	-(cd ./1; make CCC=$(CCC))
	-(cd ./2; make CCC=$(CCC))
	-(cd ./3; make CCC=$(CCC))
	-(cd ./4; make CCC=$(CCC))
	-(cd ./5; make CCC=$(CCC))
	-(cd ./6; make CCC=$(CCC))
	-(cd ./7; make CCC=$(CCC))
	-(cd ./8; make CCC=$(CCC))
	-(cd ./9; make CCC=$(CCC))
	-(cd ./10; make CCC=$(CCC))
	-(cd ./11; make CCC=$(CCC))
	-(cd ./12; make CCC=$(CCC))
	-(cd ./13; make CCC=$(CCC))

scrub:
	(cd ./1; make scrub)
	(cd ./2; make scrub)
	(cd ./3; make scrub)
	(cd ./4; make scrub)
	(cd ./5; make scrub)
	(cd ./6; make scrub; make -f makefile2 scrub)
	(cd ./7; make scrub)
	(cd ./8; make scrub)
	(cd ./9; make scrub)
	(cd ./10; make scrub)
	(cd ./11; make scrub)
	(cd ./12; make scrub)
	(cd ./13; make scrub)

tar:
	(make scrub; cd ..; tar cvfFF $(ARC)/testcpp.tar test.C++)
