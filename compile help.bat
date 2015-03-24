@echo off
set path=c:\mingw\mingw64-w32\bin\;%PATH%
makehm ID_,HID_,0x10000 IDM_,HIDM_,0x10000 src\osd\winui\resource.h >>"src\osd\winui\help\mameuifx.hm"
makehm IDP_,HIDP_,0x30000 src\osd\winui\resource.h >>"src\osd\winui\help\mameuifx.hm"
makehm IDR_,HIDR_,0x20000 src\osd\winui\resource.h >>"src\osd\winui\help\mameuifx.hm"
makehm IDD_,HIDD_,0x20000 src\osd\winui\resource.h >>"src\osd\winui\help\mameuifx.hm"
makehm IDW_,HIDW_,0x50000 src\osd\winui\resource.h >>"src\osd\winui\help\mameuifx.hm"
hhc "src\osd\winui\help\mameuifx.hhp"
copy "src\osd\winui\help\help.chm" .
pause

