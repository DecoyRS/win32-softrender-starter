@echo off

mkdir w:\libs
pushd w:\libs

cl /LD /std:c++17 /Gv  ..\src\render.cpp -DDLL

popd

@echo on