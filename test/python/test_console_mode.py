"""
Console mode resolution: how "Auto", "DS" and "DSi" pick the emulated console.

melonDS DS used to force DSi mode for any DSiWare-flagged ROM,
which broke homebrew built with BlocksDS --
its ROMs carry a DSiWare title ID by default but still run on a DS.
See https://github.com/JesseTG/melonds-ds/issues/319.

A ROM is now only treated as DSiWare -- and installed onto the DSi's NAND --
if its DSi regions are modcrypted, as a retail release's are and homebrew's aren't.
Anything else runs from the cart slot,
and its unit code alone decides whether Auto picks a DS or a DSi.

``tests_sys_scfg_registers.nds`` is such a ROM,
so it's the fixture that pins the fix.
``auto-retail-dsiware`` is the guard on the other side:
it proves a genuine DSiWare title still resolves to a DSi and still lands on the NAND.
"""

from __future__ import annotations

import struct
from ctypes import c_bool, c_char_p, c_int32
from dataclasses import dataclass, field
from pathlib import Path

import pytest
from libretro import RETRO_MEMORY_SYSTEM_RAM, Session
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.options import system_option_path

#: Values reported by ``melondsds_get_console_type``.
NO_CONSOLE = -1
DS_CONSOLE = 0
DSI_CONSOLE = 1

#: Where the DS maps main RAM.
MAIN_RAM_BASE = 0x02000000

FRAMES = 300

AUTO = {"melonds_console_mode": "auto"}
DS = {"melonds_console_mode": "ds"}
DSI = {"melonds_console_mode": "dsi"}
DIRECT = {"melonds_boot_mode": "direct"}
BUILTIN_FILES = {"melonds_sysfile_mode": "builtin"}
NATIVE_FILES = {"melonds_sysfile_mode": "native"}

#: The system files a DSi boot needs, as :func:`system_option_path` variables.
DSI_PATHS = {
    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
    "melonds_dsi_nand_path": "DSI_NAND",
}


def console_type(emulator: Session) -> int:
    """Return the type of the console the core actually created."""
    probe = emulator.get_proc_address(
        "melondsds_get_console_type", TypedFunctionPointer[c_int32, []]
    )
    assert probe is not None
    return probe()


def cart_inserted(emulator: Session) -> bool:
    """Return whether a cartridge is in the emulated console's Slot-1."""
    probe = emulator.get_proc_address(
        "melondsds_nds_cart_inserted", TypedFunctionPointer[c_bool, []]
    )
    assert probe is not None
    return probe()


# --------------------------------------------------------------------------- #
# Which console does each mode resolve to?
# --------------------------------------------------------------------------- #


@dataclass(frozen=True, kw_only=True)
class ModeCase:
    """
    One console-mode configuration and the console it must produce.

    :param content: Name of the content fixture to load,
        or :obj:`None` to boot to the console's own menu.
    :param options: Core options that don't depend on a staged system file.
    :param system_paths: Core option key ->
        the environment variable naming the system file it should point at.
        Resolved at run time,
        because the environment isn't populated while pytest-cmake is collecting tests.
    :param expected: The value ``melondsds_get_console_type`` must report.
    """

    content: str | None = None
    options: dict[str, str] = field(default_factory=dict[str, str])
    system_paths: dict[str, str] = field(default_factory=dict[str, str])
    expected: int

    def resolve(self) -> dict[str, str]:
        """Return the complete option set, with system file paths filled in."""
        return {
            **self.options,
            **{key: system_option_path(var) for key, var in self.system_paths.items()},
        }


