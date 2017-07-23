# ESPTerm

ESP8266 Wireless Terminal Emulator project

This project is based on SpriteTM's esphttpd and libesphttpd, forked by MightyPork to
[MightyPork/esphttpd](https://github.com/MightyPork/esphttpd) and 
[MightyPork/libesphttpd](https://github.com/MightyPork/libesphttpd) respectively.

Those forks include improvements not available upstream.

## Goals

The project aims to be a wireless terminal emulator that'll work with the likes of 
Arduino, AVR, PIC, STM8, STM32, mbed etc, anything with UART, even your USB-serial dongle will work.

Connect it to the master device via UART and use the terminal on the built-in web page for debug logging, 
remote control etc. It works like a simple LCD screen, in a way.

It lets you make simple UI (manipulating the screen with ANSI sequences) and receive input from buttons on
the webpage (and keyboard on PC). There is some rudimentary touch input as well.

The screen size is adjustable up to 25x80 (via a special control sequence) and uses 16 standard colors 
(8 dark and 8 bright). Both default size and colors can be configured in the settings.

## Project status

*A little buggy, but mostly okay! There are many features and fixes planned, but it should be fairly usable already.*

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

- Buttons pressed in the browser UI send ASCII codes 1..5. Mouse clicks are sent as `\e[<row>;<col>M`.

- There's a built-in WiFi config page and a Help page with a list of all supported ANSI sequences and other details.

## Development

### Installation for development

- Clone this project with `--recursive`, or afterwards run `git submodule init` and `git submodule update`.

- Install [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk/) and build it with 
  `make toolchain esptool libhal STANDALONE=n`. 
  
  Make sure the `xtensa-lx106-elf/bin` folder is on $PATH.

- Install [esptool](https://github.com/espressif/esptool) (it's in the Arch community repo and on AUR, too)

- Set up udev rules so you have access to ttyUSB0 without root, eg:

  ```
  KERNEL=="tty[A-Z]*[0-9]*", GROUP="uucp", MODE="0666"
  ```

- Install Ragel if you wish to make modifications to the ANSI sequence parser. 
  If not, comment out its call in `build_parser.sh`. The `.rl` file is the actual source, the `.c` is generated.

- Install Ruby and then the `sass` package with `gem install sass` (or try some other implementation, such as 
  `sassc`)

- Make sure your `esphttpdconfig.mk` is set up properly - link to the SDK etc.

The IoT SDK is now included in the project due to problems with obtaining the correct version and patching it.
It works with version 1.5.2, any newer seems to be incompatible. If you get it working with a newer SDK, a PR is more
than welcome!

### Web resources

The web resources are in `html_orig`. To prepare for a build, run `build_web.sh`, which packs them and 
copies over to `html`. The compression and minification is handled by scripts in libesphttpd, specifically
it runs yuicompressor on js and css and gzip or heatshrink on the other files. The `html` folder is 
then embedded in the firmware image.

It's kind of tricky to develop the web resources locally; you might want to try the "split image" 
Makefile option, then you can flash just the html portion with `make htmlflash`. I haven't tried this.

For local development, use the `server.sh` script in `html_orig`. It's possible to talk to API of a running 
ESP8266 from here, if you configure `_env.php` with its IP.

### Flashing

The Makefile should automatically build the parser and web resources for you when you run `make`. 
Sometimes it does not, particularly with `make -B`. Try just plain `make`. You can always run those 
build scripts manually, too.

To flash, just run `make flash`. 
