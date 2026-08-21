"""
Core option helpers.

melonDS DS options are plain ``str``-keyed mappings,
and libretro.py wraps a ``Mapping[str, str]``
in a :class:`~libretro.drivers.DictOptionDriver` for us.
The only thing that needs help
is the handful of options that name a system file
by its path *relative to the frontend's system directory*,
because that path depends on what the test staged.
"""

from __future__ import annotations

from .assets import asset_path

#: Subdirectory of the frontend's system directory that melonDS DS reads
#: BIOS images, firmware and NAND images from.
CORE_SYSTEM_SUBDIR = "melonDS DS"

def system_option_path(variable: str) -> str:
    """
    Return the value for a ``melonds_*_path`` option naming a staged system file.

    :param variable: Environment variable of the system file, e.g. ``NDS_FIRMWARE``.
    """
    path = asset_path(variable)
    assert path is not None
    return f"{CORE_SYSTEM_SUBDIR}/{path.name}"


#: Boot straight into the loaded ROM using the built-in FreeBIOS and firmware.
#: This needs no system files, so tests that use it run everywhere.
DIRECT_BOOT_BUILTIN = {
    "melonds_boot_mode": "direct",
    "melonds_console_mode": "ds",
    "melonds_sysfile_mode": "builtin",
}
