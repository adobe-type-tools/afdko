cd %~dp0
setlocal enabledelayedexpansion
set targetProgram=autohint
set VCPATH="C:/Windows/Microsoft.NET/Framework/v4.0.30319/msbuild.exe"

set do_release=0
set do_target=0

if "%1"=="" set do_release=1

if "%1"=="release" set do_release=1

if %do_release%==1 (
	set buildConfig=Build
	%VCPATH% /target:!buildConfig! /p:configuration=Release %~dp0%targetProgram%.sln
	copy /Y ..\..\..\exe\win\release\%targetProgram%exe.exe ..\..\..\..\..\win\
	set do_target=1
)

if "%1"=="debug" (
	set buildConfig=Build
	%VCPATH% /target:!buildConfig! /p:configuration=Debug  %~dp0%targetProgram%.sln
	set do_target=1
)

if "%1"=="clean" (
	set buildConfig=Clean
	%VCPATH% /target:!buildConfig! /p:configuration=Debug  %~dp0%targetProgram%.sln
	%VCPATH% /target:!buildConfig! /p:configuration=Release %~dp0%targetProgram%.sln
	set do_target=1
)
if %do_target%==0 (
	echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')" 
)
