set do_target=0
if "%1"=="" set do_target=1
if "%1"=="release" set do_target=1
if "%1"=="debug" set do_target=1
if "%1"=="clean" set do_target=1
if %do_target%==0 (
	echo "Build target must be 'release', 'debug', 'clean', or simply omitted (same as 'release')"
	exit /B
)

echo " "
echo "Building autohint..."
call %~dp0autohint\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building detype1..."
call %~dp0detype1\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building makeotf..."
call %~dp0makeotf\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building mergeFonts..."
call %~dp0mergeFonts\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building rotateFont..."
call %~dp0rotateFont\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building sfntdiff..."
call %~dp0sfntdiff\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building sfntedit..."
call %~dp0sfntedit\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building spot..."
call %~dp0spot\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building type1..."
call %~dp0type1\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

echo " "
echo "Building tx..."
call %~dp0tx\build\win\vc10\BuildAll.cmd %1 || EXIT /B 1

cd %~dp0
