# melonDS DS

An enhanced remake of the [melonDS][melonds] [core][melonds-libretro] for [libretro][libretro]
that prioritizes standalone parity, reliability, and a smooth user experience.

# Getting melonDS DS

At the moment, melonDS DS does not have a stable release.
I intend to set up a process for those soon.
Once melonDS DS matches all the features provided by the legacy melonDS core,
I will submit it for inclusion in the official RetroArch distribution.

In the meantime, I suggest [building melonDS DS from source](#building)
or [downloading one of the raw build artifacts][workflows].

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
   If you have the existing melonDS core installed,
   you may need to select melonDS DS explicitly.

> [!IMPORTANT]
> If you have ROM hacks or homebrew,
> you may need to manually add them to the playlist
> with the <kbd>Manual Scan</kbd> submenu.

### Installing Nintendo DS BIOS

melonDS includes a high-level BIOS replacement that works with most games.
However, some functionality requires original Nintendo DS or DSi BIOS files:

- Game Boy Advance connectivity requires native Nintendo DS BIOS and firmware.
- DSi mode requires native Nintendo DSi BIOS, firmware, and NAND files.

You can place your BIOS files in RetroArch's `system` directory, named as follows:

- DS ARM7 BIOS: `bios7.bin`
- DS ARM9 BIOS: `bios9.bin`
- DS Firmware: `firmware.bin`
- DSi ARM7 BIOS: `dsi_bios7.bin`
- DSi ARM9 BIOS: `dsi_bios9.bin`
- DSi Firmware: `dsi_firmware.bin`
- DSi System NAND: Anything, it's auto-detected

### Game Boy Advance Connectivity

The steps for loading a Game Boy Advance ROM are a little more involved.
melonDS (and therefore melonDS DS) only supports GBA connectivity
with a native Nintendo DS BIOS.

1. Install the Nintendo DS BIOS and firmware [as described above](#installing-nintendo-ds-bios).
2. Load the melonDS DS core using the <kbd>Load Core</kbd> menu.
3. Enter the <kbd>Subsystems</kbd> menu and select <kbd>Load Slot 1 & 2 Boot</kbd>.
4. Select a Nintendo DS ROM, a Game Boy Advance ROM, and optionally a Game Boy Advance save file (in that order).
5. Start the game.

This combination of ROMs will appear in your History playlist,
so you won't have to repeat this process every time you want to play.

> [!NOTE]
> melonDS can load Game Boy Advance ROMs and save data
> for the purpose of Slot-2 connectivity,
> but it cannot actually play GBA games.
> Use a GBA core instead.

# New Features

Enhancements over the legacy [melonDS core][melonds-libretro] include:

## Standalone Parity

Unlike most other libretro cores,
melonDS DS is not a fork of an existing code base.
It uses standalone melonDS as a statically-linked dependency,
which means that large changes and merge conflicts are less of an issue.
As a result, improvements to standalone melonDS can be integrated
almost as quickly as they're released!

## Wi-Fi Support

Wi-fi is fully emulated on all platforms!
You can use any server reimplementation that works with standalone melonDS,
but I recommend [Kaeru WFC][kaeru] due to its ease of use.
Follow the instructions at the link for instructions on getting started.

Future versions of melonDS DS may use the Kaeru WFC server by default.

> [!NOTE]
> Do not confuse this with local multiplayer.
> melonDS DS does not support emulating local wireless
> at this time.

## Homebrew Save Data

The legacy melonDS core does not support save data for homebrew games.
However, melonDS DS does!

melonDS DS looks in the `system` directory (i.e. alongside the BIOS files)
for a homebrew SD card image named `dldi_sd_card.bin`.
If one doesn't exist, a virtual 4GB SD card will be created if necessary.
See the core options for more information.

> [!NOTE]
> melonDS DS does not support savestates for homebrew games.

## Microphone Support

melonDS DS supports libretro's new microphone API,
allowing you to use your device's microphone for Nintendo DS games!

> [!NOTE]
> The most recent stable release of RetroArch (as of this writing)
> does not include microphone support.
> You will need to use [a nightly build][retroarch-nightly].
> Additionally, some platforms may not have microphone support yet.

## Screen Rotation

melonDS DS fully supports rotating the emulated DS left, right, and upside-down!
Now you can play games that were meant to be played sideways,
such as Brain Age.

## Enhanced Screen Layout Options

The legacy melonDS core supports multiple screen layouts,
but the only way to cycle through them is through the core options menu.
This is inconvenient if a single game uses different layouts.

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

## Other Niceties

- **Battery Support:** melonDS DS uses libretro's new power state API
  so that your device's power status (battery level, charging status, etc.)
  is reflected in the emulated DS.
  This feature requires a nightly build of RetroArch as of this writing.
  Some platforms may not support it.
- **Selectable NAND Images:** You don't need to hard-code a specific name for your DSi NAND image!
  Just place it in your system directory and select it from the core options.

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
  as the virtual SD card is not included in savestate data.
- **DSi Savestates:**
  Nintendo DSi mode does not support savestates.
  This also implies that rewinding is not supported in DSi mode.
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
- **Slot-2 Accessories:**
  Except for the [solar sensor][solar-sensor] and
  [Memory Expansion Pak][memory-pak],
  melonDS does not support emulating Slot-2 accessories.
  However, melonDS DS does not yet support these devices.

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

The ultimate goal is for melonDS DS to supersede the [legacy melonDS core][melonds-libretro].
This is a rough roadmap for achieving that goal:

1. Fix all major outstanding bugs.
2. Get melonDS DS included in the official RetroArch distribution.
3. Implement support for migrating configuration from the existing core.
4. Implement support for the [solar sensor][solar-sensor] using `retro_sensor_interface`.
5. Add support for the DSi camera using `retro_camera_callback`.

# Building

MelonDS DS is built with CMake.

## Dependencies

You will need to install the following beforehand:

- CMake 3.15 or later
- Git
- A C++17 compiler (MSVC is not supported)

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

- The [melonDS team][melonds-github] for making a great emulator
  and for being very helpful on Discord.
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