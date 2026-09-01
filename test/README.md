# melonDS DS Test Suite

melonDS DS features a suite of regression tests
that reduce the chance of changes breaking things.

The tests are written with [pytest][pytest] and driven by [libretro.py][libretro.py],
a libretro frontend built for testing cores.
[pytest-cmake][pytest-cmake] registers each one with [CTest][ctest],
so the whole suite runs as part of the normal build.

This document describes how to use the test suite.

# Usage

## Prerequisites

Before you can run the test suite,
you must be able to build melonDS DS
through the steps described in the [main README](../README.md#building).

## Extra Dependencies

Once you do that, you'll need to obtain the following dependencies:

- [Python][python] 3.12 or later.
- A Nintendo DS ROM, preferably retail.
  The tests don't assume any particular ROM.
- The set of Nintendo DS/DSi system files described in the [main README](../README.md#installing-nintendo-ds-bios)

None of these are strictly required:
tests that need something you don't have will be skipped, not failed.
See [Skipped Tests](#skipped-tests) below.

### Optional: Configuring the Python Environment

By default, the test suite will find Python,
configure its own virtual environment in the build directory,
and install `test/requirements.txt` into it.
You can use your own virtual environment or the system Python installation instead:

```bash
# If you want to use a venv, activate it beforehand.
cmake -B build -DMELONDSDS_INTERNAL_VENV=OFF
```

CMake will still install the test dependencies into whichever environment it finds.
If you'd rather manage them yourself, turn that off too:

```bash
pip install -r test/requirements.txt
cmake -B build -DMELONDSDS_INTERNAL_VENV=OFF -DMELONDSDS_INSTALL_TEST_REQUIREMENTS=OFF
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
- `DSIWARE_ROM`: Set to the location of your DSiWare ROM image
   (_not_ a DSi-enhanced cart ROM).
- `GBA_ROM`: Set to the location of your GBA ROM image.
- `GBA_SRAM`: Set to the location of a save data image for `GBA_ROM`.

Every one of these is optional, but strongly recommended.
CMake will warn about the ones you leave out,
and the tests that need them will be skipped.

> [!NOTE]
> The test suite will not modify these files.
> They will be copied into a temporary directory before each test.

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

You may want to put these variables in a script, an IDE configuration, or a CMake user preset.

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

Test discovery happens when you _build_, not when you configure.
That means a syntax error or a bad import in a test module
will fail the build rather than the test run.

### Running a Subset

Each test case is registered with CTest under a name derived from its location,
like `melonDSDS.test_booting.boots.direct-nds-builtin`,
so you can select them with a regex:

```bash
ctest --test-dir build -R '^melonDSDS\.test_booting\.'  # One module
ctest --test-dir build -R 'boots\.dsi-menu'             # One family of cases
ctest --test-dir build -E 'opengl'                      # Everything but OpenGL
```

You can also run pytest directly, which is quicker while iterating on one test.
It needs the environment variables that CTest normally supplies,
the easiest source of which is a CTest run itself:

```bash
cd test/python
ctest --test-dir ../../build -R host_sensor -VV  # Prints the environment it uses
pytest -k solar_sensor -v
pytest -m "not dsi_sysfiles"                     # Select by prerequisite
```

> [!WARNING]
> Running `pytest` on its own loads and unloads the core repeatedly
> in a single process, which the supported CTest path never does.
> It's fine for iterating on one test, but use `ctest` for the whole suite.

> [!WARNING]
> There are different revisions of the DS and DSi's system files.
> The test suite itself doesn't care which ones you use,
> but certain bugs may only appear with particular firmware revisions.
> If you can't reproduce a bug that should cause a test to fail,
> try a different firmware image.

### Skipped Tests

CTest reports a test as `Skipped` when it needs an asset you didn't configure.
That's not a failure;
it means the test declared a prerequisite that isn't available on your machine.

A wall of skipped tests usually means
you haven't pointed CMake at your ROMs and system files,
or that the core was built without OpenGL.

# How the Suite Is Organized

All of the test code lives in [`test/python`](python).

| Module | Covers |
|---|---|
| `test_lifecycle.py` | Loading, running and unloading the core |
| `test_environment.py` | Environment calls: directories, capabilities, messages, metadata |
| `test_options.py` | Core options: declaration, reading, updating, visibility |
| `test_booting.py` | Every combination of console mode and system files |
| `test_console_mode.py` | Resolving "Auto" console mode against the loaded ROM |
| `test_av.py` | Audio and video output, screen geometry, rotation |
| `test_opengl.py` | Hardware rendering and runtime renderer switching |
| `test_input.py` | Buttons, the touch pointer, the analog cursor, the microphone |
| `test_microphone.py` | When the host microphone is opened and activated |
| `test_state.py` | Savestates and exposed memory regions |
| `test_cheats.py` | Applying, resetting and validating cheats |
| `test_firmware.py` | Firmware and BIOS validation, fallback and preservation |
| `test_slot2.py` | The Memory Expansion Pak, Rumble Pak and Solar Sensor |
| `test_reset.py` | Resetting the emulated console |
| `test_homebrew_sd.py` | The virtual SD card that homebrew ROMs see |

[`conftest.py`](python/conftest.py) holds the shared fixtures,
and [`melondsds/`](python/melondsds) holds the helpers that aren't fixtures.
The suite is configured by [`pytest.toml`](python/pytest.toml),
which uses pytest 9's native TOML format
rather than the older `pytest.ini` or `[tool.pytest.ini_options]`.

## Linting

[Ruff][ruff] handles both linting and formatting,
configured by [`ruff.toml`](python/ruff.toml).
It's neither built nor run by CI,
and it isn't listed in `test/requirements.txt`;
run it by hand if you want it:

```bash
cd test/python
ruff check .
ruff format .
```

## Prerequisites Are Markers

A test declares what it needs with a marker,
and [`conftest.py`](python/conftest.py) skips it at run time if that isn't available.
The full list is in [`pytest.toml`](python/pytest.toml).

```python
@pytest.mark.nds_rom      # Needs -DNDS_ROM=...
@pytest.mark.nds_sysfiles # Needs (and stages) bios7.bin, bios9.bin and firmware.bin
@pytest.mark.opengl       # Needs an OpenGL-enabled core build
```

System file markers do double duty:
a marked file is copied into `<system>/melonDS DS/` before the core is loaded.
Only marked files are staged,
which lets us test cases where required system files are absent.

## Writing a New Test

Most of the time you're adding a `pytest.param` to a table that already exists.

1. Find the module that covers the behavior you're testing.
2. Add a case, or a new `test_*` function if none of them fit.
3. Ask for the `session` fixture, which builds a configured `libretro.Session`:
   ```python
   @pytest.mark.nds_rom
   def test_something(session, nds_rom):
       with session(nds_rom, options={"melonds_console_mode": "ds"}) as emulator:
           emulator.run()
           assert ...
   ```
4. Declare any prerequisites with markers.
5. Rebuild. Discovery picks the new case up automatically.

Some conventions worth knowing:

- **A test expected to fail gets `@pytest.mark.xfail(strict=True)`.**
  This is how the suite expresses "this configuration should not boot".
- **Never gate a test on an environment variable at import time.**
  Test discovery runs without any of them set,
  so a module-level `pytest.mark.skipif` on one would silently drop the test.
  Use a marker, or call `pytest.skip()` from inside the test.
- **Assertions about driver state can go after the `with` block.**
  The `Session` releases the core on exit, but the drivers outlive it.

# Troubleshooting

This section has information about strange issues I've encountered
while developing this core and its test suite.

### Test Collection Fails the Build

If the build fails with `An error occurred during the collection of Python tests`,
then `conftest.py` or one of the test modules failed to import.
Run pytest by hand from `test/python` to see the traceback:

```bash
cd test/python
pytest --collect-only
```

### Failing OpenGL Tests on Windows

The test suite seems to be incompatible with the `opengl32.dll`
shipped with [MSYS2's various Mesa packages](https://packages.msys2.org/base/mingw-w64-mesa).
I'm not sure why this happens, but it should only be a problem
if your MSYS2 environment's `/bin` directory is in your `PATH`;
if you're developing melonDS DS in an environment that does so (e.g. CLion's test runner),
you can work around this by setting the `TEST_EXCLUDE_PATHS` variable
to the offending directory, like so:

```bash
cmake -B build -DTEST_EXCLUDE_PATHS="C:\tools\msys64\ucrt64\bin" # ...other variables...
```

CTest will then ensure that the specified directory
is not included in each test process's library search path.

### CMake Finds the Wrong pytest

`PYTEST_EXECUTABLE` is cached,
so toggling `MELONDSDS_INTERNAL_VENV` between runs can leave a stale value behind.
Clear it and reconfigure:

```bash
cmake -B build -UPYTEST_EXECUTABLE
```

[ctest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html
[libretro.py]: https://github.com/JesseTG/libretro.py
[pytest]: https://docs.pytest.org
[pytest-cmake]: https://python-cmake.github.io/pytest-cmake
[python]: https://www.python.org
[ruff]: https://docs.astral.sh/ruff
