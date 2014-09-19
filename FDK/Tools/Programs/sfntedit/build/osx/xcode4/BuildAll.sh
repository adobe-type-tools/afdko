#! /bin/sh
target=sfntedit
xcodebuild -target $target -project $target.xcodeproj -configuration Debug $1
xcodebuild -target $target -project $target.xcodeproj -configuration Release $1

if [ -z "$1" ]
then
	cp ../../../exe/osx/release/$target ../../../../../osx/
fi
