# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0),
and this project roughly adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- Updated melonDS to commit 4b4239d.
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