@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /clr /EHa /utf-8 /nologo /I..\ComponentesConexas Pruebas.cpp
