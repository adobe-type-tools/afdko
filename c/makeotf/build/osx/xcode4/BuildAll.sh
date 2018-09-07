#! /bin/sh
set -e
set -x

target=makeotf

if [ -z "$1" ] || [ "$1" = "release" ]
then
	xcodebuild -target BuildAll -project $target.xcodeproj -configuration Release
	cp ../../../exe/osx/release/${target}exe ../../../../build_all
elif [ "$1" = "debug" ]
then
	xcodebuild -target BuildAll -project $target.xcodeproj -configuration Debug
	cp ../../../exe/osx/debug/${target}exe ../../../../build_all
elif [ "$1" = "clean" ]
then
	xcodebuild -target BuildAll -project $target.xcodeproj -configuration Debug "$1"
	xcodebuild -target BuildAll -project $target.xcodeproj -configuration Release "$1"
else
   echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')"
fi
