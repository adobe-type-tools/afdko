#! /bin/sh
set -e
set -x

target=sfntedit
curdir=`pwd`

if [ -z "$1" ] || [ $1 = "release" ]
then
	cd release
	make
	cd $curdir
	cp -dR ../../../exe/linux/release/$target ../../../../../linux/
elif [ $1 = "debug" ]
then
	cd debug
	make
	cd $curdir
	cp -dR ../../../exe/linux/debug/$target ../../../../../linux/
elif [ $1 = "clean" ]
then
	cd release
	make $1
	cd $curdir
	cd debug
	make $1
	cd $curdir
else
   echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')"
fi
