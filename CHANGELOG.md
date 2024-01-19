# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0),
and this project roughly adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

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