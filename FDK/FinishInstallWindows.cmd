@echo off
rem v 1.2 July 28 2014

rem See comments in FDK\Tools\win\setFDKPaths.cmd for how this sets all the dir paths.
rem See comments in FDK\Tools\SharedData\FDKScripts\FinishInstallWindows.py for
rem how this set the system envonment variable PATH.

call "%~dp0\Tools\win\setFDKPaths.cmd"
%AFDKO_Python% %AFDKO_Scripts%\FinishInstallWindows.py
