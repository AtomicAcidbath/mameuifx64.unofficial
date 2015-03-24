@echo off
set PATH=C:\mingw\mingw64-w64\bin\;C:\mingw\mingw64-w64\opt\bin;%PATH%
make OSD=winui
echo Generating XML file...
mameuifx64 -listxml >list.xml
mkdir gui
parser list.xml
copy *.ini gui
del *.ini
echo Job Done!
pause