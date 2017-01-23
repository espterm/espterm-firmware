# esp-vt100-firmware

ESP8266 Remote Terminal project

This project is based on SpriteTM's esphttpd and libesphttpd, forked by MightyPork to
[MightyPork/esphttpd](https://github.com/MightyPork/esphttpd) and 
[MightyPork/libesphttpd](https://github.com/MightyPork/libesphttpd) respectively.

Those forks include improvements not yet available upstream.

## Goals

This project aims to be a wireless terminal emulator that'll work with the likes of 
Arduino, AVR, PIC, STM8, STM32, mbed etc, anything with UART, even your USB-serial dongle will work.

Connect it to the master device via UART and use the terminal (on your PC or phone) for debug logging, 
remote control etc. It works like a simple LCD screen, in a way.

It lets you make simple UI (manipulating the screen with ANSI sequences) and receive input from buttons on
the webpage (and keyboard on PC). Touch input is a possibility but not currently implemented.

The screen size should be adjustable up to 25x80 (via a special control sequence) and uses 16 standard colors.

## Project status

*Still far from finished and also there's like zero documentation, but you can give it a spin if you wish.*

- Presently we have a working **1-way terminal** (UART->ESP->Browser) with real-time update via websocket.
  The terminal should support multiple sockets at once (not tested).
  
- All ANSi sequences that make sense, as well as control codes like Backspace and CR / LF are implemented.
  Set colors with your usual `\e[31;1m` etc (see Wikipedia). `\e` is the hex code 0x18 or dec 27, or ESC. 
  This may be hard to type on a desktop terminal emulator, if you're trying it with a USB-serial dongle. 
  It'll be easy with the microcontroller, of course.

- The buttons or other input from the browser don't work yet.

- There is also currently no way to set up the WiFi, so it'll use whatever you configured the ESP to
in your previous project. This'll be addressed later.

## Setting it all up

- Install [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk/) and build it with 
`make toolchain esptool libhal STANDALONE=n`. Make sure the `xtensa-lx106-elf/bin` folder is on $PATH.
- Install [esptool](https://github.com/espressif/esptool) (it's in Arch community repo and on AUR, too)
- Set up udev rules so you have access to ttyUSB0 without root, eg:

  ```
  KERNEL=="tty[A-Z]*[0-9]*", GROUP="uucp", MODE="0666"
  ```
- Clone this project with `--recursive`, or afterwards run `git submodule init` and `git submodule update`.

## Development

The web resources are in `html_orig`. To prepare for a build, run `compress_html.sh`. 
This should pack them and put in `html`.

If you're missing the compression tools, you can simply copy them there manually uncompressed and hope for the best.

serial comm is handled in `serial.c`, sockets etc in `user_main.c`.

Get the IoT SDK from one of:

```
ESP8266_NONOS_SDK_V2.0.0_16_08_10.zip:
	wget --content-disposition "http://bbs.espressif.com/download/file.php?id=1690"
ESP8266_NONOS_SDK_V2.0.0_16_07_19.zip:
	wget --content-disposition "http://bbs.espressif.com/download/file.php?id=1613"
ESP8266_NONOS_SDK_V1.5.4_16_05_20.zip:
	wget --content-disposition "http://bbs.espressif.com/download/file.php?id=1469"
ESP8266_NONOS_SDK_V1.5.3_16_04_18.zip:
	wget --content-disposition "http://bbs.espressif.com/download/file.php?id=1361"
ESP8266_NONOS_SDK_V1.5.2_16_01_29.zip:
	wget --content-disposition "http://bbs.espressif.com/download/file.php?id=1079"
```

It's tested with 1.5.2, 2.0.0 won't work without adjusting the build scripts. Any 1.5.x could be fine.

To flash, just run `make flash`. First make sure your `esphttpdconfig.mk` is set up properly - link to sdk etc.
