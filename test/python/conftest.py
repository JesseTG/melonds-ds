"""
Fixtures shared by the melonDS DS test suite.

This suite gets its assets (ROMs, firmware, etc.) from environment variables
and its per-test configuration from markers and parametrization.

Each generated CTest test runs exactly one pytest node in its own process,
so nothing here needs to undo the global state
that loading a libretro core leaves behind.
"""

from __future__ import annotations

import ctypes
import os
import shutil
import sys
from collections.abc import Mapping
from pathlib import Path
from typing import Any

import pytest
from libretro import Content, Session, SubsystemContent, TempDirPathDriver

# The helper package holds a few invariant assertions of its own.
# pytest only rewrites conftest and test modules by default,
# so without this they'd report a bare `AssertionError` with no operand values.
pytest.register_assert_rewrite("melondsds")

from melondsds import SessionFactory
from melondsds.assets import (
    ASSET_VARIABLES,
    ASSETS,
    SYSTEM_FILE_MARKERS,
    asset_path,
    missing_assets,
)
from melondsds.options import CORE_SYSTEM_SUBDIR

#: Exit code that tells CTest "this test case was skipped".
#:
#: Paired with ``SKIP_RETURN_CODE 77`` in ``test/CMakeLists.txt``.
#: 77 is the conventional Automake "skipped" status,
#: and it doesn't collide with any value in :class:`pytest.ExitCode`,
#: which only uses 0-6.
SKIP_EXIT_CODE = 77


# --------------------------------------------------------------------------- #
# Prerequisites -> runtime skips
# --------------------------------------------------------------------------- #


def _asset_markers(item: pytest.Item) -> tuple[str, ...]:
    """Names of the asset markers applied to ``item``, including via parametrization."""
    return tuple(marker.name for marker in item.iter_markers() if marker.name in ASSETS)


def _required_variables(item: pytest.Item) -> tuple[str, ...]:
    """Environment variables ``item`` needs, de-duplicated, in declaration order."""
    names: list[str] = []
    for marker in _asset_markers(item):
        names.extend(ASSETS[marker])

    return tuple(dict.fromkeys(names))


def pytest_runtest_setup(item: pytest.Item) -> None:
    """
    Skip at runtime when a declared prerequisite isn't available.

    This has to be a *runtime* skip
    rather than an import-time :func:`pytest.mark.skipif`.
    pytest-cmake collects tests with only ``PYTHONPATH``
    and the platform library path set,
    so an import-time condition would evaluate against an empty environment
    and silently drop the affected tests from CTest entirely.
    """
    if missing := missing_assets(_required_variables(item)):
        pytest.skip("not configured: " + ", ".join(missing))

    if item.get_closest_marker("opengl") is not None:
        if os.environ.get("MELONDSDS_HAVE_OPENGL") != "1":
            pytest.skip("the core was built without OpenGL support")

        pytest.importorskip("moderngl", reason="install libretro.py[opengl]")


#: Windows C runtimes whose ``getenv`` the core might be reading.
#:
#: :data:`os.environ` writes through the CRT that CPython itself links,
#: which for an official build is ``ucrtbase.dll``.
#: A core built for MSYS2's MINGW64 environment links ``msvcrt.dll`` instead,
#: which keeps its own copy of the environment
#: that it snapshots when the process starts;
#: it never sees :meth:`~pytest.MonkeyPatch.setenv`.
#: Writing to every CRT keeps the suite working
#: no matter which one the core under test was built against.
_CRTS = ("msvcrt.dll", "ucrtbase.dll") if sys.platform == "win32" else ()


def _putenv_all_crts(name: str, value: str | None) -> None:
    """Set ``name`` in every CRT loaded into this process, or unset it if ``value`` is None."""
    if sys.platform != 'win32':
        return

    assignment = f"{name}={'' if value is None else value}".encode()

    for crt in _CRTS:
        ctypes.CDLL(crt)._putenv(assignment)


@pytest.fixture(autouse=True)
def _error_screen(request: pytest.FixtureRequest, monkeypatch: pytest.MonkeyPatch) -> None:
    """
    Control the core's in-emulator error screen.

    The core reads this with ``getenv`` when content is loaded
    (see ``src/libretro/core/core.cpp``),
    so flipping it per test works.
    Setting it unconditionally also makes a bare ``pytest`` run behave like a CTest run.

    Both branches go through :func:`_putenv_all_crts` as well as :mod:`monkeypatch`,
    because ``monkeypatch`` alone can't reach a core that links a different CRT.
    Nothing needs to be undone afterwards;
    every test takes one of these two branches,
    and each CTest case runs in its own process anyway.
    """
    assert isinstance(request.node, pytest.Item)
    if request.node.get_closest_marker("no_skip_error_screen") is not None:
        monkeypatch.delenv("MELONDSDS_SKIP_ERROR_SCREEN", raising=False)
        _putenv_all_crts("MELONDSDS_SKIP_ERROR_SCREEN", None)
    else:
        monkeypatch.setenv("MELONDSDS_SKIP_ERROR_SCREEN", "1")
        _putenv_all_crts("MELONDSDS_SKIP_ERROR_SCREEN", "1")


