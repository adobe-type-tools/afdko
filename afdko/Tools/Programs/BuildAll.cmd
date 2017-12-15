echo " "

echo "Building AddGlobalColoring..."

call %~dp0AddGlobalColoring\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building autohint..."

call %~dp0autohint\build\win\vc10\BuildAll.cmd %1
REM ai2bez is Mac only.

REM echo " "
REM echo "Building ai2bez..."

REM call %~dp0ai2bez\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building BezierLab..."

call %~dp0BezierLab\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building checkOutlines..."

call %~dp0checkOutlines\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building detype1..."

call %~dp0detype1\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building IS..."

call %~dp0IS\build\win\vc10\BuildAll.cmd %1

echo " "
echo "Building makeotf..."

call %~dp0makeotf\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building mergeFonts..."

call %~dp0mergeFonts\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building rotateFont..."

call %~dp0rotateFont\build\win\vc10\BuildAll.cmd %1
echo " "

echo "Building sfntdiff..."
call %~dp0sfntdiff\build\win\vc10\BuildAll.cmd %1

echo " "

echo "Building sfntedit..."

call %~dp0sfntedit\build\win\vc10\BuildAll.cmd %1
echo " "

echo "Building spot..."

call %~dp0spot\build\win\vc10\BuildAll.cmd %1

echo " "
echo "Building type1..."

call %~dp0type1\build\win\vc10\BuildAll.cmd %1

echo " "
echo "Building tx..."

call %~dp0tx\build\win\vc10\BuildAll.cmd %1

cd %~dp0