@echo off

rem See comments in setFDKPaths.cmd

call "%~dp0setFDKPaths.cmd"

%AFDKO_Python% %AFDKO_SCRIPTS%\otc2otf.py %*
