# melonDS DS

An enhanced remake of the [melonDS][melonds] [core][melonds-libretro] for [libretro][libretro]
that prioritizes standalone parity, reliability, and usability.

![melonDS DS Latest Release](https://img.shields.io/github/v/release/JesseTG/melonds-ds?label=melonDS%20DS&link=https%3A%2F%2Fgithub.com%2FJesseTG%2Fmelonds-ds%2Freleases%2Flatest)
![GitHub Workflow Status (with event)](https://img.shields.io/github/actions/workflow/status/JesseTG/melonds-ds/main.yaml?label=Build%20Status)

# Getting melonDS DS

You can get the latest stable release of melonDS DS
from [this repo's Releases](https://github.com/JesseTG/melonds-ds/releases/latest),
or the raw build artifacts from [here][workflows].
In the near future you'll be able to get it from the official RetroArch build server,
or even from within RetroArch itself.

# Installation

Installation instructions may vary depending on your chosen libretro frontend.

## RetroArch

Here's how you can install melonDS DS in [RetroArch][retroarch]:

1. Place `melondsds_libretro.dll` (or `.so` or `.dylib`, depending on the platform) in RetroArch's `cores` directory.
2. Place `melondsds_libretro.info` in the same directory as the other `.info` files,
   which is usually `cores` or `info` depending on the platform.

# Using melonDS DS

Usage instructions may vary depending on your chosen libretro frontend.

## RetroArch

### Playing Nintendo DS Games

1. Start RetroArch.
2. Scan your Nintendo DS game library with the <kbd>Import Content</kbd> menu 
   to build a playlist if you haven't already.
3. Load a Nintendo DS game from the playlist.
   If you have the legacy melonDS core installed,
   you may need to select melonDS DS explicitly.

> [!IMPORTANT]
> If you have ROM hacks or homebrew,
> you may need to manually add them to the playlist
> with the <kbd>Manual Scan</kbd> submenu.

### Installing Nintendo DS BIOS

melonDS includes built-in BIOS and firmware replacements that work with most games.
However, additional system files may be required
for certain features or games to work properly.

You can place your system files in RetroArch's `system` directory
or in a subdirectory named `melonDS DS`.
Name the system files as follows:

- DS ARM7 BIOS: `bios7.bin`\*
- DS ARM9 BIOS: `bios9.bin`\*
- DS Firmware: Anything, pick an image in the core options
- DSi ARM7 BIOS: `dsi_bios7.bin`\*
- DSi ARM9 BIOS: `dsi_bios9.bin`\*
- DSi Firmware: Anything, pick an image in the core options\*
- DSi System NAND: Anything, pick an image in the core options\*

<small>*Required for DSi mode.</small>

### Game Boy Advance Connectivity

The steps for loading a Game Boy Advance ROM are a little more involved.
Native BIOS files are not required.

1. Load the melonDS DS core using the <kbd>Load Core</kbd> menu.
2. Enter the <kbd>Subsystems</kbd> menu and select <kbd>Load Slot 1 & 2 Boot</kbd>.
3. Select a Nintendo DS ROM, a Game Boy Advance ROM, and optionally a Game Boy Advance save file (in that order).
4. Start the game.

This combination of ROMs will appear in your History playlist,
so you won't have to repeat this process every time you want to play.

> [!NOTE]
> melonDS can load Game Boy Advance ROMs and save data
> for the purpose of Slot-2 connectivity,
> but it cannot actually play GBA games.
> Use a GBA core instead.

# New Features

Enhancements over the [legacy melonDS core][melonds-libretro] include:

## Standalone Parity

Unlike most other libretro cores,
melonDS DS is not a fork of an existing code base.
It uses standalone melonDS as a statically-linked dependency,
which means that large changes and merge conflicts are less of an issue.
As a result, improvements to standalone melonDS
are much easier to integrate!

## Wi-Fi Support

Wi-Fi is fully emulated on all platforms!

For your convenience, you can choose from
one of several preconfigured servers in the core options menu,
with [Kaeru WFC][kaeru] being the default.

If there's another server you'd like to use,
you can set its DNS address
from within the emulated console's Wi-Fi settings menu.

> [!NOTE]
> Do not confuse this with local multiplayer.
> melonDS DS does not support emulating local wireless
> at this time.

## Homebrew Save Data

The legacy core does not support save data for homebrew games.
However, melonDS DS does!

melonDS DS looks in the `system/melonDS DS` directory (i.e. alongside the BIOS files)
for a homebrew SD card image named `dldi_sd_card.bin`.
If one doesn't exist, a virtual 4GB SD card will be created if necessary.
See the core options for more information.

> [!NOTE]
> melonDS DS does not support savestates for homebrew games.

## Microphone Support

melonDS DS supports libretro's new microphone API,
allowing you to use your device's microphone for Nintendo DS games!

> [!NOTE]
> This feature requires support from the frontend.
> The latest stable release of RetroArch
> includes microphone support on several platforms.

## Screen Rotation

melonDS DS fully supports rotating the emulated DS left, right, and upside-down!
Now you can play games that were meant to be played sideways,
such as Brain Age.

## Enhanced Screen Layout Options

The legacy melonDS core supports multiple screen layouts,
but they can only be switched out through the core options menu.
This is inconvenient for games that use different layouts.

melonDS DS allows you to cycle through up to 8 screen layouts ([including rotations](#screen-rotation))
at the push of a button!

## Streamlined DSiWare Installation

melonDS does not support direct-booting DSiWare at this time;
you need to install DSiWare games to a NAND image,
then start them from the DSi menu when you want to play.

But melonDS DS streamlines this process!

- When you select a DSiWare ROM,
  it's temporarily installed on the NAND
  and removed when you exit the core.
- Title metadata is automatically downloaded from Nintendo's servers
  and cached locally for later.
- When a DSi game is loaded,
  the DSi menu will boot with the temp-installed game selected.

## Other Niceties

- **Battery Support:**
  melonDS DS uses libretro's new power state API
  to reflect your device's power status (battery level, charging status, etc.)
  in the emulated console!
  Requires support by the frontend.
- **Selectable NAND and Firmware Images:**
  You don't need to hard-code a specific name for your firmware or NAND images!
  Just place them in `system/melonDS DS` (or local equivalent)
  and pick them from the core options.

# Missing Features

These features have not yet been implemented in standalone [melonDS][melonds],
or they haven't been integrated into melonDS DS.
If you want to see them, ask how you can get involved!

- **Local Wireless:**
  Upstream melonDS supports emulating local wireless multiplayer
  (e.g. Multi-Card Play, Download Play) with multiple instances of melonDS on the same computer.
  However, melonDS DS does not support this functionality at this time.
- **Homebrew Savestates:**
  melonDS has limited support for taking savestates of homebrew games,
  as the virtual SD card is not included in savestate data.
- **DSi Savestates:**
  Nintendo DSi mode does not support savestates.
  This implies that rewinding and runahead are not supported in DSi mode.
- **DSi Direct Boot:**
  Direct Boot does not support DSiWare games at this time.
  They must be installed on a NAND image,
  and they must be started from the DSi menu.
- **Game Boy Advance Emulation:**
  melonDS can load Game Boy Advance ROMs and save data
  for use by compatible Nintendo DS games,
  but it cannot actually emulate the GBA.
  GBA emulation is not within the scope of melonDS;
  use a GBA emulator instead.
- **Threaded Software Renderer:**
  melonDS supports offloading its software renderer to a separate thread,
  but rewinding while using it [can crash RetroArch](https://github.com/JesseTG/melonds-ds/issues/81).
  This was holding up the initial release,
  so the threaded renderer is disabled until this bug is fixed.
- **Slot-2 Accessories:**
  The only Slot-2 accessories that melonDS currently supports
  are the [solar sensor][solar-sensor] and [Memory Expansion Pak][memory-pak].
  However, melonDS DS does not yet support these devices.
- **GDB Stub Support:**
  melonDS recently gained support
  for debugging emulated DS games with GDB.
  This is a low priority for melonDS DS,
  since libretro frontends are typically used for playing games.
  However, I may integrate it if there's enough demand.

# Compatibility

## Games

melonDS DS is compatible with all games that melonDS supports,
unless otherwise noted in the [Missing Features section](#missing-features).
If this is not the case, please [report it][issue-tracker].

## Frontends

melonDS DS primarily targets RetroArch,
but you can use it with most libretro frontends.
If you encounter problems using this core with other frontends,
please [report them][issue-tracker]!
Support is not guaranteed, but I'll do the best I can.

## Platforms

melonDS DS will run on the following platforms,
assuming it's used with a frontend that also supports them:

- Windows (x86_64)
- macOS (x86_64 and arm64)
- Linux (x86_64 and arm64)
- Android (arm64)
- iOS (arm64)

Available features may vary depending on the platform and frontend.

The legacy melonDS core has builds for the Nintendo Switch
and for 32-bit versions of the above platforms,
_but melonDS DS will not support these platforms unless there's enough demand_.

# Building

melonDS DS is built with CMake.

## Dependencies

You will need to install the following beforehand:

- CMake 3.19 or later
- Git
- A C++20 compiler (MSVC is not supported)

Most other dependencies are fetched automatically by CMake.

### Windows

1. Install [MSYS2](https://www.msys2.org).
2. Open the <kbd>MSYS2 MinGW 64-bit</kbd> terminal from the Start Menu.
3. Install dependencies like so:

   ```sh
   pacman -Syu # update the package database
   pacman -S git mingw-w64-x86_64-{cmake,toolchain} # install dependencies
   ```
4. Proceed to [Compilation](#compilation).
   You may need to remain in the MSYS2 terminal.

### macOS

1. Install [Homebrew](https://brew.sh).
2. Install dependencies like so:

   ```sh
   brew install cmake git pkg-config cmake
   ```
3. Install Xcode and the Xcode command-line tools.
4. Proceed to [Compilation](#compilation).

> [!NOTE]
> macOS builds exclude OpenGL by default,
> as the OpenGL renderer [doesn't currently work on the platform](https://github.com/JesseTG/melonds-ds/issues/12).
> To enable it anyway, pass `-DENABLE_OPENGL=ON` to CMake.

### Linux

1. Install dependencies like so:

   ```sh
   sudo apt install cmake git pkg-config # Ubuntu/Debian
   sudo pacman -S base-devel cmake extra-cmake-modules git # Arch Linux
   ```
2. Proceed to [Compilation](#compilation).

### Android

1. Install the Android SDK and [NDK](https://developer.android.com/ndk/downloads).
   The simplest way to do this is through [Android Studio](https://developer.android.com/studio).
2. Proceed to [Compilation](#compilation).

### iOS

These steps can only be done on macOS.

1. Install Xcode and the Xcode command-line tools.
2. Proceed to [Compilation](#compilation).

## Compilation

Once you've installed the dependencies,
the process for building melonDS DS is mostly the same on all platforms:

```sh
git clone https://github.com/JesseTG/melonds-ds
cd melonds-ds
cmake -B build # Generate the build system, and add any -D or --toolchain flags here
cmake --build build # Build the project
```

However, some platforms or features need you to add some extra flags to the first `cmake` command:

### Android

You'll need to add the following flags to build for Android.

- `--toolchain=...`:
  The path to the `android.toolchain.cmake` file in your NDK installation.
  The location varies depending on how you installed the NDK;
  it will most likely be in `$ANDROID_NDK/build/cmake`.
- `-DANDROID_ABI=...`:
  The ABI to build for.
  This should be `arm64-v8a` or `x86_64`.
  If in doubt, use `arm64-v8a`.
- `-DANDROID_PLATFORM=...`:
  The Android API level to target.
  The minimum level supported by melonDS DS is 24.

You should also use the version of `cmake` that the NDK includes.

Here's an example configure step for `cmake` on Windows.
This command uses the NDK-bundled toolchain
to prepare a 64-bit ARM build for Android API level 24.

```pwsh
PS C:\Users\Jesse\Projects\melonds-ds> $Env:ANDROID_SDK_ROOT\cmake\3.22.1\bin\cmake.exe `
    -DANDROID_ABI=arm64-v8a `
    -DANDROID_PLATFORM=24 `
    -DCMAKE_TOOLCHAIN_FILE=$Env:ANDROID_NDK\build\cmake\android.toolchain.cmake
```

The command will be more or less the same on other platforms,
but the paths will be different.

See [here](https://developer.android.com/ndk/guides/cmake#variables) for more information
about these and other Android-specific CMake variables.

### iOS/tvOS

You will need to add the following flags to build for iOS or tvOS:

- `--toolchain=./cmake/toolchain/ios.toolchain.cmake`:
  The path to the `ios.toolchain.cmake` that's bundled with melonDS DS.
- `-DPLATFORM=...`:
  The target platform to build for.
  Use `OS64` for iOS and `TVOS` for tvOS.
  See `cmake/toolchain/ios.toolchain.cmake` for more information
  about the available CMake variables that this toolchain defines.
- `-DDEPLOYMENT_TARGET=...`:
  The minimum SDK version to target.
  The minimum level supported by melonDS DS is 14.

### Tracy Integration

melonDS DS supports the [Tracy](https://github.com/wolfpld/tracy) frame profiler.
To enable it, add `-DTRACY_ENABLE=ON` to the initial `cmake` command.

## CMake Variables

These are some of the most important CMake variables
that can be used to configure the build.
To see the rest, run `cmake -LH` in the build directory.

| Variable                          | Description                                                                                                            |
|-----------------------------------|------------------------------------------------------------------------------------------------------------------------|
| `ENABLE_OPENGL`                   | Whether to build the OpenGL renderer. Defaults to `ON` on Windows and Linux, `OFF` on other platforms.                 |
| `ENABLE_THREADED_RENDERER`        | Enables the multithreaded software renderer. Crashes when rewinding in RetroArch, so it's not generally available yet. |
| `TRACY_ENABLE`                    | Enables the Tracy frame profiler.                                                                                      |
| `MELONDS_REPOSITORY_URL`          | The Git repo from which melonDS will be cloned. Set this to use a fork.                                                |
| `MELONDS_REPOSITORY_TAG`          | The melonDS commit to use in the build.                                                                                |
| `FETCHCONTENT_SOURCE_DIR_MELONDS` | Path to a copy of the melonDS repo on your system. Set this to use a local branch _instead_ of cloning.                |
| `LIBRETRO_COMMON_REPOSITORY_URL`  | The Git repo from which `libretro-common` will be cloned. Set this to use a fork.                                      |
| `LIBRETRO_COMMON_REPOSITORY_TAG`  | The `libretro-common` commit to use in the build.                                                                      |

See [here](https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html)
and [here](https://cmake.org/cmake/help/latest/module/FetchContent.html#id8)
for more information about the variables that CMake and its modules define;
these can also be used to customize the build.

# About the Name

Various games received enhanced remakes or ports to the Nintendo DS, including such gems as:

- [Super Mario 64 DS](https://www.mobygames.com/game/31024/super-mario-64-ds)
- [Ridge Racer DS](https://www.mobygames.com/game/16054/ridge-racer-ds)
- [Brothers in Arms DS](https://www.mobygames.com/game/29865/brothers-in-arms-ds)
- [Mega Man Battle Network 5: Double Team DS](https://www.mobygames.com/game/23356/mega-man-battle-network-5-double-team-ds)
- [Diddy Kong Racing DS](https://www.mobygames.com/game/26746/diddy-kong-racing-ds)

What do these games have in common?
**They're all remakes or enhanced ports with a suffix of "DS"!**

I see this core as an enhanced remake of the [legacy melonDS core][melonds-libretro],
so I wanted to embody that in the name.

# Special Thanks

- The [melonDS team][melonds-github] for making a great emulator
  and for their help on Discord.
- The [libretro team][libretro] for making a great app,
  for their help on Discord,
  and for fixing RetroArch bugs that affected melonDS DS.
- Everyone who's ever reported a bug,
  for their role in ensuring a polished product.
- Nintendo, for all the memories.

# Disclaimers

This project is not affiliated with, developed by, or endorsed by the melonDS team or by Nintendo.

[kaeru]: https://kaeru.world/projects/wfc
[libretro]: https://www.libretro.com
[retroarch]: https://www.retroarch.com
[melonds]: https://melonds.kuribo64.net
[melonds-github]: https://github.com/melonDS-emu
[melonds-libretro]: https://github.com/libretro/melonDS
[issue-tracker]: https://github.com/JesseTG/melonds-ds/issues
[memory-pak]: https://en.wikipedia.org/wiki/List_of_Nintendo_DS_accessories#Memory_Expansion_Pak
[solar-sensor]: https://en.wikipedia.org/wiki/List_of_Nintendo_DS_accessories#Solar_Sensors
[retroachievements]: https://retroachievements.org
[retroarch-nightly]: https://buildbot.libretro.com/nightly
[wiimfi]: https://wiimmfi.de
[workflows]: https://nightly.link/JesseTG/melonds-ds/workflows/main.yaml/main