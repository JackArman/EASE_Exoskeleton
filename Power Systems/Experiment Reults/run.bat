@echo off
REM ==========================================================
REM launch_scripts_parallel.bat
REM Runs serial_in.py and owon_logger.py at the same time
REM Each opens in its own Command Prompt window and closes when stopped
REM ==========================================================

REM Change directory to where this batch file is located
cd /d "C:\Users\india\Downloads\thesis\EASE_Exoskeleton\Power Systems\Experiment Reults"

REM Optional: activate your Python environment first
REM call venv\Scripts\activate

echo Launching serial_in.py and owon_logger.py in parallel...

REM /c = run command then close window when script ends or Ctrl+C pressed
start "Serial Input" cmd /c python serial_in.py
start "OWON Logger" cmd /c python owon_logger.py

echo.
echo Both scripts launched in separate Command Prompt windows.
echo They will close automatically when finished or on CTRL+C.
pause
