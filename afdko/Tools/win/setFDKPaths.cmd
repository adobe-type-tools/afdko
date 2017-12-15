rem v 2.6 Feb 3 2012

REM "%~dp0" takes the argument %0 (the path to this script) and returns the drive and path to the
REM homedirectory of this script, i.e returns the absolute path minus the file name. Note
REM that this includes the final directory separator char.

set AFDKO_EXE_PATH="%~dp0"

set AFDKO_Python=%AFDKO_EXE_PATH:~0,-1%Python\AFDKOPython27\python"
set AFDKO_Python_BIN=%AFDKO_EXE_PATH:~0,-1%Python\AFDKOPython27\Scripts"

REM I need the path to this script's home directory.
REM I know that this scripts directory ends
REM in "\win\", so I can just strip off the last 5 chars.
REM note that in doing so, I strip off the closing quote char at the end of
REM AFDKO_EXE_PATH, so I need to add it again at the end of the path.

set AFDKO_Scripts=%AFDKO_EXE_PATH:~0,-5%SharedData\FDKScripts"
