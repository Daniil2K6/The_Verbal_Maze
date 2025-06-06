@echo off
chcp 65001 > nul
echo Компиляция Word Maze...

REM Проверяем, не запущена ли программа
taskkill /F /IM word_maze.exe 2>nul

REM Удаляем старые файлы если они существуют
if exist app.res del app.res
if exist word_maze.exe del word_maze.exe

REM Компилируем ресурсы
windres resources/resource.rc -O coff -o app.res
if %errorlevel% neq 0 (
    echo Ошибка при компиляции ресурсов!
    pause
    exit /b 1
)

REM Компилируем программу
g++ src/main.cpp app.res -o word_maze.exe -municode -lgdi32 -luser32 -mwindows -static -static-libgcc -static-libstdc++
if %errorlevel% neq 0 (
    echo Ошибка при компиляции программы!
    if exist app.res del app.res
    pause
    exit /b 1
)

REM Очищаем временные файлы
if exist app.res del app.res

echo Компиляция успешна!
echo Запуск программы...
start word_maze.exe