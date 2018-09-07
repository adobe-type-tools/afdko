#!/usr/bin/env sh

rom=Roman/Masters

ro_name=TestVF

# build variable OTFs

# Note: psautohint must be run manually, and the vstems in 'T' must be manually edited in order
# to make them compatible.
# psautohint -all -m Masters/master_0/TestVF_0.ufo Masters/master_1/TestVF_1.ufo Masters/master_2/TestVF_2.ufo

buildmasterotfs $rom/$ro_name.designspace
buildcff2vf -p $rom/$ro_name.designspace cff2_vf.otf

echo "Done"