@pytest.mark.parametrize(
    "case",
    [
        # --- Auto leaves ordinary DS content alone ----------------------------
        pytest.param(
            ModeCase(
                content="nds_rom",
                options={**AUTO, **DIRECT, **BUILTIN_FILES},
                expected=DS_CONSOLE,
            ),
            id="auto-nds",
            marks=pytest.mark.nds_rom,
        ),
        pytest.param(
            # GodMode9i.nds is unit code 02h (DS+DSi), which is not DSi-only.
            ModeCase(
                content="godmode9i_rom",
                options={**AUTO, **DIRECT, **BUILTIN_FILES},
                expected=DS_CONSOLE,
            ),
            id="auto-dsi-enhanced",
            marks=pytest.mark.godmode9i_rom,
        ),
        pytest.param(
            # The #319 case.
            # BlocksDS stamps this homebrew ROM with the DSiWare title ID,
            # but its unit code is 02h and its DSi regions aren't modcrypted,
            # so Auto must leave it on a DS -- with no DSi system files at all.
            ModeCase(
                content="scfg_registers_nds",
                options={**AUTO, **DIRECT, **BUILTIN_FILES},
                expected=DS_CONSOLE,
            ),
            id="auto-dsiware-homebrew",
            marks=pytest.mark.scfg_registers_nds,
        ),
        pytest.param(
            ModeCase(
                options={**AUTO, **NATIVE_FILES, "melonds_boot_mode": "native"},
                system_paths={"melonds_firmware_nds_path": "NDS_FIRMWARE"},
                expected=DS_CONSOLE,
            ),
            id="auto-no-content",
            marks=pytest.mark.nds_sysfiles,
        ),
        # --- Auto picks DSi for content that needs one ------------------------
        # Only retail DSiWare can be checked positively here:
        # installing a title onto the NAND needs metadata from Nintendo's CDN,
        # which has nothing for a homebrew title ID.
        # test_auto_mode_avoids_ds[] covers the homebrew ROMs instead.
        pytest.param(
            ModeCase(
                content="dsiware_rom",
                options=AUTO,
                system_paths=DSI_PATHS,
                expected=DSI_CONSOLE,
            ),
            id="auto-retail-dsiware",
            marks=[pytest.mark.dsiware_rom, pytest.mark.dsi_sysfiles],
        ),
        # --- Forcing a console overrides the heuristic ------------------------
        pytest.param(
            # The #319 case: DSiWare-flagged BlocksDS homebrew, forced onto a DS.
            # Needs no DSi system files at all, so this one runs everywhere.
            ModeCase(
                content="scfg_registers_nds",
                options={**DS, **DIRECT, **BUILTIN_FILES},
                expected=DS_CONSOLE,
            ),
            id="ds-dsiware-homebrew",
            marks=pytest.mark.scfg_registers_nds,
        ),
        pytest.param(
            # Homebrew with a real game code, so NDSHeader::IsHomebrew() is false.
            # Its DSi regions aren't modcrypted, though, so DS mode must not refuse it.
            # Its secure area *is* encrypted, hence the native DS BIOS files.
            ModeCase(
                content="godmode9i_dsi_rom",
                options={**DS, **DIRECT, **NATIVE_FILES},
                system_paths={"melonds_firmware_nds_path": "NDS_FIRMWARE"},
                expected=DS_CONSOLE,
            ),
            id="ds-dsi-only-homebrew",
            marks=[pytest.mark.godmode9i_dsi_rom, pytest.mark.nds_sysfiles],
        ),
        pytest.param(
            ModeCase(
                content="nds_rom",
                options=DSI,
                system_paths=DSI_PATHS,
                expected=DSI_CONSOLE,
            ),
            id="dsi-nds",
            marks=[pytest.mark.nds_rom, pytest.mark.dsi_sysfiles],
        ),
    ],
)
def test_resolves_console(
    session: SessionFactory, case: ModeCase, request: pytest.FixtureRequest
) -> None:
    """The console mode and the loaded ROM's header pick the console together."""
    content: Path | None = (
        request.getfixturevalue(case.content) if case.content is not None else None
    )

    with session(content, options=case.resolve()) as emulator:
        assert console_type(emulator) == case.expected


# --------------------------------------------------------------------------- #
# Running DSiWare-flagged homebrew on a DS (the #319 regression)
# --------------------------------------------------------------------------- #

#: DS mode with no system files of any kind, native or DSi.
FORCED_DS = {**DS, **DIRECT, **BUILTIN_FILES}

#: The same, but reached through Auto -- which is the default,
#: so it's the path players actually take.
AUTO_DS = {**AUTO, **DIRECT, **BUILTIN_FILES}

#: Both ways of landing a DSiWare-flagged homebrew ROM on a DS.
on_a_ds = pytest.mark.parametrize(
    "options",
    [
        pytest.param(FORCED_DS, id="forced"),
        pytest.param(AUTO_DS, id="auto"),
    ],
)


