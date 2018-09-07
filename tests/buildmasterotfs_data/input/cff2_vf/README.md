# TestVF

TestVF is a CFF2 variable font, derived from Source Serif Pro. It uses a subset of glyphs from Source Serif Pro. Some of the ufo font data has been further edited to provide useful test cases. The file 'cff2_vf.otf' used by several of the test scripts under afdko/Test.

## Building the fonts from source

cd to the parent directory for this VF font, and use the command
```sh
$ ./buildVF.sh
```
Note: psautohint must first be run manually, and then the vstems in 'T' must be manually edited in order to make them compatible. Use the command from this directory:
psautohint -all -m Masters/master_0/TestVF_0.ufo Masters/master_1/TestVF_1.ufo Masters/master_2/TestVF_2.ufo

Once this bug in psautohint is fixed, them it be uncommented in the buildVFs.ah
