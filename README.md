# ESPTerm

ESPTerm is a terminal emulator running on the ESP8266 WiFi chip.
The firmware emulates VT102 with some additional features based on `xterm` and later VT models, and the terminal screen can be accessed using any web browser, even on a phone or tablet. It works with ESP-01, ESP-01S and ESP-12 and likely many other modules.

ESPTerm can be used to add remote access via WiFi to any embeded project, all you need is a serial port.

## Screenshots, photos

- Look at the [GitHub releases page][releases], there are some pics.

## ESPTerm features

- **Robust WiFi configuration interface**
  - static IP
  - DHCP
  - AP channel and strength setting
  - AP password
  - SSID search for finding your existing network
- **Almost complete VT102 emulation** with some extras
  - *Sufficient to run most Linux console applications, including ncurses-based ones\**
  - All standard SGR (text style attributes) supported
  - Full UTF-8 support
  - Alternate character sets support
  - 16 colors
  - Scrolling Region, Origin Mode
  - Tab Stops
  - Cursor save/restore
  - Character/Line insert, delete, clear operations
  - Audible BEL (ASCII 7 beep)
  - Most standard queries (get cursor position, get SGR, get screen size...)
  - All DEC private options either implemented or safely consumed by the parser
- **Other features:**
  - Screen size up to 80x25
  - Real-time screen update via WebSocket
  - Button to open Android software keyboard
  - 5 buttons under the screen for quick commands
  - Button labels can be changed (by OSC commands or via settings)
  - Screen title can be changed (by OSC commands or via settings)
  - Built-in help page with basic troubleshooting and command reference

\*) Linux console applications run via agetty at ttyUSB0 were used for testing, obviously you'll be better off using SSH for remote shell

ESPTerm firmware is written in C and is based on SpriteTM's `libesphttpd` http server library, forked by MightyPork to
[MightyPork/libesphttpd](https://github.com/MightyPork/libesphttpd) respectively. This fork includes various improvements and changes required by the project.

## Running ESPTerm

To run ESPTerm on your ESP8266, either build it yourself from source using `xtensa-lx106-elf-gcc` (and the included Makefile), or download pre-built binaries from the [GitHub releases section][releases]. Flash the binaries using [esptool](https://github.com/espressif/esptool).

### Pins

- Pin GPIO2 is used for debug messages at 115200 baud, 8 bit, no parity.
- Pins Rx and Tx are used for the main communication UART. Connect your USB-serial dongle or application microcontroller here.

### Console commands - escape sequences

- A list of most supported commands can be found here: http://bits.ondrovo.com/espterm-xterm.html
- To set the screen title, use `OSC 0 ; title ST` (`OSC` = `ESC ]`, `ST` = `ESC \` or ASCII 7)
- To set buttons text, use `OSC 8 1 ; text ST` with 81 through 85.
- Mouse clicks (as of v0.6.9) generate `CSI row ; col M` at the Tx pin (`CSI` = `ESC [`)
- For more info, look eg. on Wikipedia or here: http://ascii-table.com/ansi-escape-sequences-vt-100.php

### Setup

- When flashed for the first time, ESPTerm wipes any possible previous WiFi configuration, because it implements its own WiFi config manager with many additional features. 
- It should start in AP mode, the default SSID being `TERM-MACADR` with `MACADR` being three unique bytes from the MAC address / Device ID.
- Connect to it via a smartphone or laptop and configure WiFi as desired.
- To re-enable the built-in AP, hold the BOOT (GPIO0) button for about 1 s, until the blue LED starts slowly flashing. Then release the button!
- To reset all settings to defaults, hold the BOOT (GPIO0) button until the blue LED flashes rapidly, then release the button.
- When you accidentally make the blue LED flash, you can cancel the operation by pressing Reset or disconnecting its power supply. The wipe is done on the button's release.

### Config files

ESPTerm has two config "files", one for defaults and one for the currently used settings. In the case of the terminal config, there is also a third, temporary file for changes done via ESC commands.

When you get your settings *just right*, you can store them as defaults, which can then be at any time restored by holding the BOOT (GPIO0) button. You can do this on the System Settings page. This asks for an "admin password", which you can define when building the firmware in the `esphttpdconfig.mk` file. The default password is `19738426`. This password can't presently be changed without re-flashing the firmware.

You can also restore everything (except the saved defaults) to "factory defaults", there is a button for this on the System Settings page. Those are the initial values in the config files.

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
copies over to `html`. The compression and minification is handled by scripts in libesphttpd, specifically,
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

[releases]: https://github.com/MightyPork/esp-vt100-firmware/releases
