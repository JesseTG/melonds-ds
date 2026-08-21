"""
Resolution of the test assets that CMake supplies through the environment.

``test/CMakeLists.txt`` passes every ROM, BIOS and firmware path
to ``pytest_discover_tests(... ENVIRONMENT ...)``,
which applies them uniformly to every generated CTest test.
None of them are set while pytest-cmake is *collecting* tests,
so nothing in this module may be evaluated at import time --
see :func:`conftest.pytest_runtest_setup`.
"""

from __future__ import annotations

import os
from collections.abc import Mapping, Sequence
from pathlib import Path

#: Marker name -> the environment variables it requires.
#:
#: For system files this is also the set staged into ``<system>/melonDS DS/``.
ASSETS: Mapping[str, tuple[str, ...]] = {
    # Content
    "nds_rom": ("NDS_ROM",),
    "dsiware_rom": ("DSIWARE_ROM",),
    "gba_rom": ("GBA_ROM",),
    "gba_sram": ("GBA_SRAM",),
    "godmode9i_rom": ("GODMODE9I_ROM",),
    "micrecord_nds": ("MICRECORD_NDS",),
    "periph_slot2_nds": ("PERIPH_SLOT2_NDS",),
    # System files, individually
    "arm7_bios": ("ARM7_BIOS",),
    "arm9_bios": ("ARM9_BIOS",),
    "arm7_dsi_bios": ("ARM7_DSI_BIOS",),
    "arm9_dsi_bios": ("ARM9_DSI_BIOS",),
    "nds_firmware": ("NDS_FIRMWARE",),
    "dsi_firmware": ("DSI_FIRMWARE",),
    "dsi_nand": ("DSI_NAND",),
    # ...and as the aggregates the CMake harness used
    "nds_sysfiles": ("ARM7_BIOS", "ARM9_BIOS", "NDS_FIRMWARE"),
    "dsi_sysfiles": (
        "ARM7_BIOS",
        "ARM9_BIOS",
        "ARM7_DSI_BIOS",
        "ARM9_DSI_BIOS",
        "DSI_FIRMWARE",
        "DSI_NAND",
    ),
}

#: Markers whose files are copied into ``<system>/melonDS DS/`` before a test runs.
SYSTEM_FILE_MARKERS = frozenset(
    name for name in ASSETS if name.endswith(("_bios", "_firmware", "_nand", "_sysfiles"))
)

#: Every environment variable named by any marker.
ASSET_VARIABLES: tuple[str, ...] = tuple(
    dict.fromkeys(variable for names in ASSETS.values() for variable in names)
)


def asset_path(variable: str) -> Path | None:
    """
    Return the path CMake configured for ``variable``, or :obj:`None`.

    CMake writes ``<VAR>-NOTFOUND`` into the cache for assets it couldn't find,
    and pytest-cmake's ``ENVIRONMENT`` is uniform across every generated test.
    An unconfigured asset therefore shows up here,
    rather than as a missing ``REQUIRED_FILES`` entry the way it did under the old harness.
    """
    value = os.environ.get(variable)
    if not value or value.endswith("-NOTFOUND"):
        return None

    path = Path(value)
    return path if path.is_file() else None


def required_asset(variable: str) -> Path:
    """
    Like :func:`asset_path`, but for assets a marker has already vouched for.

    :raises AssertionError: If the asset isn't available,
        which means the calling test is missing the marker that would have skipped it.
    """
    path = asset_path(variable)
    assert path is not None
    return path


def missing_assets(variables: Sequence[str]) -> tuple[str, ...]:
    """Return the entries of ``variables`` that don't resolve to a real file."""
    return tuple(variable for variable in variables if asset_path(variable) is None)
