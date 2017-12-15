#! /bin/sh
target=makeotfexe
curdir=`pwd`

cd debug
make $1
cd $curdir

cd release
make $1
cd $curdir

if [ -z "$1" ]
then
	if [ $OSX ]; then
		cp -R ../../../exe/linux/release/$target ../../../../../linux/
	else
		cp -dR ../../../exe/linux/release/$target ../../../../../linux/
	fi
fi
