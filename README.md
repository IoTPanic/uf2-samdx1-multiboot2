# UF2 MultiBoot2 Bootloader for SAMDx1 Devices

**[Initially copied from Adafruit's Implementation of Microsoft's UF2 bootloader](https://github.com/adafruit/uf2-samdx1)**

This bootloader is made to boot the ovule microkernel on Adafruit SAMD51 and SAMD21 devices using a modified version of the GNU multiboot2 standard, with the only differencing being it follows an unofficial ARM extension, and the text section of the binary is not copied into system memory. The second point is done since all supported devices will be using a unified memory model.

## UF2

**UF2 (USB Flashing Format)** is a name of a file format, developed by Microsoft, that is particularly
suitable for flashing devices over MSC devices. The file consists
of 512 byte blocks, each of which is self-contained and independent
of others.

Each 512 byte block consist of (see `uf2format.h` for details):
* magic numbers at the beginning and at the end
* address where the data should be flashed
* size of data
* data (up to 476 bytes; for SAMD it's 256 bytes so it's easy to flash in one go)

Thus, it's really easy for the microcontroller to recognize a block of
a UF2 file is written and immediately write it to flash.

* **UF2 specification repo:** https://github.com/Microsoft/uf2
* [#DeskOfLadyada UF24U ! LIVE @adafruit #adafruit #programming](https://youtu.be/WxCuB6jxLs0)

## Build

### Requirements

* `make` and an Unix environment
* `node`.js in path (optional)
* `arm-none-eabi-gcc` in the path (the one coming with Yotta will do just fine). You can get the latest version from ARM: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
* `openocd` - you can use the one coming with Arduino (after your install the M0 board support)

Atmel Studio is not supported.

You will need a board with `openocd` support.

Arduino Zero (or M0 Pro) will work just fine as it has an integrated USB EDBG
port. You need to connect both USB ports to your machine to debug - one is for
flashing and getting logs, the other is for the exposed MSC interface.

Otherwise, you can use other SAMD21 board and an external `openocd` compatible
debugger. IBDAP is cheap and seems to work just fine. Another option is to use
Raspberry Pi and native bit-banging.

`openocd` will flash 16k, meaning that on SAMD21 the beginning of user program (if any) will
be overwritten with `0xff`. This also means that after fresh flashing of bootloader
no double-tap reset is necessary, as the bootloader will not try to start application
at `0xffffffff`.

### Build commands

The default board is `metro-m4-airlift`. You can build a different one using:

```
make BOARD=metro_m0
```

If you're working on different board, it's best to create `Makefile.user`
with say `BOARD=metro` to change the default.
The names `zero` and `metro` refer to subdirectories of `boards/`.

There are various targets:
* `all` - just compile the board
* `burn` or `b` - compile and deploy to the board using openocd
* `logs` or `l` - shows logs
* `run` or `r` - burn, wait, and show logs

Typically, you will do:

```
make r
```

### Configuration

There is a number of configuration parameters at the top of `uf2.h` file.
Adjust them to your liking.

By default, you cannot enable all the features, as the bootloader would exceed
the 8k(SAMD21)/16k(SAMD51) allocated to it by Arduino etc. It will assert on startup that it's not bigger
than 8k(SAMD21)/16k(SAMD51). Also, the linker script will not allow it.

Three typical configurations are:

* HID, WebUSB, MSC, plus flash reading via FAT; UART and CDC disabled;
  logging optional; **recommended**
* USB CDC and MSC, plus flash reading via FAT; UART disabled;
  logging optional; this may have Windows driver problems
* USB CDC and MSC, no flash reading via FAT (or at least `index.htm` disabled); UART enabled;
  logging disabled; no handover; no HID;
  only this one if you need the UART support in bootloader for whatever reason

CDC and MSC together will work on Linux and Mac with no drivers.
On Windows, if you have drivers installed for the USB ID chosen,
then CDC might work and MSC will not work;
otherwise, if you have no drivers, MSC will work, and CDC will work on Windows 10 only.
Thus, it's best to set the USB ID to one for which there are no drivers.

The bootloader sits at 0x00000000, and the application starts at 0x00002000 (SAMD21) or 0x00004000 (SAMD51).

## License

See THIRD-PARTY-NOTICES.txt for the original SAM-BA bootloader license from Atmel.

The new code is licensed under MIT.
