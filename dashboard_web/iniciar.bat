@echo off
echo ============================================
echo    BIORREACTOR - Dashboard Web
echo ============================================
echo.

:: Mostrar la IP para compartir
echo Tu IP para compartir:
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /c:"IPv4"') do (
    echo    http://%%a:5000
)
echo.
echo Abre esa direccion desde cualquier dispositivo
echo en la misma red WiFi.
echo.
echo Presiona Ctrl+C para detener.
echo ============================================
echo.

python app.py
pause