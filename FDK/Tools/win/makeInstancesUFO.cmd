@echo off
rem v 2.6 Feb 3 2012


rem See comments in setFDKPaths.cmd

call "%~dp0setFDKPaths.cmd"

%AFDKO_Python% %AFDKO_SCRIPTS%\makeInstancesUFO.py %*
