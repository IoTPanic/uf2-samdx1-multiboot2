# UF2 MultiBoot2 Bootloader for SAMDx1 Devices

**[Initially copied from Adafruit's Implementation of Microsoft's UF2 bootloader](https://github.com/adafruit/uf2-samdx1)**

**Currently WIP**

This bootloader is made to boot the ovule microkernel on Adafruit SAMD51 and SAMD21 devices using a modified version of the GNU multiboot2 standard, with the only differencing being it follows an unofficial ARM extension, and the text section of the binary is not copied into system memory. The second point is done since all supported devices will be using a unified memory model.

The bootloader sits at 0x00000000, and the application space starts at 0x00002000 (SAMD21) or 0x00004000 (SAMD51).

We're going to follow this basic control flow-
1. Entry into boot loader
2. Check for double reset, or a fault indicating we failed to jump last reset, if so, configure clocks and wait for BIN over USB.
3. Read boot image table to discover all modules, if failure, config clocks and await image.
4. Read image tags from all modules within the table, if failure, config clocks and await image.
5. Load modules into memory.
6. Generate OS Information Tags
7. Jump to OS.

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

## Build

### Requirements

* `make` and an Unix environment
* `arm-none-eabi-gcc`
* `openocd`

### Build commands

The default board is `metro-m4-airlift`. You can build a different one using:

```
make BOARD=metro_m0
```

```
make
```

## License

See THIRD-PARTY-NOTICES.txt for the original SAM-BA bootloader license from Atmel.

The new code is licensed under MIT.
