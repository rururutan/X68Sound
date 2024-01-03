del *.dll *.exp *.lib *.obj

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

cl.exe /MT /EHsc /Ox /Ot /Oi /Ob2 /DC86CTL /c X68Sound.cpp
if %errorlevel% neq 0 (
	goto :fail
)
rc.exe x68sound.rc
if %errorlevel% neq 0 (
	goto :fail
)

link.exe /dll /def:x68sound.def x68sound.obj x68sound.res winmm.lib user32.lib
if %errorlevel% neq 0 (
	goto :fail
)

exit /b

:fail
pause
