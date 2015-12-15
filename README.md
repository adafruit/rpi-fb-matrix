# rpi-fb-matrix
Raspberry Pi framebuffer copy tool for RGB LED matrices.  Show what's on a Pi HDMI output on a big RGB LED matrix chain!

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

*   rpi-fb-matrix: The main program that will copy the contents of the primary
    display (HDMI output) to attached LED matrices.
*   display-test: A program to display the order and orientation of chained
    together LED matrices.  Good for building complex display chains.

## Acknowledgments

This program makes use of the following excellent libraries:

*   [rpi-rgb-led-matrix](https://github.com/hzeller/rpi-rgb-led-matrix)
*   [libconfig](http://www.hyperrealm.com/libconfig/)

Framebuffer capture code based on information from the [rpi-fbcp](https://github.com/tasanakorn/rpi-fbcp) tool.

## License

Released under the GPL v2.0 license, see LICENSE.txt for details.
