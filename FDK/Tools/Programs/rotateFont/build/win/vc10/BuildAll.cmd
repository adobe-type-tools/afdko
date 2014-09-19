cd %~dp0
set buildConfig=Clean
set targetProgram=rotateFont

if "%1"=="" set buildConfig="Build"

set VCPATH="C:/Windows/Microsoft.NET/Framework/v4.0.30319/msbuild.exe"
%VCPATH% /target:%buildConfig% /p:configuration=Debug  %~dp0%targetProgram%.sln
%VCPATH% /target:%buildConfig% /p:configuration=Release %~dp0%targetProgram%.sln

if "%1"=="" copy /Y ..\..\..\exe\win\release\%targetProgram%.exe ..\..\..\..\..\win\