# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0),
and this project roughly adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Bugfixes and minor improvements will increment the patch version.
New features will increment the minor version.
Breaking changes (**except for savestates**) will increment the major version;
a design goal is to avoid a 2.x release for as long as possible.

## [Unreleased]

With thanks to **@parkerlreed**, **@romatthe**, and **@scarrillo**
for their donations this month!

### Added

- Added support for emulated LAN multiplayer.
  Only works on an actual LAN.
  [#225](https://github.com/JesseTG/melonds-ds/issues/225)
  **Thanks for [#229](https://github.com/JesseTG/melonds-ds/pull/242), @BernardoGomesNegri!**
- Integrated support for the GBA Memory Expansion Pak.
  [#44](https://github.com/JesseTG/melonds-ds/issues/44)
- Integrated support for the Rumble Pak.
  [#224](https://github.com/JesseTG/melonds-ds/issues/224)
- Integrated support for the Solar Sensor.
  It can be enabled in the core options menu,
  even without using a real Boktai trilogy ROM.
  [#43](https://github.com/JesseTG/melonds-ds/issues/43)
- Integrated support for Gaussian audio interpolation.
  [#234](https://github.com/JesseTG/melonds-ds/issues/234)
- Added support for direct-booting DSiWare games.
  DSi system files are still required.
  [#148](https://github.com/JesseTG/melonds-ds/issues/148)
  **Thanks for the technical information, @CasualPokePlayer!**

### Changed

- Updated melonDS to commit [d8f1d10](https://github.com/melonDS-emu/melonDS/tree/d8f1d10).
- Refactored input handling to enable future improvements
  to Slot-2 peripherals and screen layouts.
- Disabled fast-forwarding while a LAN multiplayer session is active.
- Moved the <kbd>Screen Filtering</kbd> option to the <kbd>Screen</kbd> category,
  renamed it to <kbd>Hybrid Screen Filtering</kbd>,
  and hide it if none of the configured layouts is a hybrid layout.
  [#250](https://github.com/JesseTG/melonds-ds/issues/250)

### Fixed

- Adjusted the core's framerate and sample rate
  to match that of the actual hardware.
- Show the screen filtering option even when the software renderer is enabled.
  [#250](https://github.com/JesseTG/melonds-ds/issues/250)
- Fixed a crash that would occur when inadvertently loading
  NDS or GBA save data as a ROM.
  The core now exits with an error message instead.

## [1.1.8] - 2024-10-18

Thanks to **@oddballparty** and a private sponsor for their generosity!

### Added

- Added `RelWithDebInfo` builds that include Tracy support.
  These will be distributed on GitHub for all supported platforms,
  starting with this release.
- Added `Debug` builds that include debugging information and limited optimizations.
  Useful for debugging crashes and other issues,
  not recommended for general gameplay.
- Added a contributor's guide at `CONTRIBUTING.md`.
  [#107](https://github.com/JesseTG/melonds-ds/issues/107)
- Added guidelines for reporting security vulnerabilities at `SECURITY.md`.
- Added right-handed versions of the hybrid screen layouts.
  [#38](https://github.com/JesseTG/melonds-ds/issues/38)
  **Thanks for [#229](https://github.com/JesseTG/melonds-ds/pull/229), @roblar91!**

### Changed

- Moved build instructions from `README.md` to the new `CONTRIBUTING.md`.

### Fixed

- Fixed encrypted NDS ROMs failing to load without any feedback;
  loading one without using the native BIOS will now display an error message.
  [#228](https://github.com/JesseTG/melonds-ds/issues/228)
- Fixed Blow mode for emulated microphone input not being implemented
  despite being available in the core options.
  [#187](https://github.com/JesseTG/melonds-ds/issues/187)

## [1.1.7] - 2024-08-20

### Fixed

- Fixed a build error on iOS on the libretro infrastructure.

## [1.1.6] - 2024-08-20

### Changed

- Updated melonDS to commit [824eb37](https://github.com/melonDS-emu/melonDS/tree/824eb37).
- **BREAKING:** The savestate format has changed.
  Savestates from previous versions are incompatible with this one.
  Please save your progress in-game before updating.

### Fixed

- Fixed cheat codes not being applied to the game. [#196](https://github.com/JesseTG/melonds-ds/issues/196)
- Fixed changes to the built-in firmware's settings and data
  not being persisted to disk.
  [#211](https://github.com/JesseTG/melonds-ds/issues/211)
- Fixed the built-in firmware's Wi-fi settings
  not being loaded from the correct file.
  [#205](https://github.com/JesseTG/melonds-ds/issues/205)

## [1.1.5] - 2024-07-25

### Fixed

- Fixed system files from other cores
  being incorrectly treated as NDS firmware images,
  which resulted in strange behavior when such a file was chosen.
  [#183](https://github.com/JesseTG/melonds-ds/issues/183)

## [1.1.4] - 2024-07-08

### Changed

- Updated the "DSi NAND Path" core option description to clarify the role of the no$gba footer.

### Fixed

- Fixed some log entries not being output with a newline.
- Fixed a crash when using a hybrid screen layout with a screen ratio of 3:1.
- Fixed DSi NAND images not being recognized if they lacked a no$gba footer
  despite having equivalent data at offset 0xFF800. [#195](https://github.com/JesseTG/melonds-ds/issues/195)
- Fixed the screen being rendered when using the OpenGL renderer while the emulated lid is closed,
  which caused flickering in some games. [#214](https://github.com/JesseTG/melonds-ds/issues/214)

## [1.1.3] - 2024-06-14

### Fixed

- Fixed a crash that would occur when attempting to use the OpenGL renderer on some GPUs. [#203](https://github.com/JesseTG/melonds-ds/issues/203)

## [1.1.2] - 2024-06-12

### Fixed

- Fixed a bug where native BIOS images would be used
  when the core was supposed to fall back to built-in system files.
- Fixed a bug where GBA SRAM wouldn't be loaded.
- Fixed a bug where the core would crash when trying to load the error screen
  while using the OpenGL renderer.

## [1.1.1] - 2024-02-29

### Fixed

- Fixed a bug where the core would crash
  when RetroArch autosaves after a reset.

## [1.1.0] - 2024-02-13

### Added

- Added a subsystem for loading a GBA ROM without save data.
- Added options to configure the emulated console's starting time.
  The starting time can be the local time (possibly with an offset) or an absolute time.
- Added the ability to synchronize the emulated console's time with the host system's time.

### Fixed

- Fixed an issue where some homebrew would be incorrectly detected as DSiWare,
  resulting in a crash.
- Fixed an issue where trying to load an NDS ROM and a GBA ROM
  without GBA save data would fail.

## [1.0.3] - 2024-01-30

### Fixed

- Fixed an issue where most screen gap sizes would not be honored.
- Fixed an issue where usernames with non-ASCII characters would crash the core
  or be improperly truncated.
  Such names are now properly converted to UCS-2 if possible,
  or else the default username is used instead.

## [1.0.2] - 2024-01-19

### Changed

- Add support for screen gaps between 0 and 126 pixels, inclusive.
  128-pixel gaps are no longer available due to
  libretro limits on how many core option values can be defined.

### Fixed

- Fixed an issue where the emulated console could not connect to the network.

## [1.0.1] - 2024-01-12

### Fixed

- Unmark the core as experimental so it appears in RetroArch's core downloader by default.

## [1.0.0] - 2024-01-12

First stable release.

### Fixed

- Rename the Android build artifact to `melondsds_libretro_android.so`
  to adhere to libretro's naming conventions.
  If you installed it manually, be sure to remove the older release.
- Fixed an issue where the version number would be displayed incorrectly
  in the core information menu.

## [0.8.7] - 2024-01-09

### Changed

- Temporarily mark the core as experimental until
  we're sure that the Buildbot can handle it.

## [0.8.6] - 2024-01-09

### Fixed

- Test release for CI purposes.

## [0.8.5] - 2024-01-09

### Fixed

- Test release for CI purposes.

## [0.8.4] - 2024-01-09

### Fixed

- Test release for CI purposes.

## [0.8.3] - 2024-01-09

### Fixed

- Test release for CI purposes.

## [0.8.2] - 2024-01-09

### Fixed

- Bump the version number after the release workflow failed to start.

## [0.8.1] - 2024-01-09

### Fixed

- Bump the version number after the release workflow failed to start.

## [0.8.0] - 2024-01-09

### Added

- Enabled the threaded software renderer after fixing a related bug.

### Fixed

- Fixed the graphics not displaying when switching
  from the OpenGL renderer to the software renderer at runtime.
- Fixed a crash that occurred when rewinding in RetroArch
  while using the threaded software renderer.
- Fixed a typo in the description for a core option.
- Fixed changes to the software renderer's threading mode not being honored.

### Changed

- Updated melonDS to commit 740305c.
- Enabled the threaded software renderer by default.

## [0.7.29] - 2024-01-02

### Fixed

- Fixed the screen remaining black if restarting the game with the OpenGL renderer.
- Fixed an incorrect aspect ratio if changing the screen layout from the core options menu mid-game.
- Fixed a loaded DSiWare ROM from being inserted into the virtual cartridge slot.

### Changed

- Reordered some options in the Screen category to prevent the cursor from jumping around
  when adjusting options that affect other options' visibility.

## [0.7.28] - 2023-12-28

### Changed

- Updated melonDS to commit c926f79.
- Stop providing universal macOS builds in favor of split x64/ARM64 builds.
- Updated glm to commit 7882684.

### Fixed

- Fixed a memory leak involving the software renderer.
- Fixed homebrew ROMs not saving data to the SD card.
- Fixed incorrect information in some core option descriptions.

## [0.7.27] - 2023-12-26

### Changed

- Updated melonDS to commit d55a384.

### Fixed

- Fixed homebrew ROMs being rejected due to an invalid logo checksum.
  (Only retail ROMs need embedded logo data.)
- Fixed an issue where the "Screen Filtering" option wouldn't always be honored.
- Fixed the core crashing when the OpenGL context is lost
  while the OpenGL renderer is active.
- Fixed an issue where SD card images would be created with the wrong size.
- Fixed a crash that occurred in the in-core error screen.
- Fixed system files not being found inside a subdirectory named "melonDS DS".

## [0.7.26] - 2023-12-19

### Changed

- Updated melonDS to commit 24cb428.
- Updated libretro-common to commit fce57fd.
- Allow switching between the software and OpenGL renderers
  (where available) without restarting the core.

### Fixed

- GBA ROM and save data support is now supported with the built-in BIOS.
- Fixed a crash that occurred upon starting the core.
- Fixed a potential crash when attempting to load an invalid NDS ROM.
- Fixed an issue where the software renderer would not clear the entire framebuffer
  when using a hybrid screen layout
- Fix an issue where the OpenGL state wouldn't be updated
  when the screen layout was changed.
- Fixed an issue where the JIT wouldn't be used even if it was enabled.

## [0.7.25] - 2023-11-06

### Changed

- Test release.

## [0.7.24] - 2023-11-06

### Changed

- Test release.

## [0.7.23] - 2023-11-06

### Changed

- Fixed a typo.

## [0.7.22] - 2023-11-06

### Changed

- Fixed `git remote add` usage.

## [0.7.21] - 2023-11-06

### Changed

- Some more housekeeping related to `libretro-super`.

## [0.7.20] - 2023-11-06

### Changed

- See if `libretro-super` gets a pull request now that I'm in @libretro.

## [0.7.19] - 2023-11-06

### Changed

- Submit a pull request to `libretro-super` when this file is updated.

## [0.7.18] - 2023-11-06

### Changed

- Bump version number.

## [0.7.17] - 2023-11-06

### Changed

- Ensure my fork of `libretro-super` is up-to-date before opening a PR.

## [0.7.16] - 2023-11-06

### Changed

- Submit the PR to update the `.info` file.

## [0.7.15] - 2023-11-06

### Changed

- Fix the release branch creation workflow.

## [0.7.14] - 2023-11-06

### Changed

- Another test release as I work out the submission workflow.

## [0.7.13] - 2023-11-06

### Changed

- Simplify the workflow for submitting releases to the libretro buildbot.

## [0.7.12] - 2023-11-06

### Changed

- Another attempt at fixing the release workflow.

## [0.7.11] - 2023-11-06

### Changed

- Added an initial workflow for submitting releases to the libretro buildbot.

## [0.7.10] - 2023-11-06

I'm still working out a workflow for releases,
but that process is almost done.

### Fixed

- Simplify the hierarchy of the artifacts in the release.

## [0.7.9] - 2023-11-06

This is an overview of this release's changes.

### Fixed

- Still another test release.

## [0.7.8] - 2023-11-06

This is an overview of this release's changes.

### Fixed

- Still another test release.

## [0.7.7] - 2023-11-06

This is an overview of this release's changes.

### Fixed

- Another test release. Getting closer...

## [0.7.6] - 2023-11-03

This is an overview of this release's changes. Pretty cool, huh?

### Fixed

- Another test release.

## [0.7.5] - 2023-11-03

### Fixed

- Test release.

## [0.7.4] - 2023-11-03

### Fixed

- Test release.

## [0.7.3] - 2023-11-03

### Fixed

- Test release.

## [0.7.2] - 2023-11-02

### Added

- Added changelog.
- Added all features from legacy melonDS core except for
  the threaded software renderer,
  OpenGL support for macOS,
  and support for 32-bit platforms and the Switch.

### Changed

- Test release to see what happens.
