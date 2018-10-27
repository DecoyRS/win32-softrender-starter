@echo off

mkdir w:\libs
pushd w:\libs

REM cl -Zi /LD /std:c++17 /Gv  ..\src\render.cpp -DDLL
cl /Ox /LD /std:c++17 /Gv  ..\src\render.cpp -DDLL

popd

@echo on