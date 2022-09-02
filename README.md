# Glowzwiebel RGB
RGB - Control software for my setup using OpenRGB

## Features
- define which rgb devices will be affected by which setting
- process-watch mode: settings for certain programs, eg. blue for vim, off for mpv and rainbow for steam
- run as daemon through systemd
- change modes/colors at runtime by placing files, which makes it easy to integrate rgb-control into other software (eg define a polybar menu module)
- set the time at which the lights will turn on

## Configuration
You will have to edit the constants at the top of main.hpp to configure the program to your needs.

## Installation
### Arch Linux (ABS)
- Download PKGBUILD: `wget https://raw.github.com/MatthiasQuintern/gz-rgb/main/PKGBUILD`
- Make and install with pacman: `makepkg -si`

### Linux
- Make a *recursive* clone of this repo
- `cd src && make && make install`

## Enable with systemd
- Install OpenRGB and enable `openrgb.service`
- `systemctl daemon-realod && systemctl enable --now gz-rgb.service`


## Changelog
### 2022-09-02
- initial version

### Delevopement Begin 2022-08-30

