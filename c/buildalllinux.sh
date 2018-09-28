#! /bin/sh

# How to find and add all the xcode files for a new program:
# find .   \( -path "*/xcode/*" \)  -and \( \(  -name build.sh \) -or \( -name project.pbxproj \) -or \( -name contents.xcworkspacedata  \) \)  -exec p4 add {} \;curDir=`pwd`

set -e
set -x

curDir=$(pwd)

if [ -z "$1" ] || [ "$1" = "release" ] || [ "$1" = "debug" ] || [ "$1" = "clean" ]
then
	buildAllList=$(ls -1 ./*/build/linux/gcc/build.sh)
	for shFile in $buildAllList
	do
		echo "***Running $shFile"
		newDir=$(dirname "$shFile")
		shFile=$(basename "$shFile")
		cd "$newDir"
		sh	"$shFile" "$1"
		failed=$?
		cd "$curDir"
		if [ "$failed" -ne 0 ]; then
			exit "$failed"
		fi
	done
	echo "Done"
else
   echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')"
fi
