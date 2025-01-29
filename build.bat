@echo off

pushd %~dp0
clang++ -o breakout.exe code/game.cpp code/audio_win32.cpp -g -Wall -Wextra -Werror -Wno-unused-function -luser32.lib -lgdi32.lib
popd
