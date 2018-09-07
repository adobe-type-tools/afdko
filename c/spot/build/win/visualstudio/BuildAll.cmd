@echo off
cd %~dp0
setlocal enabledelayedexpansion
set targetProgram=spot

set do_release=0
set do_target=0

if "%1"=="" set do_release=1

if "%1"=="release" set do_release=1

if %do_release%==1 (
	set buildConfig=Build
	msbuild /target:!buildConfig! /p:configuration=Release %~dp0%targetProgram%.sln
	set do_target=1
)

if "%1"=="debug" (
	set buildConfig=Build
	msbuild /target:!buildConfig! /p:configuration=Debug  %~dp0%targetProgram%.sln
	set do_target=1
)

if %do_target%==1 (
	copy /Y ..\..\..\exe\win\release\%targetProgram%.exe ..\..\..\..\build_all
)

if "%1"=="clean" (
	set buildConfig=Clean
	msbuild /target:!buildConfig! /p:configuration=Debug  %~dp0%targetProgram%.sln
	msbuild /target:!buildConfig! /p:configuration=Release %~dp0%targetProgram%.sln
	set do_target=1
)
if %do_target%==0 (
	echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')"
)
