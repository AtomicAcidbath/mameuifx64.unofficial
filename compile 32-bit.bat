@echo off
set PATH=C:\mingw\mingw64-w32\bin\;C:\mingw\mingw64-w32\opt\bin;%PATH%
make OSD=winui
rename mameuifx.exe mameuifx32.exe
echo Generating XML file...
mameuifx32 -listxml >list.xml
mkdir gui
parser list.xml
copy *.ini gui
del *.ini
echo Job Done!
pause