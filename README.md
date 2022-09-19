# gz-rgb
RGB - control software for my setup using OpenRGB

## Features
- process-watching: settings for certain programs, eg. blue for vim, off for mpv and rainbow for steam
- change modes/colors at runtime through file-based system, which makes it easy to integrate rgb-control into other software (eg define a polybar menu module)
- set the time at which the lights will turn on
- define which rgb devices will be affected by which setting
- run as daemon through systemd


## Configuration
You will have to edit the constants at the top of main.hpp to configure the program to your needs.

## Installation
### Dependecies
- [gz-cpp-util](https://github.com/MatthiasQuintern/gz-cpp-util)

### Arch Linux (ABS)
- Download PKGBUILD: `wget https://raw.github.com/MatthiasQuintern/gz-rgb/main/PKGBUILD`
- Make and install with pacman: `makepkg -si`

### Linux
- Make a *recursive* clone of this repo
- `cd src && make && make install`

### Enable with systemd
- Install OpenRGB and enable `openrgb.service`
- `systemctl daemon-realod && systemctl enable --now gz-rgb.service`

## Usage
Once started, the program will wait until the set time is reached and then activate the process watching. 
You can also send a command to skip the waiting period or to set a custom color.
### Sending commands 
To send a command, create a file in `FILE_COMMAND_DIR` (defaults to `/tmp/gzrgb`). 
The program will detect the file and run the command that is predefined in `externalCommandSettingVec`.
To set a custom color, name the file `colorHexRRGGBB where RRGGBB is a hex rgb color code.


## Changelog
### 1.1 - 2022-09-19
- now re-setting colors when resuming from suspend or hibernate
- added exception handling for when OpenRGB server is not running
### 1.0 2022-09-02
- initial version

### Delevopement Begin 2022-08-30

