# melonDS DS Test Suite

melonDS DS features a suite of regression tests
that reduce the chance of changes breaking things.

This document describes how to use the test suite.

# Usage

## Prerequisites

Before you can run the test suite,
you must be able to build melonDS DS
through the steps described in the [main README](../README.md#building).

## Extra Dependencies

Once you do that, you'll need to obtain the following dependencies:

- [Python][python] 3.11 or later.
- A Nintendo DS ROM, preferably retail.
  The tests don't assume any particular ROM.
- The set of Nintendo DS/DSi system files described in the [main README](../README.md#installing-nintendo-ds-bios)

### Optional: Configuring the Python Environment

By default, CMake finds Python and configures its own internal Python environment in the build directory.
You can use your own virtual environment or the system Python installation with the following steps:

```bash
# If you want to use a venv, activate it beforehand.
pip install -r test/requirements.txt # Install the test framework's dependencies
cmake -B build -DMELONDSDS_INTERNAL_VENV=OFF
```

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

> [!NOTE]
> The test suite will not modify these files;
> they will be copied into a temporary directory before each test.

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
    -DNDS_ROM="$ROM_PATH/your_nds_rom.nds"
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

> [!WARNING]
> There are different revisions of the DS and DSi's system files.
> The test suite itself doesn't care which ones you use,
> but certain bugs may only appear with particular firmware revisions.
> If you can't reproduce a bug that should cause a test to fail,
> try a different firmware image.

[python]: https://www.python.org