def _arm9_binary(rom: Path) -> tuple[int, bytes]:
    """
    Return where a ROM's ARM9 binary belongs in main RAM, and what should be there.

    melonDS decrypts the secure area in place when the ARM9 binary starts within it,
    so the first 2 KiB of such a binary won't match the file and are skipped.
    """
    data = rom.read_bytes()
    rom_offset, _entry, ram_address, size = struct.unpack_from("<IIII", data, 0x20)
    skip = 0x800 if 0x4000 <= rom_offset < 0x8000 else 0

    return ram_address + skip, data[rom_offset + skip : rom_offset + size]


@pytest.mark.scfg_registers_nds
@on_a_ds
def test_ds_mode_boots_dsiware_homebrew(
    session: SessionFactory, scfg_registers_nds: Path, options: dict[str, str]
) -> None:
    """
    DSiWare-flagged homebrew runs on a DS, with no DSi system files.

    This is the configuration from
    `#319 <https://github.com/JesseTG/melonds-ds/issues/319>`_.
    """
    with session(scfg_registers_nds, options=options) as emulator:
        assert console_type(emulator) == DS_CONSOLE

        for _ in range(FRAMES):
            emulator.run()


@pytest.mark.scfg_registers_nds
@on_a_ds
def test_ds_mode_direct_boots_dsiware_homebrew(
    session: SessionFactory, scfg_registers_nds: Path, options: dict[str, str]
) -> None:
    """
    Direct boot actually happens once the ROM is on a DS.

    Reporting a DS console isn't enough:
    the core used to skip ``SetupDirectBoot`` for anything DSiWare-flagged,
    which left the ROM loaded but never started.
    So check that the ARM9 binary really was copied into main RAM.
    """
    address, expected = _arm9_binary(scfg_registers_nds)

    # StartConsole() runs during retro_load_game for the software renderer,
    # so direct boot has already happened by the time the session is entered --
    # and the game hasn't yet had a chance to overwrite its own binary.
    with session(scfg_registers_nds, options=options) as emulator:
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        start = address - MAIN_RAM_BASE
        assert memory[start : start + 0x100].tobytes() == expected[:0x100]


@pytest.mark.godmode9i_dsi_rom
@pytest.mark.nds_sysfiles
@pytest.mark.xfail(
    strict=True,
    reason="Auto sends a unit-code-03h ROM to a DSi, which needs DSi system files",
)
def test_auto_mode_still_needs_dsi_files(
    session: SessionFactory, godmode9i_dsi_rom: Path
) -> None:
    """
    Auto still sends DSi-only content to a DSi, which needs DSi system files.

    This is the counterpart that makes the forced-DS cases meaningful:
    without it they'd pass even if the console mode were ignored entirely.
    ``GodMode9i.dsi`` declares unit code 03h,
    so no amount of homebrew detection can keep it on a DS.

    Native DS system files are staged
    so that an encrypted secure area can't be what stops it from booting.
    """
    options = {
        **AUTO,
        **DIRECT,
        **NATIVE_FILES,
        "melonds_firmware_nds_path": system_option_path("NDS_FIRMWARE"),
    }

    with session(godmode9i_dsi_rom, options=options) as emulator:
        for _ in range(FRAMES):
            emulator.run()


@pytest.mark.scfg_registers_nds
@pytest.mark.dsi_sysfiles
def test_dsi_mode_runs_dsiware_homebrew_from_cart(
    session: SessionFactory, scfg_registers_nds: Path
) -> None:
    """
    DSiWare-flagged homebrew forced onto a DSi runs from the cart slot.

    It used to be installed onto the NAND instead,
    which failed outright because Nintendo publishes no title metadata for it.
    See https://github.com/JesseTG/melonds-ds/issues/319.
    """
    options = {
        **DSI,
        **DIRECT,
        **{key: system_option_path(var) for key, var in DSI_PATHS.items()},
    }

    with session(scfg_registers_nds, options=options) as emulator:
        assert console_type(emulator) == DSI_CONSOLE
        assert cart_inserted(emulator)


# --------------------------------------------------------------------------- #
# Forcing DS mode on a retail DSi title
# --------------------------------------------------------------------------- #


