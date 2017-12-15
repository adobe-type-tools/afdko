#! /bin/sh
target=makeotf
xcodebuild -target BuildAll -project $target.xcodeproj -configuration Debug $1
xcodebuild -target BuildAll -project $target.xcodeproj -configuration Release $1

if [ -z "$1" ]
then
	cp ../../../exe/osx/release/${target}exe ../../../../../osx/
fi
