@echo off
echo ========================================
echo Тест сборки EndStone-PlayerRegister для Windows
echo ========================================
echo.

REM Проверка наличия необходимых файлов
echo Проверка структуры проекта...
if exist "CMakeLists.txt" (
    echo [OK] CMakeLists.txt найден
) else (
    echo [ERROR] CMakeLists.txt не найден
    pause
    exit /b 1
)

if exist "include" (
    echo [OK] Директория include найдена
) else (
    echo [ERROR] Директория include не найдена
    pause
    exit /b 1
)

if exist "src" (
    echo [OK] Директория src найдена
) else (
    echo [ERROR] Директория src не найдена
    pause
    exit /b 1
)

echo.
echo Проверка исходных файлов...
if exist "include\player_manager.h" (
    echo [OK] player_manager.h найден
) else (
    echo [ERROR] player_manager.h не найден
)

if exist "src\player_manager.cpp" (
    echo [OK] player_manager.cpp найден
) else (
    echo [ERROR] player_manager.cpp не найден
)

if exist "src\account_manager.cpp" (
    echo [OK] account_manager.cpp найден
) else (
    echo [ERROR] account_manager.cpp не найден
)

echo.
echo Проверка наличия Visual Studio...
where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] Компилятор Visual Studio найден
) else (
    echo [WARNING] Компилятор Visual Studio не найден в PATH
    echo Пожалуйста, запустите этот скрипт из Visual Studio Developer Command Prompt
)

echo.
echo Проверка наличия CMake...
where cmake >nul 2>&1
if %errorlevel% equ 0 (
    echo [OK] CMake найден
) else (
    echo [ERROR] CMake не найден в PATH
    echo Пожалуйста, установите CMake и добавьте его в PATH
    pause
    exit /b 1
)

echo.
echo ========================================
echo Начало сборки проекта
echo ========================================
echo.

REM Создание директории для сборки
if not exist "build" mkdir build
cd build

REM Очистка предыдущей сборки
if exist "CMakeCache.txt" (
    echo Очистка предыдущей сборки...
    del /q CMakeCache.txt
    del /q *.cmake
)

REM Конфигурация CMake
echo Конфигурация CMake...
cmake .. -G "Visual Studio 16 2019" -A x64
if %errorlevel% neq 0 (
    echo [ERROR] Ошибка конфигурации CMake
    cd ..
    pause
    exit /b 1
)

REM Сборка проекта
echo.
echo Сборка проекта...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Ошибка сборки проекта
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo Сборка завершена успешно!
echo ========================================
echo.

REM Проверка наличия скомпилированного файла
if exist "Release\player_register.dll" (
    echo [OK] player_register.dll успешно создан
    echo.
    echo Расположение файла: build\Release\player_register.dll
    echo.
    echo Для использования плагина:
    echo 1. Скопируйте player_register.dll в папку plugins вашего Endstone сервера
    echo 2. Перезапустите сервер
    echo 3. Проверьте логи сервера на наличие ошибок
) else (
    echo [ERROR] player_register.dll не найден
    cd ..
    pause
    exit /b 1
)

cd ..
echo.
echo ========================================
echo Тестирование завершено
echo ========================================
echo.
pause