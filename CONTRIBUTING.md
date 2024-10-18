# Contributing to melonDS DS

Thanks for your interest in contributing to melonDS DS!
There are many ways you can help improve the project,
even if you're not a coder.

## Contributing Upstream

First and foremost, you can help improve melonDS DS
by contributing to the [underlying emulator][melonds-contributing]
that this project is based on;
since parity with standalone melonDS is a priority for melonDS DS,
most improvements to the original emulator
will eventually make their way here.

## Reporting Issues

Found a bug? Have a feature request?
You can open a ticket by going [here][melondsds-issues]
and following the instructions --
or if your issue is something I already know about,
you can comment on an existing ticket
with details about how it affects you.

Having a record of bugs or feature requests helps me keep my backlog organized
and plan long-term improvements to the project,
so don't be shy!

When reporting a bug,
you'll usually want to include supporting information.
Here are some common artifacts that I may ask for:

### Logs

Providing a log when reporting a bug or asking for help
can eliminate a lot of blind guesswork.
When in doubt, include a log.

See [here](https://docs.libretro.com/guides/generating-retroarch-logs)
for guidance on generating a log with RetroArch.
Instructions may vary for other libretro frontends.

### Performance Traces

melonDS DS supports the [Tracy][tracy] frame profiler,
which is a great way to diagnose performance issues.
You can take a trace with the following steps,
including for mobile builds:

1. Download a `RelWithDebInfo` build of melonDS DS for your platform
   from the [Releases][melondsds-releases]
   and replace your installed copy of the core with it.
2. Download, install, and launch [Tracy][tracy].
3. If you're running RetroArch and Tracy on different devices,
   enter the IP address of the device running RetroArch in the "client address" field.
4. Start your game in RetroArch.
   The trace will start as soon as the core loads,
   and the Tracy window will start updating immediately.
5. Perform the actions that you want to profile
   (i.e. do the thing that's causing the slowdown).
6. Close RetroArch to stop the trace.
7. Save the trace to a file,
   then attach it to the relevant ticket.

> [!NOTE]
> For most platforms, running RetroArch (or any Tracy-instrumented app)
> with admin privileges will allow Tracy to capture extra data.
> However, the resulting trace **will contain personal information**
> about everything else that's running on your system.
>
> If you capture a trace with elevated privileges,
> don't post it publicly.
> Send it to me privately instead.

## Translating Text

melonDS DS is only available in English right now,
but support for other languages is planned.
Once the infrastructure for that is in place,
you'll be able to help translate melonDS DS
through libretro's [Crowdin][libretro-crowdin] project.

## Sponsorship

Spare change burning a hole in your bank account?
[Sponsoring me through GitHub Sponsors][sponsor] is a great way to say thanks!
melonDS DS is a passion project that will always be free to use
(just like the original emulator),
but I'm always grateful for financial support!
The fine people behind [melonDS](https://melonds.kuribo64.net/donate.php)
and [RetroArch](https://www.retroarch.com/index.php?page=donate)
certainly wouldn't mind, either.

> [!NOTE]
> Right now I can't guarantee specific benefits for sponsors,
> but I will reevaluate that stance if I see a sustained desire for it.

## Spreading the Word

If you enjoy using melonDS DS,
you can help me out by spreading the good word!
Tweet it, stream it, invite me on your podcast, book a television ad --
whatever you think will help.
The more people who use the project,
the more likely one of them will want to contribute
(and the more joy it'll bring to players)!

## Contributing Code

> [!TIP]
> Submitting improvements to melonDS DS is a great way to help out,
> but it also requires the most attention and coordination.
> I don't want to see your hard work go to waste,
> so if there's something specific you want to work on
> then I _strongly_ recommend you run it by me beforehand.

The following sections explain how to start
building and running melonDS DS locally.

### Installing Dependencies

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

#### iOS & tvOS

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

### Customizing the Build

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

### Getting Your Patch Merged

Remember, I ultimately have to maintain whatever changes you submit.
Submissions that complicate this would harm melonDS DS in the long run,
so I will be picky about what gets merged.

That said, I _want_ you to succeed!
If you come to me in advance with your idea,
I can guide you towards a solution that everyone can be happy with --
or at least prevent you from wasting time on a non-starter.

Here are some rules and guidelines you'll need to follow
as you implement your contribution:

#### Tests

melonDS DS has a suite of tests to ensure that
most of the core works as expected.
The tests will automatically be run on the CI pipeline
when new commits are pushed.

> [!TIP]
> You are _strongly_ encouraged to run the test suite locally,
> as doing so will help you catch issues early and speed up iteration.
> See [here][melondsds-tests] for instructions on doing so.

**All builds must succeed and all test cases must pass
for your contribution to be merged,
barring exceptional circumstances.**

You encouraged to write new tests
if doing so makes sense for your contribution
(and [libretro.py][libretro.py] supports it) --
I may even ask you to do so as a condition of merging.

Note that tests are only run on GitHub Actions --
they are not run on libretro's build infrastructure.

#### Style

There isn't currently a style guide for the codebase (C++ or otherwise).
But I do have some rules you'll need to follow:

- Do not introduce new dependencies unless absolutely necessary.
- If you _do_ need to introduce a new dependency,
  then fetch it at configure-time with `FetchContent` instead of vendoring it.
  See [here](cmake/FetchDependencies.cmake) for more details.
- All C++ code (including dependencies) *must* be built as C++17.
- All text should use [Semantic Line Breaks][sembr] (aka SemBr),
  including comments and string literals in the code.
  It helps with readability and version control.
- Please update documentation and comments as you work,
  if relevant to your changes.

[libretro.py]: https://github.com/JesseTG/libretro.py
[libretro-crowdin]: https://docs.libretro.com/development/retroarch/new-translations-crowdin
[melonds]: https://github.com/melonDS-emu/melonDS
[melonds-contributing]: https://github.com/melonDS-emu/melonDS/blob/master/CONTRIBUTING.md
[melondsds-actions]: https://github.com/JesseTG/melonds-ds/actions
[melondsds-issues]: https://github.com/JesseTG/melonds-ds/issues/new/choose
[melondsds-releases]: https://github.com/JesseTG/melonds-ds/releases
[melondsds-tests]: test/README.md
[sembr]: https://sembr.org
[sponsor]: https://github.com/sponsors/JesseTG
[tracy]: https://github.com/wolfpld/tracy