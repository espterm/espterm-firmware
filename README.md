# esp-vt100-firmware

ESP8266 Remote Terminal project

This project is based on SpriteTM's esphttpd and libesphttpd, forked by MightyPork to
[MightyPork/esphttpd](https://github.com/MightyPork/esphttpd) and 
[MightyPork/libesphttpd](https://github.com/MightyPork/libesphttpd) respectively.

Those forks include improvements not yet available upstream.

## Goals

This project aims to be a wireless terminal emulator that'll work with the likes of 
Arduino, AVR, PIC, STM8, STM32, mbed etc, anything with UART, even your USB-serial dongle will work.

Connect it to the master device via UART and use the terminal on the built-in web page for debug logging, 
remote control etc. It works like a simple LCD screen, in a way.

It lets you make simple UI (manipulating the screen with ANSI sequences) and receive input from buttons on
the webpage (and keyboard on PC). Touch input is planned as well.

The screen size is adjustable up to 25x80 (via a special control sequence) and uses 16 standard colors 
(8 dark and 8 bright).

## Project status

*Almost finished, still possibly buggy, but it looks promising. Most of the features are there now.*

- We have a working **2-way terminal** (UART->ESP->Browser and vice versa) with real-time update via websocket.
  
  This means that what you type in the browser is sent to UART0 and what's received on UART0 is processed by the 
  ANSI parser and applied to the internal screen buffer. You'll also immediately see the changes in your browser. 
  There's a filter in the way that discards garbage characters (like unicode and most ASCII outside 32-126).
  
  For a quick test, try connecting the UART0 Rx and Tx with a piece of wire to make a loopback interface. 
  *NOTE: Use the bare module, not something like LoLin or NodeMCU with a FTDI, it'll interfere*. 
  You should then directly see what you type & can even try some ANSI sequences, right from the browser.
  
- All ANSI sequences that make sense, as well as control codes like Backspace and CR / LF are implemented.
  Set colors with your usual `\e[31;1m` etc (see Wikipedia). `\e` is the ASCII code 27 (ESC).
  
  Arrow keys generate ANSI sequences, ESC sends literal ASCII code 27 etc. Almost everything can be input 
  straight from the browser.
  
- To resize the screen, send `\e]W<rows>;<cols>\a` (it's an OSC code, terminated by ST).

- Buttons pressed in the browser UI send ASCII codes 1..5. Mouse clicks also send events to the server, 
  but currently don't generate any output in the terminal, I still hadn't decided on the best encoding.

- By tapping the header on the terminal page, you'll open the WiFi config page. It's in essence the 
  esphttpd's wifi config page, but re-styled and much improved. You can set AP SSID, channel, see the IP
  address etc right there.

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
