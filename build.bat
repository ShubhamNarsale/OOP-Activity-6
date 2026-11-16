@echo off
REM Build the TechStore C++17 console application with MSYS2 UCRT64 GCC.
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp store.cpp file_manager.cpp password_utils.cpp -o techstore.exe -lbcrypt
if %errorlevel% neq 0 (
    echo Build FAILED.
    exit /b 1
)
echo Build successful. Run with: .\techstore.exe