def pytest_sessionfinish(session: pytest.Session, exitstatus: int) -> None:
    """
    Exit with :data:`SKIP_EXIT_CODE` if the entire run was skipped.

    pytest exits 0 when every test is skipped,
    which CTest would otherwise report as "Passed".
    Each generated CTest test runs exactly one pytest node,
    so "everything was skipped" means "this test case was skipped".
    """
    if exitstatus != pytest.ExitCode.OK:
        return

    reporter = session.config.pluginmanager.getplugin("terminalreporter")
    if reporter is None:  # -p no:terminal
        return

    assert isinstance(reporter, pytest.TerminalReporter)

    executed = sum(
        len(reporter.stats.get(outcome, ()))
        for outcome in ("passed", "failed", "error", "xfailed", "xpassed")
    )
    if reporter.stats.get("skipped") and not executed:
        session.exitstatus = SKIP_EXIT_CODE


# --------------------------------------------------------------------------- #
# The core, its directories, and staged system files
# --------------------------------------------------------------------------- #


@pytest.fixture(scope="session")
def core_path() -> str:
    """Absolute path to the ``melondsds_libretro`` shared library under test."""
    path = os.environ.get("MELONDSDS_CORE")
    if not path:
        pytest.skip("MELONDSDS_CORE is unset; run the suite through CTest")

    if not Path(path).is_file():
        pytest.fail(f"MELONDSDS_CORE points at a missing file: {path}")

    return path


@pytest.fixture
def path_driver(core_path: str) -> TempDirPathDriver:
    """Build a fresh temporary system/save/assets tree, one per test."""
    return TempDirPathDriver(core_path, ".libretro")


@pytest.fixture
def system_dir(path_driver: TempDirPathDriver) -> Path:
    """Return the frontend's system directory."""
    return Path(os.fsdecode(path_driver.system_dir))


@pytest.fixture
def save_dir(path_driver: TempDirPathDriver) -> Path:
    """Return the frontend's save directory."""
    return Path(os.fsdecode(path_driver.save_dir))


@pytest.fixture
def core_system_dir(system_dir: Path) -> Path:
    """``<system>/melonDS DS``, where the core looks for BIOS and firmware images."""
    directory = system_dir / CORE_SYSTEM_SUBDIR
    directory.mkdir(parents=True, exist_ok=True)
    return directory


@pytest.fixture
def core_save_dir(save_dir: Path) -> Path:
    """``<save>/melonDS DS``, where the core writes save data."""
    directory = save_dir / CORE_SYSTEM_SUBDIR
    directory.mkdir(parents=True, exist_ok=True)
    return directory


@pytest.fixture
def wfcsettings_path(core_system_dir: Path) -> Path:
    """Where the core saves its Wi-Fi configuration when using built-in firmware."""
    return core_system_dir / "wfcsettings.bin"


@pytest.fixture
def dldi_sd_card_path(core_save_dir: Path) -> Path:
    """Return the path to the homebrew SD card image."""
    return core_save_dir / "dldi_sd_card.bin"


@pytest.fixture
def dldi_sd_card_sync_path(core_save_dir: Path) -> Path:
    """Return the directory that the homebrew SD card image is synced to."""
    return core_save_dir / "dldi_sd_card"


@pytest.fixture
def staged_system_files(
    request: pytest.FixtureRequest, core_system_dir: Path, core_save_dir: Path
) -> Mapping[str, Path]:
    """
    Copy every system file named by the test's markers into ``<system>/melonDS DS/``.

    Only *marked* files are staged.
    That selectivity is what makes the
    "boot fails because bios7.bin is absent" cases meaningful.

    :return: A mapping of environment variable name to the staged copy's path.
    """
    del core_save_dir  # Requested so it exists before the core loads, like prelude did.

    staged: dict[str, Path] = {}
    assert isinstance(request.node, pytest.Item)
    for marker in _asset_markers(request.node):
        if marker not in SYSTEM_FILE_MARKERS:
            continue

        for variable in ASSETS[marker]:
            if variable in staged:
                continue

            # pytest_runtest_setup would have skipped the test if this were None.
            source = asset_path(variable)
            assert source is not None
            target = core_system_dir / source.name
            shutil.copyfile(source, target)
            staged[variable] = target

    return staged


# --------------------------------------------------------------------------- #
# Asset fixtures (nds_rom, gba_rom, arm7_bios, ...)
# --------------------------------------------------------------------------- #


def _make_asset_fixture(variable: str) -> Any:
    """Build a session-scoped fixture returning the path CMake configured for ``variable``."""

    def fixture() -> Path:
        path = asset_path(variable)
        if path is None:
            pytest.skip(f"{variable} is not configured")

        return path

    fixture.__name__ = variable.lower()
    fixture.__doc__ = f"Path to the file CMake configured as ``{variable}``."
    return pytest.fixture(scope="session", name=variable.lower())(fixture)


for _variable in ASSET_VARIABLES:
    globals()[_variable.lower()] = _make_asset_fixture(_variable)

del _variable # type: ignore
# _variable is always bound because ASSET_VARIABLES is non-empty


# --------------------------------------------------------------------------- #
# Session factory
# --------------------------------------------------------------------------- #


@pytest.fixture
def session(
    core_path: str,
    path_driver: TempDirPathDriver,
    staged_system_files: Mapping[str, Path],  # noqa: ARG001
) -> SessionFactory:
    """
    Return a factory that builds a configured, un-entered :class:`~libretro.Session`.

    Every driver argument accepts an instance, a class, a zero-arg callable,
    or one of libretro.py's shorthands --
    a :class:`dict` becomes a ``DictOptionDriver``,
    a generator function becomes an ``IterableInputDriver``,
    a :class:`logging.Logger` becomes a log driver, and so on.
    """

    def make(game: Content | SubsystemContent | None = None, /, **kwargs: Any) -> Session:
        kwargs.setdefault("path", path_driver)
        return Session(core_path, game, **kwargs)

    return make
