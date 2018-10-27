@echo off

mkdir w:\build
pushd w:\build

REM cl -FC -Zi /std:c++17 /GR- /Gv /Oi ..\src\win32_main.cpp user32.lib gdi32.lib kernel32.lib
cl -FC /std:c++17 /GR- /Gv /Ox ..\src\win32_main.cpp user32.lib gdi32.lib kernel32.lib
popd

@echo on