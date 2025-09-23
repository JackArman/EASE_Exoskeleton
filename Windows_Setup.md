
# Espressif VSCode SetUp Guide (Windows)
## Prerequisites
* VSCode setup with WSL2 with Ubuntu set as the default distribution
* Download [Docker Desktop for Windows](https://docs.docker.com/desktop/setup/install/windows-install/)
* Install VSCode's **Remote - WSL**, **Remote Development** and **Espressif IDF** Extensions
* git pull the latest updates (ensure you have the .devcontainer.json and Dockerfile in the repository)

## Steps

Open **PowerShell as Admin** and run:

```powershell
winget install usbipd
```
Then connect the esp32 via USB cable and run the following command to get a list of all USB serial devices
```powershell
usbipd list
```
Identifying which one is the ESP32 (Should start with Silicon labs CP210x ...), run the following commands with the particular busid tied to this connection. Note for the second command, a WSL terminal must be active.

```powershell
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```
To verify that the USB connection is accessible within VSCode view the output of the following command
```powershell
ls /dev/ttyUSB*
```
Now opening the folder EASE_Exoskeleton folder within VSCode, ensuring that Docker Desktop is running open the command palette with
```powershell
CTRL + SHIFT + P
```
And select the option "Reopen Folder in Container". Note this may take quite a while for the first time.