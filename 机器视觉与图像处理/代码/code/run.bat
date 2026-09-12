@echo off
echo ===================================================
echo    Underwater Dam Crack Detection System
echo ===================================================
echo.
call conda activate ./dam_env
:: Start application
echo Starting application...
python app.py

pause