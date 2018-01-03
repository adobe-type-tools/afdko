#!/bin/sh
# FDK tool GUI tool wrapper
# rroberts 2/15/2001
# asp 3/28/2000
#########################################################################
# initialize
#########################################################################

copyright=""" Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. """
scriptname=`/bin/basename $0`


#########################################################################
# Process options
#########################################################################
yes_copyright=false

for arg
do
	if [ "$arg" = "-@" ]
	then
		yes_copyright=true
	fi
done

if $yes_copyright
then
echo "$copyright"; exit 1
fi


#########################################################################
# Apply glyph renaming tool.
#########################################################################
LD_LIBRARY_PATH=/disks/quad/public/sparcsun/lib
export LD_LIBRARY_PATH
/disks/quad/public/sparcsun/bin/python $AB/FDKTools/Programs/makeotf/exe/MakeOTF.py "$@"
exit $?


