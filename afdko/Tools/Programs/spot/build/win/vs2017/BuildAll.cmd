cd %~dp0
set buildConfig=Clean
set targetProgram=spot

if "%1"=="" set buildConfig="Build"

REM Find location of latest VS installer. Works with only VS 2017 and later.
set vswbin="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=1* delims=: " %%a in (`%vswbin% -latest -requires Microsoft.Component.MSBuild`) do (
    if /i "%%a"=="installationPath" set vspath=%%b
)

echo ON
set msbuildPath=%vspath%\MSBuild\15.0\Bin

type %msbuildPath%\msbuild.exe
dir %msbuildPath%\msbuild.exe

"%msbuildPath%\msbuild.exe" /target:%buildConfig% /p:configuration=Debug  %~dp0%targetProgram%.sln

"%msbuildPath%\msbuild.exe" /target:%buildConfig% /p:configuration=Release %~dp0%targetProgram%.sln

if "%1"=="" copy /Y ..\..\..\exe\win\release\%targetProgram%.exe ..\..\..\..\..\win\
