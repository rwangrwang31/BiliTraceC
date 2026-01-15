@echo off
REM ============================================================
REM BiliTraceC Windows 编译脚本
REM 支持 MSYS2/MinGW 和 Visual Studio
REM ============================================================

echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║     BiliTraceC - Windows 编译脚本                        ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

REM 检测编译环境
where gcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [检测] 发现 GCC 编译器
    goto :COMPILE_GCC
)

where cl >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [检测] 发现 MSVC 编译器
    goto :COMPILE_MSVC
)

echo [错误] 未找到支持的编译器!
echo.
echo 请安装以下任一编译环境:
echo   1. MSYS2 + MinGW-w64: https://www.msys2.org/
echo      安装后运行: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-curl
echo.
echo   2. Visual Studio + vcpkg: https://visualstudio.microsoft.com/
echo      安装后运行: vcpkg install curl:x64-windows
echo.
goto :END

:COMPILE_GCC
echo [编译] 使用 GCC 编译...
echo.

REM 检查curl库
where curl-config >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [警告] 未找到 curl-config，尝试使用默认路径
    set CURL_CFLAGS=
    set CURL_LIBS=-lcurl
) else (
    for /f "tokens=*" %%i in ('curl-config --cflags') do set CURL_CFLAGS=%%i
    for /f "tokens=*" %%i in ('curl-config --libs') do set CURL_LIBS=%%i
)

gcc -O3 -Wall -pthread %CURL_CFLAGS% -o bilitrace.exe main.c cracker.c network.c %CURL_LIBS% -lws2_32

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ╔══════════════════════════════════════════════════════════╗
    echo ║  [成功] 编译完成: bilitrace.exe                          ║
    echo ╚══════════════════════════════════════════════════════════╝
    echo.
    echo 运行示例:
    echo   bilitrace.exe -hash bc28c067
    echo   bilitrace.exe -cid 123456789
    echo.
) else (
    echo.
    echo [错误] 编译失败!
    echo 请确保已安装 libcurl 开发库
    echo.
)
goto :END

:COMPILE_MSVC
echo [编译] 使用 MSVC 编译...
echo.
echo 请使用 CMake 进行 MSVC 编译:
echo   mkdir build
echo   cd build
echo   cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
echo   cmake --build . --config Release
echo.
goto :END

:END
pause
