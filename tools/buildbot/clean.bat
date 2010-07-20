rd /S/Q build
mkdir build
cd build
"C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv.exe" ^
/clean ^
release ^
RoR.sln
rem force clean exit
exit /b 0