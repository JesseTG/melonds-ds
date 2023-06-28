# melonDS DS

An enhanced remake of the [melonDS][melonds-libretro] core for [libretro][libretro] that prioritizes upstream compatibility.

# Getting melonDS DS

At the moment, melonDS DS does not have a stable release.
Once it attains feature parity with the original core,
I will set up a process for stable releases
and submit it for inclusion in the official RetroArch distribution.

In the meantime, I suggest [building melonDS DS from source](#building).

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
2. Scan your Nintendo DS game library with the <kbd>Import Content</kbd> menu to build a playlist if you haven't already.
   If you have ROM hacks or homebrew, 
   you may need to manually add them to the playlist
   with the <kbd>Manual Scan</kbd> submenu.
3. Load a Nintendo DS game from the playlist.
   If you have the existing melonDS core installed,
   you may need to select melonDS DS explicitly.

### Installing Nintendo DS BIOS

melonDS includes a high-level BIOS replacement that works with most games.
However, some functionality requires original Nintendo DS or DSi BIOS files:

- Game Boy Advance connectivity requires native Nintendo DS BIOS and firmware.
- DSi mode requires native Nintendo DSi BIOS, firmware, and NAND files.

You can place your BIOS files in RetroArch's `system` directory, named as follows:

- Nintendo DS ARM7 BIOS: `bios7.bin`
- Nintendo DS ARM9 BIOS: `bios9.bin`
- Nintendo DS Firmware: `firmware.bin`
- Nintendo DSi ARM7 BIOS: `dsi_bios7.bin`
- Nintendo DSi ARM9 BIOS: `dsi_bios9.bin`
- Nintendo DSi Firmware: `dsi_firmware.bin`
- Nintendo DSi System NAND: `dsi_nand.bin`

### Game Boy Advance Connectivity

**NOTE:** melonDS and melonDS DS do not support emulating Game Boy Advance games.
They _do_ support loading Game Boy Advance ROMs
and save data from most major emulators.

If you want to load a Game Boy Advance ROM,
the steps are a little more involved.
melonDS (and therefore melonDS DS) only supports Game Boy Advance connectivity
with a native Nintendo DS BIOS.

1. Install the Nintendo DS BIOS and firmware as described above.
2. Load the melonDS DS core using the <kbd>Load Core</kbd> menu.
3. Enter the <kbd>Subsystems</kbd> menu and select <kbd>Load Slot 1 & 2 Boot</kbd>.
4. Select a Nintendo DS ROM, a Game Boy Advance ROM, and optionally a Game Boy Advance save file (in that order).
5. Start the game.

This combination of ROMs will appear in your History playlist,
so you won't have to repeat this process every time you want to play.

# New Features

Enhancements over the original [melonDS][melonds-libretro] core include:

## Upstream Parity

melonDS DS is not a fork of an existing code base;
instead, it uses standalone melonDS as a statically-linked dependency.
This means melonDS DS is less sensitive to large changes and merge conflicts.
As a result, it can take advantage of improvements to standalone melonDS
almost as quickly as they're released!

## Homebrew Save Data

The existing melonDS core does not support save data for homebrew games.
However, melonDS DS does!

You can keep homebrew save data on its own virtual SD card,
or use one of five shared virtual cards.
No extra setup is required;
the options menu will tell you everything you need to know.

**NOTE:** melonDS DS does not support savestates for homebrew games.

## Microphone Support

melonDS DS supports libretro's new microphone API,
allowing you to use your computer's microphone as a DS microphone!

**NOTE:** The most recent stable release of RetroArch (as of this writing)
does not yet include microphone support.
You will need to use a nightly build.

# Missing Features

These features have not yet been implemented in standalone [melonDS][melonds],
or they're not feasible to port to melonDS DS.
If you want to see them, you should contribute to the upstream project!

- **Local Wireless:**
  Upstream melonDS supports emulating local wireless multiplayer
  (e.g. Multi-Card Play, Download Play) across multiple instances of melonDS on the same computer.
  However, melonDS DS does not support this functionality due to libretro limitations;
  this feature is unlikely to be supported unless
  melonDS's local wireless support is refactored to work in a single process.
  melonDS does not currently support emulating local wireless multiplayer over the Internet.
- **Homebrew Savestates:**
  melonDS has limited support for taking savestates of homebrew games,
  as the (virtual) SD card is not included in savestate data.
- **DSi Savestates:**
  Nintendo DSi mode does not support savestates.
  This implies that rewinding is not supported in DSi mode.
- **DSi Direct Boot:**
  Direct Boot does not support DSiWare games at this time.
  They must be installed on a NAND image,
  and they must be started from the DSi menu.
- **Game Boy Advance Emulation:**
  melonDS can load Game Boy Advance ROMs and save data for use by compatible Nintendo DS games,
  but it cannot actually emulate the GBA.
  GBA emulation is not within the scope of melonDS;
  use a GBA emulator instead.
- **Slot-2 Accessories:**
  Except for the [solar sensor][solar-sensor] and
  [Memory Expansion Pak][memory-pak],
  melonDS does not support emulating Slot-2 accessories.

# Compatibility

## Games

melonDS DS is compatible with all games that melonDS supports,
unless otherwise noted in the [Missing Features section](#missing-features).
If this is not the case, please [report it][issue-tracker].

## Libretro Frontends

melonDS DS primarily targets RetroArch,
but you may be able to use it with other libretro frontends.
If you encounter problems with other frontends, please [report them][issue-tracker]!
Support is not guaranteed, but I'll do the best I can.

# Roadmap

The ultimate goal is for melonDS DS to supersede the [existing melonDS core][melonds-libretro].
This is a rough roadmap for achieving that goal:

1. Attain feature parity with the [existing libretro core][melonds-libretro].
2. Implement support for Nintendo Wi-Fi Connection.
3. Implement support for migrating configuration from the existing core.
4. Implement support for the [solar sensor][solar-sensor] using `retro_sensor_interface`.
5. Get melonDS DS included in the official RetroArch distribution.
6. Improve screen layout selection (e.g. toggling between multiple layouts, rotation).
7. Produce builds for platforms other than Windows, macOS, and Linux.
8. Add support for DSi mode (including DSiWare),
   subject to the limitations described in [the Missing Features section](#missing-features).
9. Add support for the DSi camera using `retro_camera_callback`.
10. Support Action Replay cheat codes.

# Building

MelonDS DS is built with CMake.

## Dependencies

You will need to install the following beforehand:

- CMake 3.15 or later
- Git
- A C++17 compiler

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
3. Proceed to [Compilation](#compilation).

### Linux

1. Install dependencies like so:

   ```sh
   sudo apt install cmake git pkg-config # Ubuntu/Debian
   sudo pacman -S base-devel cmake extra-cmake-modules git # Arch Linux
   ```
2. Proceed to [Compilation](#compilation).

## Compilation

Once you've installed the dependencies, the process for building melonDS DS is the same on all platforms:

```sh
git clone https://github.com/JesseTG/melonds-ds
cd melonds-ds
cmake -B build # Generate the build system
cmake --build build # Build the project
```

## CMake Variables

The following CMake variables can be used to configure the build:

| Variable                         | Description                                                                       |
|----------------------------------|-----------------------------------------------------------------------------------|
| `MELONDS_REPOSITORY_URL`         | The Git repo from which melonDS will be cloned. Set this to use a fork.           |
| `MELONDS_REPOSITORY_TAG`         | The melonDS commit to use in the build.                                           |
| `LIBRETRO_COMMON_REPOSITORY_URL` | The Git repo from which `libretro-common` will be cloned. Set this to use a fork. |
| `LIBRETRO_COMMON_REPOSITORY_TAG` | The `libretro-common` commit to use in the build.                                 |

See [here](https://cmake.org/cmake/help/latest/manual/cmake-variables.7.html) for more information
about the variables that CMake defines.

# About the Name

I see this core as an enhanced remake of the [original libretro core][melonds-libretro].
Various games received enhanced remakes or ports to the Nintendo DS, including such gems as:

- [Super Mario 64 DS](https://www.mobygames.com/game/31024/super-mario-64-ds)
- [Ridge Racer DS](https://www.mobygames.com/game/16054/ridge-racer-ds)
- [Brothers in Arms DS](https://www.mobygames.com/game/29865/brothers-in-arms-ds)
- [Mega Man Battle Network 5: Double Team DS](https://www.mobygames.com/game/23356/mega-man-battle-network-5-double-team-ds)
- [Diddy Kong Racing DS](https://www.mobygames.com/game/26746/diddy-kong-racing-ds)

What do these games have in common?
**They're all remakes or enhanced ports with a suffix of "DS"!**
I figured I'd get in on the fun.

# Special Thanks

- The [melonDS team](https://github.com/melonDS-emu) for making a great emulator
  and for being very helpful on Discord.
- Nintendo, for all the memories.

# Disclaimers

This project is not affiliated with, developed by, or endorsed by the melonDS team or by Nintendo.

[libretro]: https://www.libretro.com
[retroarch]: https://www.retroarch.com
[melonds]: https://melonds.kuribo64.net
[melonds-libretro]: https://github.com/libretro/melonDS
[issue-tracker]: https://github.com/JesseTG/melonds-ds/issues
[memory-pak]: https://en.wikipedia.org/wiki/List_of_Nintendo_DS_accessories#Memory_Expansion_Pak
[solar-sensor]: https://en.wikipedia.org/wiki/List_of_Nintendo_DS_accessories#Solar_Sensors
[retroachievements]: https://retroachievements.org
[wiimfi]: https://wiimmfi.de