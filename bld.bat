@echo off
SET PROGNAME=totp.exe
mkdir output 2>NUL:
rem call "c:\Program Files\PellesC\Bin\povars32.bat"
del %PROGNAME%
echo compiling %PROGNAME%
pocc.exe -std:C11 -DLITTLE_ENDIAN_MACHINE -Tx86-coff -Ot -Ob1 -fp:precise -W1 -Gz -Ze -Go mysha1.c -Fo"output\mysha1.obj"
pocc.exe -std:C11 -DUSE_OWN_SHA1 -Tx86-coff -Ot -Ob1 -fp:precise -W1 -Gz -Ze -Go sha1_otp.c -Fo"output\sha1_otp.obj"
pocc.exe -std:C11 -Tx86-coff -Ot -Ob1 -fp:precise -W1 -Gz -Ze -Go b32.c -Fo"output\b32.obj"
pocc.exe -std:C11 -Tx86-coff -Ot -Ob1 -fp:precise -W1 -Gz -Ze -Go main.c -Fo"output\main.obj"
porc.exe "main.rc" -Fo"output\main.res"
echo linking
polink.exe -subsystem:windows -machine:x86 kernel32.lib user32.lib gdi32.lib comctl32.lib comdlg32.lib advapi32.lib delayimp.lib shell32.lib -out:"%PROGNAME%" "output\b32.obj" "output\main.obj" "output\sha1_otp.obj" "output\mysha1.obj" "output\main.res"
dir %PROGNAME%
