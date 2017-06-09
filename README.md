# rpi-fb-matrix
Raspberry Pi framebuffer copy tool for RGB LED matrices.  Show what's on a Pi HDMI output on a big RGB LED matrix chain!
See the full guide with details on installation and configuration at: https://learn.adafruit.com/raspberry-pi-led-matrix-display/overview

## Setup

You **must** clone this repository with the recursive option so that necessary
submodules are also cloned.  Run this command:

    git clone --recursive https://github.com/adafruit/rpi-fb-matrix.git

Next you will need a few dependencies (libconfig++) to compile the code.  Run
these commands to install the dependencies:

    sudo apt-get update
    sudo apt-get install -y build-essential libconfig++-dev

Now if necessary you can change how the RGB LED matrix library is configured
by editing the variables set in the Makefile.  By default the Makefile is
configured to work with the Adafruit LED matrix HAT and a Raspberry Pi 2.  If
you're using a different configuration open the Makefile and edit the `export DEFINES=...`
line at the top.

Build the project by running make:

    make

Once compiled there will be two executables:

*   `rpi-fb-matrix`: The main program that will copy the contents of the primary
    display (HDMI output) to attached LED matrices.
*   `display-test`: A program to display the order and orientation of chained
    together LED matrices.  Good for building complex display chains.

Both executables understand the standard command line flags provided in the
rpi-rgb-led-matrix library, for instance for choosing the gpio mapping.
The default compile-choice gpio mapping is `adafruit-hat`, but you can change
that to any [supported gpio mapping] depending on your set-up.

The [configuration file](./matrix.cfg) allows to describe the geometry and
panel-layout. It overrides geometry-related settings provided as flags
(e.g. `--led-chain`).

You can get a list of available command line options by giving `--led-help`
```
$ ./rpi-fb-matrix --led-help
Usage: ./rpi-fb-matrix [flags] [config-file]
Flags:
        --led-gpio-mapping=<name> : Name of GPIO mapping used. Default "adafruit-hat"
        --led-rows=<rows>         : Panel rows. 8, 16, 32 or 64. (Default: 32).
        --led-chain=<chained>     : Number of daisy-chained panels. (Default: 1).
        --led-parallel=<parallel> : For A/B+ models or RPi2,3b: parallel chains. range=1..3 (Default: 1).
        --led-pwm-bits=<1..11>    : PWM bits (Default: 11).
        --led-brightness=<percent>: Brightness in percent (Default: 100).
        --led-scan-mode=<0..1>    : 0 = progressive; 1 = interlaced (Default: 0).
        --led-show-refresh        : Show refresh rate.
        --led-inverse             : Switch if your matrix has inverse colors on.
        --led-rgb-sequence        : Switch if your matrix has led colors swapped (Default: "RGB")
        --led-pwm-lsb-nanoseconds : PWM Nanoseconds for LSB (Default: 130)
        --led-no-hardware-pulse   : Don't use hardware pin-pulse generation.
        --led-slowdown-gpio=<0..2>: Slowdown GPIO. Needed for faster Pis and/or slower panels (Default: 1).
        --led-daemon              : Make the process run in the background as daemon.
```

## Acknowledgments

This program makes use of the following excellent libraries:

*   [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix)
*   [libconfig](http://www.hyperrealm.com/libconfig/)

Framebuffer capture code based on information from the [rpi-fbcp](https://github.com/tasanakorn/rpi-fbcp) tool.

## License

Released under the GPL v2.0 license, see LICENSE.txt for details.

[supported gpio mapping]: https://github.com/hzeller/rpi-rgb-led-matrix/blob/master/lib/Makefile#L19
