# melonDS DS Test Suite

melonDS DS features a suite of regression tests
that reduce the chance of changes breaking things.

This document describes how to use the test suite.

# Usage

## Prerequisites

Before you can run the test suite,
you must be able to build melonDS
through the steps described in the [main README](../README.md#building).

## Extra Dependencies

Once you do that, you'll need to obtain the following dependencies:

- [RetroArch][retroarch]
- [Emutest][emutest]
- [Go][go], to install Emutest (since it doesn't offer prebuilt binaries)
- [Python 3][python]
- A Nintendo DS ROM
- The set of Nintendo DS/DSi system files described in the [main README](../README.md#installing-nintendo-ds-bios)

## Configuring the Tests

Once you have all of the above,
you'll need to run `cmake` on the project
with the following variables defined on the command line:

- `BUILD_TESTING`: Set to `ON` to enable the test suite.
- `ARM7_BIOS`: Set to the location of your NDS ARM7 BIOS image.
- `ARM9_BIOS`: Set to the location of your NDS ARM9 BIOS image.
- `ARM7_DSI_BIOS`: Set to the location of your DSi ARM7 BIOS image.
- `ARM9_DSI_BIOS`: Set to the location of your DSi ARM9 BIOS image.
- `NDS_FIRMWARE`: Set to the location of your NDS firmware image.
- `DSI_FIRMWARE`: Set to the location of your DSi firmware image.
- `DSI_NAND`: Set to the location of your DSi NAND image.
- `NDS_ROM`: Set to the location of your NDS ROM image.
- `RETROARCH`: Set to the location of the RetroArch executable.
  Not necessary if it's on your `PATH`.
- `EMU_TEST`: Set to the location of the Emutest executable.
  Not necessary if it's on your `PATH`.

> [!NOTE]
> The test suite will not modify these files;
> they will be copied into appropriate directories before each test.

Here's an example:

```bash

cmake -B build \
    -DBUILD_TESTING=ON \
    -DARM7_BIOS="$SYSTEM_PATH/bios7.bin" \
    -DARM9_BIOS="$SYSTEM_PATH/bios9.bin" \
    -DARM7_DSI_BIOS="$SYSTEM_PATH/dsi_bios7.bin" \
    -DARM9_DSI_BIOS="$SYSTEM_PATH/dsi_bios9.bin" \
    -DNDS_FIRMWARE="$SYSTEM_PATH/firmware.bin" \
    -DDSI_FIRMWARE="$SYSTEM_PATH/dsi_firmware.bin" \
    -DDSI_NAND="$SYSTEM_PATH/dsi_nand.bin" \
    -DNDS_ROM="$ROM_PATH/your_nds_rom.nds" \
    -DRETROARCH="$RETROARCH_PATH/retroarch" \
    -DEMU_TEST="$EMUTEST_PATH/emutest"
```

You may want to put these variables in a script or an IDE configuration.

## Running the Tests

Build melonDS DS [as usual](../README.md#compilation).
After that finishes, call `ctest` from within the CMake build directory
to run the test suite.

```bash
git clone https://github.com/JesseTG/melonds-ds
cd melonds-ds
cmake -B build # Generate the build system
cmake --build build # Build the project
ctest --test-dir build # Run the tests. (CTest is included with CMake)
```

[emutest]: https://github.com/kivutar/emutest
[go]: https://go.dev
[python]: https://www.python.org
[retroarch]: https://www.retroarch.com