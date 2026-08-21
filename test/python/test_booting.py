"""
Every supported (and unsupported) combination of console mode and system files.

Some cases that test negatives are strict xfails here --
the body is identical,
and the marker says whether it's expected to get through it.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import pytest

from melondsds import SessionFactory
from melondsds.options import system_option_path

FRAMES = 300

xfail = pytest.mark.xfail(strict=True, reason="this configuration is expected to fail to boot")

# Marker bundles matching the sysfile flags the old add_python_test() took.
NDS_BIOS = [pytest.mark.arm7_bios, pytest.mark.arm9_bios]
DSI_BIOS = [pytest.mark.arm7_dsi_bios, pytest.mark.arm9_dsi_bios]


@dataclass(frozen=True, kw_only=True)
class BootCase:
    """
    One boot configuration.

    :param content: Name of the content fixture to load,
        or :obj:`None` to boot to the console's own menu.
    :param options: Core options that don't depend on a staged system file.
    :param system_paths: Core option key ->
        the environment variable naming the system file it should point at.
        Resolved at run time,
        because the environment isn't populated while pytest-cmake is collecting tests.
    """

    content: str | None = None
    options: dict[str, str] = field(default_factory=dict[str, str])
    system_paths: dict[str, str] = field(default_factory=dict[str, str])

    def resolve(self) -> dict[str, str]:
        """Return the complete option set, with system file paths filled in."""
        return {
            **self.options,
            **{key: system_option_path(var) for key, var in self.system_paths.items()},
        }


DS = {"melonds_console_mode": "ds"}
DSI = {"melonds_console_mode": "dsi"}
DIRECT = {"melonds_boot_mode": "direct"}
NATIVE_FILES = {"melonds_sysfile_mode": "native"}
BUILTIN_FILES = {"melonds_sysfile_mode": "builtin"}


@pytest.mark.parametrize(
    "case",
    [
        # --- Direct boot of an NDS game ---------------------------------------
        pytest.param(
            BootCase(content="nds_rom", options={**DIRECT, **DS, **BUILTIN_FILES}),
            id="direct-nds-builtin",
            marks=pytest.mark.nds_rom,
        ),
        pytest.param(
            BootCase(
                content="nds_rom",
                options={**DIRECT, **DS, **NATIVE_FILES},
                system_paths={"melonds_firmware_nds_path": "NDS_FIRMWARE"},
            ),
            id="direct-nds-native",
            marks=[pytest.mark.nds_rom, pytest.mark.nds_sysfiles],
        ),
        pytest.param(
            BootCase(
                content="nds_rom",
                options={**DIRECT, **DS, **NATIVE_FILES},
                # DSi firmware isn't bootable on a DS, but direct boot doesn't need it to be.
                system_paths={"melonds_firmware_nds_path": "DSI_FIRMWARE"},
            ),
            id="direct-nds-native-bios-nonbootable-fw",
            marks=[pytest.mark.nds_rom, *NDS_BIOS, pytest.mark.dsi_firmware],
        ),
        # --- Booting to the DS menu -------------------------------------------
        pytest.param(
            BootCase(
                options={**DS, **NATIVE_FILES, "melonds_boot_mode": "native"},
                system_paths={"melonds_firmware_nds_path": "NDS_FIRMWARE"},
            ),
            id="nds-menu-native",
            marks=pytest.mark.nds_sysfiles,
        ),
        pytest.param(
            BootCase(options={**DS, **BUILTIN_FILES}),
            id="nds-menu-builtin",
            marks=xfail,
        ),
        pytest.param(
            BootCase(
                options={**DS, **BUILTIN_FILES},
                system_paths={"melonds_firmware_nds_path": "DSI_FIRMWARE"},
            ),
            id="nds-menu-nonbootable-fw",
            marks=[xfail, *NDS_BIOS, pytest.mark.dsi_firmware],
        ),
        # --- Booting to the DSi menu ------------------------------------------
        pytest.param(
            BootCase(
                options={**DSI, "melonds_dsi_nand_path": "/notfound"},
                system_paths={"melonds_firmware_dsi_path": "DSI_FIRMWARE"},
            ),
            id="dsi-menu-no-nand",
            marks=[xfail, *NDS_BIOS, *DSI_BIOS, pytest.mark.dsi_firmware],
        ),
        pytest.param(
            BootCase(
                options=DSI,
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="dsi-menu-no-nds-bios",
            marks=[xfail, *DSI_BIOS, pytest.mark.dsi_firmware, pytest.mark.dsi_nand],
        ),
        pytest.param(
            BootCase(
                options=DSI,
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="dsi-menu-no-dsi-bios",
            marks=[xfail, *NDS_BIOS, pytest.mark.dsi_firmware, pytest.mark.dsi_nand],
        ),
        pytest.param(
            BootCase(
                options=DSI,
                system_paths={
                    # DS firmware on a DSi is the wrong image entirely.
                    "melonds_firmware_dsi_path": "NDS_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="dsi-menu-nds-firmware",
            marks=[
                xfail,
                *NDS_BIOS,
                *DSI_BIOS,
                pytest.mark.nds_firmware,
                pytest.mark.dsi_nand,
            ],
        ),
        pytest.param(
            BootCase(options=DSI, system_paths={"melonds_dsi_nand_path": "DSI_NAND"}),
            id="dsi-menu-no-firmware",
            marks=[xfail, *NDS_BIOS, *DSI_BIOS, pytest.mark.dsi_nand],
        ),
        pytest.param(
            BootCase(
                options=DSI,
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="dsi-menu-all-sysfiles",
            marks=[*NDS_BIOS, *DSI_BIOS, pytest.mark.dsi_firmware, pytest.mark.dsi_nand],
        ),
        # --- Direct DSi boot into a game --------------------------------------
        pytest.param(
            BootCase(
                content="nds_rom",
                options={**DSI, **DIRECT, "melonds_dsi_nand_path": "/notfound"},
                system_paths={"melonds_firmware_dsi_path": "DSI_FIRMWARE"},
            ),
            id="direct-dsi-nds-no-nand",
            marks=[xfail, pytest.mark.nds_rom, *NDS_BIOS, *DSI_BIOS, pytest.mark.dsi_firmware],
        ),
        pytest.param(
            BootCase(
                content="dsiware_rom",
                options={**DSI, **DIRECT, "melonds_dsi_nand_path": "/notfound"},
                system_paths={"melonds_firmware_dsi_path": "DSI_FIRMWARE"},
            ),
            id="direct-dsi-dsiware-no-nand",
            marks=[
                xfail,
                pytest.mark.dsiware_rom,
                *NDS_BIOS,
                *DSI_BIOS,
                pytest.mark.dsi_firmware,
            ],
        ),
        pytest.param(
            BootCase(
                content="nds_rom",
                options={**DSI, **DIRECT},
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-nds-no-nds-bios",
            marks=[
                xfail,
                pytest.mark.nds_rom,
                *DSI_BIOS,
                pytest.mark.dsi_firmware,
                pytest.mark.dsi_nand,
            ],
        ),
        pytest.param(
            BootCase(
                content="dsiware_rom",
                options={**DSI, **DIRECT},
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-dsiware-no-nds-bios",
            marks=[
                xfail,
                pytest.mark.dsiware_rom,
                *DSI_BIOS,
                pytest.mark.dsi_firmware,
                pytest.mark.dsi_nand,
            ],
        ),
        pytest.param(
            BootCase(
                content="nds_rom",
                options={**DSI, **DIRECT},
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-nds-no-dsi-bios",
            marks=[
                xfail,
                pytest.mark.nds_rom,
                *NDS_BIOS,
                pytest.mark.dsi_firmware,
                pytest.mark.dsi_nand,
            ],
        ),
        pytest.param(
            BootCase(
                content="dsiware_rom",
                options={**DSI, **DIRECT},
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-dsiware-no-dsi-bios",
            marks=[
                xfail,
                pytest.mark.dsiware_rom,
                *NDS_BIOS,
                pytest.mark.dsi_firmware,
                pytest.mark.dsi_nand,
            ],
        ),
        pytest.param(
            BootCase(
                content="nds_rom",
                options=DSI,
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-nds-all-sysfiles",
            marks=[pytest.mark.nds_rom, pytest.mark.dsi_sysfiles],
        ),
        pytest.param(
            BootCase(
                content="dsiware_rom",
                options=DSI,
                system_paths={
                    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
                    "melonds_dsi_nand_path": "DSI_NAND",
                },
            ),
            id="direct-dsi-dsiware-all-sysfiles",
            marks=[pytest.mark.dsiware_rom, pytest.mark.dsi_sysfiles],
        ),
    ],
)
def test_boots(session: SessionFactory, case: BootCase, request: pytest.FixtureRequest) -> None:
    """The core loads this configuration and runs for a while without raising."""
    content: Path | None = (
        request.getfixturevalue(case.content) if case.content is not None else None
    )

    with session(content, options=case.resolve()) as emulator:
        for _ in range(FRAMES):
            emulator.run()
