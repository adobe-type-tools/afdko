cd %~dp0
set buildConfig=Clean
set targetProgram=spot

if "%1"=="" set buildConfig="Build"

echo ON

set msbuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\BuildTools\MSBuild\15.0\Bin\msbuild.exe"

if not exist %msbuildPath% (
set msbuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin\msbuild.exe"
)

if not exist %msbuildPath% (
set msbuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0\Bin\msbuild.exe"
)

if not exist %msbuildPath% (
set msbuildPath="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\msbuild.exe"
)
if not exist %msbuildPath% (
echo Cound not find the Windows compiler MSBuild.exe.
exit /b
)

%msbuildPath% /target:%buildConfig% /p:configuration=Debug  %~dp0%targetProgram%.sln

%msbuildPath% /target:%buildConfig% /p:configuration=Release %~dp0%targetProgram%.sln

if "%1"=="" copy /Y ..\..\..\exe\win\release\%targetProgram%.exe ..\..\..\..\..\win\
