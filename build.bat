@echo off
windres icon/resource.rc -O coff -o app.res
g++ main.cpp app.res -o word_maze.exe -municode -lgdi32 -luser32 -mwindows -static -static-libgcc -static-libstdc++
del app.res
if %errorlevel% equ 0 (
    echo Компиляция успешна!
    echo Запуск программы...
    start word_maze.exe
) else (
    echo Ошибка компиляции!
    pause
)