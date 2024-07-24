# Bad Apple !!gs

This project is based on [A2Pico](https://github.com/oliverschmidt/a2pico).

The single purpose of this firmware is to play the [Bad Apple!!](https://en.wikipedia.org/wiki/Bad_Apple!!) music video on an Apple IIgs using:
* 320×200 pixels in fill-mode at 30 fps
* Stereo audio at 26.32 kHz

Notes:
* This is just a silly little project that shows how to have fun with the A2Pico.
* The use of an Apple IIgs stereo audio interface is strongly recommended!
* The mono speaker/headphone volume adheres to the Control Panel setting at playback start.

Usage:
* Flash [Bad-Apple-IIgs.uf2](https://github.com/oliverschmidt/bad-apple-iigs/releases/latest/download/Bad-Apple-IIgs.uf2) to the A2Pico.
* Copy [Bad-Apple-IIgs.code](https://github.com/oliverschmidt/bad-apple-iigs/releases/latest/download/Bad-Apple-IIgs.code) to the root directory of an SD card.
* Insert the SD card into the A2Pico.
* Insert the A2Pico into an Apple IIgs.
* Get to the BASIC prompt.
* Start the playback from the BASIC prompt via `PR#n`.
* Return to the BASIC prompt via `Ctrl-Reset`.

Demo:

* [Bad Apple!! On The Apple IIgs](https://youtu.be/CnemTrIuyy0)

Building the code generator:

* `gcc -O gen-code.c -lpng -lz`
