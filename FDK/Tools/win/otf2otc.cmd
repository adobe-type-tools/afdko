@echo off

rem See comments in setFDKPaths.cmd

call "%~dp0setFDKPaths.cmd"

%AFDKO_Python% %AFDKO_SCRIPTS%\otf2otc.py %*
