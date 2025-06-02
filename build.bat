@echo off
g++ main.cpp -o word_maze.exe -municode -lgdi32 -luser32 -mwindows
if %errorlevel% equ 0 (
    echo Компиляция успешна!
    echo Запуск программы...
    start word_maze.exe
) else (
    echo Ошибка компиляции!
    pause
)