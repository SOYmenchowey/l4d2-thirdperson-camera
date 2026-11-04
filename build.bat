@echo off
setlocal
call :load_msvc
if errorlevel 1 exit /b 1
cd /d "%~dp0"
if not exist "build\obj" mkdir "build\obj"
echo [*] Compilando overlay y camara de tercera persona (x86)...
cl /nologo /O2 /MT /EHsc /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0600 /LD ^
    src\core\dllmain.cpp ^
    src\thirdperson\thirdperson.cpp ^
    src\overlay\dx9_hook.cpp ^
    src\overlay\menu.cpp ^
    third_party\imgui\imgui.cpp ^
    third_party\imgui\imgui_draw.cpp ^
    third_party\imgui\imgui_tables.cpp ^
    third_party\imgui\imgui_widgets.cpp ^
    third_party\imgui\imgui_impl_dx9.cpp ^
    third_party\imgui\imgui_impl_win32.cpp ^
    /I"src" /I"third_party\imgui" /Fo"build\obj\\" ^
    /Fe:build\l4d2_thirdperson.dll /link /OUT:build\l4d2_thirdperson.dll user32.lib d3d9.lib
if errorlevel 1 (
    echo [!] Compilacion fallo
    exit /b 1
)
copy /Y "build\l4d2_thirdperson.dll" "l4d2_thirdperson.dll" >nul
if errorlevel 1 (
    echo [!] No se pudo actualizar .\l4d2_thirdperson.dll - el archivo puede estar en uso
    exit /b 1
)
if not exist "vmconfig.ini" copy /Y "vmconfig.example.ini" "vmconfig.ini" >nul
if not exist "build\vmconfig.ini" copy /Y "vmconfig.ini" "build\vmconfig.ini" >nul
echo [OK] DLL lista: build\l4d2_thirdperson.dll
endlocal
exit /b 0

:load_msvc
where cl.exe >nul 2>&1
if not errorlevel 1 exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [!] MSVC no esta en PATH. Ejecuta desde Developer Command Prompt x86 de Visual Studio.
    exit /b 1
)
set "VSINSTALL="
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if not defined VSINSTALL (
    echo [!] No se encontro MSVC x86. Instala el componente C++ Build Tools.
    exit /b 1
)
call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 exit /b 1
exit /b 0