@pytest.mark.dsiware_rom
@pytest.mark.dsi_sysfiles
@pytest.mark.no_skip_error_screen
def test_ds_mode_refuses_retail_dsiware(session: SessionFactory, dsiware_rom: Path) -> None:
    """
    A retail DSiWare title forced onto a DS shows the error screen.

    The core keeps running so that the player can fix their settings,
    so ``retro_load_game`` still succeeds -- but no console is created.

    The DSi system files are staged even though DS mode was asked for,
    so that a missing NAND can't be what refuses the game.
    """
    options = {
        **FORCED_DS,
        **{key: system_option_path(var) for key, var in DSI_PATHS.items()},
    }

    with session(dsiware_rom, options=options) as emulator:
        assert console_type(emulator) == NO_CONSOLE

        for _ in range(10):
            emulator.run()


@pytest.mark.godmode9i_dsi_rom
@pytest.mark.nds_sysfiles
@pytest.mark.no_skip_error_screen
def test_ds_mode_allows_unmodcrypted_dsi_homebrew(
    session: SessionFactory, godmode9i_dsi_rom: Path
) -> None:
    """
    Homebrew isn't refused, even when it doesn't look like homebrew.

    ``GodMode9i.dsi`` is DSi-only *and* carries a real game code,
    so ``NDSHeader::IsHomebrew()`` returns false for it.
    Lacking modcrypt is what distinguishes it from a retail title.
    """
    options = {
        **DS,
        **DIRECT,
        **NATIVE_FILES,
        "melonds_firmware_nds_path": system_option_path("NDS_FIRMWARE"),
    }

    with session(godmode9i_dsi_rom, options=options) as emulator:
        assert console_type(emulator) == DS_CONSOLE


# --------------------------------------------------------------------------- #
# Telling a homebrew game code apart from a real one
# --------------------------------------------------------------------------- #

#: Game codes and whether ``melondsds_is_valid_game_code`` should accept them,
#: grouped so that the suite gets a handful of test cases
#: rather than one per code.
GAME_CODE_GROUPS = {
    "valid": ((b"KGUV", True), (b"KS3E", True), (b"HGMA", True), (b"ASME", True)),
    "placeholder": ((b"####", False), (b"", False), (b"    ", False)),
    "length": ((b"ASM", False), (b"ASMEE", False), (b"A", False)),
    "characters": ((b"asme", False), (b"AS-E", False), (b"AS.E", False)),
}


@pytest.mark.scfg_registers_nds
@pytest.mark.parametrize("group", sorted(GAME_CODE_GROUPS), ids=sorted(GAME_CODE_GROUPS))
def test_game_code_validity(
    session: SessionFactory, scfg_registers_nds: Path, group: str
) -> None:
    """
    The game code validator matches the shape GBATEK documents.

    This is half of what separates homebrew that claims a DSiWare title ID
    from a real DSiWare release; the other half is the modcrypt flag.
    A session is entered only because ``get_proc_address`` needs a loaded core --
    the probe itself doesn't touch the console.
    """
    with session(scfg_registers_nds, options=FORCED_DS) as emulator:
        probe = emulator.get_proc_address(
            "melondsds_is_valid_game_code", TypedFunctionPointer[c_bool, [c_char_p]]
        )
        assert probe is not None

        for code, expected in GAME_CODE_GROUPS[group]:
            assert probe(c_char_p(code)) == expected, code


# --------------------------------------------------------------------------- #
# Consequences of the resolved console type
# --------------------------------------------------------------------------- #


@pytest.mark.scfg_registers_nds
@on_a_ds
def test_ds_mode_enables_savestates(
    session: SessionFactory, scfg_registers_nds: Path, options: dict[str, str]
) -> None:
    """Savestates work for a DSiWare-flagged ROM once it's running on a DS."""
    with session(scfg_registers_nds, options=options) as emulator:
        emulator.run()

        assert emulator.core.serialize_size() > 0


@pytest.mark.scfg_registers_nds
@on_a_ds
def test_ds_mode_exposes_ds_sized_ram(
    session: SessionFactory, scfg_registers_nds: Path, options: dict[str, str]
) -> None:
    """A DS reports 4 MiB of main RAM, not the DSi's 16 MiB."""
    with session(scfg_registers_nds, options=options) as emulator:
        assert emulator.core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) == 4 * 1024 * 1024
