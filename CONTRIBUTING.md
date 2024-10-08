# Contributing to melonDS DS

TODO thanks for helping me out
you can help me even if you're not a programmer

## Reporting Issues

## Capturing Traces

TODO use a tracy build

## Contributing Upstream

TODO you can also help out upstream
any changes you make will eventually make their way to melondsds
see [here][melonds-contributing] for more information

## Contributing Code

TODO build instructions


### Building

melonDS DS is built with CMake.

### Dependencies

You will need to install the following beforehand:

- CMake 3.19 or later
- Git
- A C++17 compiler (MSVC is not supported)

Most other dependencies are fetched automatically by CMake.

#### Windows

1. Install [MSYS2](https://www.msys2.org).
2. Open the <kbd>MSYS2 MinGW 64-bit</kbd> terminal from the Start Menu.
3. Install dependencies like so:

   ```sh
   pacman -Syu # update the package database
   pacman -S git mingw-w64-x86_64-{cmake,toolchain} # install dependencies
   ```
4. Proceed to [Compilation](#compilation).
   You may need to remain in the MSYS2 terminal.

#### macOS

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

#### Linux

1. Install dependencies like so:

   ```sh
   sudo apt install cmake git pkg-config # Ubuntu/Debian
   sudo pacman -S base-devel cmake extra-cmake-modules git # Arch Linux
   ```
2. Proceed to [Compilation](#compilation).

#### Android

1. Install the Android SDK and [NDK](https://developer.android.com/ndk/downloads).
   The simplest way to do this is through [Android Studio](https://developer.android.com/studio).
2. Proceed to [Compilation](#compilation).

#### iOS

These steps can only be done on macOS.

1. Install Xcode and the Xcode command-line tools.
2. Proceed to [Compilation](#compilation).

### Compilation

Once you've installed the dependencies,
the process for building melonDS DS is mostly the same on all platforms:

```sh
git clone https://github.com/JesseTG/melonds-ds
cd melonds-ds
cmake -B build # Generate the build system, and add any -D or --toolchain flags here
cmake --build build # Build the project
```

However, some platforms or features need you to add some extra flags to the first `cmake` command:

#### macOS

If building for the macOS architecture that your device uses,
no extra flags are required.
To produce a build for a specific architecture,
pass `-DCMAKE_OSX_ARCHITECTURES:STRING=$ARCH` to the initial `cmake` command,
where `$ARCH` is one of the following:

- `x86_64` for x86_64 builds.
- `arm64` for Apple Silicon builds.
- `x86_64;arm64` for universal builds.

> [!WARNING]
> Universal builds of melonDS DS are not supported,
> as [there is a history](https://github.com/JesseTG/melonds-ds/issues/131)
> of them not working reliably.

#### Android

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

#### iOS/tvOS

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

#### Tracy Integration

melonDS DS supports the [Tracy](https://github.com/wolfpld/tracy) frame profiler.
To enable it, add `-DTRACY_ENABLE=ON` to the initial `cmake` command.
For best results, build with the `RelWithDebInfo` configuration
by adding `-DCMAKE_BUILD_TYPE=RelWithDebInfo` when running `cmake`.

### CMake Variables

These are some of the most important CMake variables
that can be used to configure the build.
To see the rest, run `cmake -LH` in the build directory.

| Variable                          | Description                                                                                                            |
|-----------------------------------|------------------------------------------------------------------------------------------------------------------------|
| `ENABLE_OPENGL`                   | Whether to build the OpenGL renderer. Defaults to `ON` on Windows and Linux, `OFF` on other platforms.                 |
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

[melonds]: https://github.com/melonDS-emu/melonDS
[melonds-contributing]: https://github.com/melonDS-emu/melonDS/blob/master/CONTRIBUTING.md