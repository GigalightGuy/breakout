@echo off

pushd %~dp0
clang++ -o breakout.exe code/main.cpp -Wall -Werror -Wno-unused-function -luser32.lib -lgdi32.lib
popd
