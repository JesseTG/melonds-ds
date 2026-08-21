"""Loading, running and unloading the core."""

from __future__ import annotations

from ctypes import c_size_t, c_uint8
from itertools import repeat
from pathlib import Path

import pytest
from libretro import Core, CoreShutDownException, DeviceIdJoypad, Region, SubsystemContent
from libretro.ctypes import TypedFunctionPointer, TypedPointer

from melondsds import SessionFactory

# --------------------------------------------------------------------------- #
# The bare core, without a session around it
# --------------------------------------------------------------------------- #


def test_core_loads(core_path: str) -> None:
    """
    The core loads and exposes every required ``retro_*`` symbol.

    :class:`~libretro.Core` binds each one on construction
    and raises :class:`ValueError` if any is missing,
    so constructing it is the whole test.
    """
    assert Core(core_path) is not None


def test_sets_callbacks(core_path: str) -> None:
    """
    The core accepts all six frontend callbacks.

    There's nothing to assert:
    each ``retro_set_*`` returns ``void``,
    so the test passes if none of them raise.
    """
    core = Core(core_path)

    core.set_video_refresh(lambda _data, _width, _height, _pitch: None)
    core.set_audio_sample(lambda _left, _right: None)
    core.set_audio_sample_batch(lambda _data, _frames: 0)
    core.set_input_poll(lambda: None)
    core.set_input_state(lambda _port, _device, _index, _id: 0)
    core.set_environment(lambda _cmd, _data: False)


def test_api_version(core_path: str) -> None:
    """``retro_api_version`` reports 1."""
    core = Core(core_path)

    assert core.api_version() == 1


def test_system_info(core_path: str) -> None:
    """``retro_get_system_info`` identifies the core as melonDS DS."""
    core = Core(core_path)
    system_info = core.get_system_info()

    assert system_info is not None
    assert system_info.library_name == b"melonDS DS"


def test_region(core_path: str) -> None:
    """``retro_get_region`` reports NTSC."""
    core = Core(core_path)

    assert core.get_region() == Region.NTSC


# --------------------------------------------------------------------------- #
# Loading and unloading
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_init_deinit(session: SessionFactory) -> None:
    """
    The core survives ``retro_init`` followed by ``retro_deinit`` with no content.

    ``content=None`` disables the content driver entirely.
    It's not the same as passing no game:
    the core is never asked to load anything,
    not even the DS menu.
    """
    with session(content=None):
        pass


@pytest.mark.nds_rom
def test_loads_unloads_with_content(session: SessionFactory, nds_rom: Path) -> None:
    """The core loads an NDS ROM and unloads it again."""
    with session(nds_rom):
        pass

@pytest.mark.dsiware_rom
@pytest.mark.dsi_firmware
@pytest.mark.no_skip_error_screen
def test_unloads_dsiware_from_error_screen(session: SessionFactory, dsiware_rom: Path) -> None:
    """
    The core unloads normally if the core shows an error screen
    when trying to play DSiWare.
    """
    with session(dsiware_rom) as emulator:
        for _ in range(10):
            emulator.run()

@pytest.mark.parametrize(
    "content_fixture",
    [
        # The core's own shared library is definitely not a valid ROM.
        pytest.param("core_path", id="core-binary"),
        pytest.param("gba_sram", id="gba-sram", marks=pytest.mark.gba_sram),
    ],
)
@pytest.mark.xfail(strict=True, reason="loading invalid content must fail, not crash")
def test_load_invalid_content(
    session: SessionFactory, content_fixture: str, request: pytest.FixtureRequest
) -> None:
    """Loading a file that isn't a ROM fails cleanly instead of crashing."""
    content = Path(request.getfixturevalue(content_fixture))

    with session(content):
        pass


@pytest.mark.nds_rom
def test_system_av_info(session: SessionFactory, nds_rom: Path) -> None:
    """``retro_get_system_av_info`` reports a usable sample rate and geometry."""
    with session(nds_rom) as emulator:
        av_info = emulator.core.get_system_av_info()

        assert av_info is not None
        assert av_info.timing.sample_rate != 0
        assert av_info.geometry.base_width != 0


# --------------------------------------------------------------------------- #
# Running frames
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
@pytest.mark.parametrize("frames", [1, 300], ids=["one-frame", "many-frames"])
def test_runs_frames(session: SessionFactory, nds_rom: Path, frames: int) -> None:
    """The core runs for a while without raising."""
    with session(nds_rom) as emulator:
        for _ in range(frames):
            emulator.run()


@pytest.mark.nds_sysfiles
def test_loads_unloads_without_content(session: SessionFactory) -> None:
    """
    The core boots to the DS menu with no content loaded.

    This needs bootable native firmware,
    since there's no ROM to boot directly into.
    """
    with session() as emulator:
        for _ in range(10):
            emulator.run()


@pytest.mark.nds_sysfiles
def test_can_shut_down(session: SessionFactory) -> None:
    """
    Exiting the DS options menu shuts the console down, and the core reports it.

    ``Session.__exit__`` swallows :class:`~libretro.CoreShutDownException`,
    so the assertions after the ``with`` block are the ones that matter.
    """

    def generate_input():
        # Wait for intro to finish
        yield from repeat(None, 240)

        # Go to NDS main menu and wait for it to appear
        yield DeviceIdJoypad.A
        yield from repeat(None, 59)

        # Select the options menu and wait for the cursor to move
        yield DeviceIdJoypad.DOWN
        yield from repeat(None, 29)

        # Go to the options menu and wait for it to appear
        yield DeviceIdJoypad.A
        yield from repeat(None, 179)

        # Exit the options menu and wait for the window to appear
        yield DeviceIdJoypad.B
        yield from repeat(None, 29)

        # Confirm the exit (exiting the NDS options menu shuts down the console)
        yield DeviceIdJoypad.A
        yield from repeat(None)

    with session(input=generate_input) as emulator:
        for _ in range(600):
            emulator.run()

        pytest.fail("The core should have raised CoreShutDownException and exited the context")

    assert emulator.is_shutdown
    assert emulator.is_exited

    with pytest.raises(CoreShutDownException):
        emulator.run()


@pytest.mark.nds_rom
@pytest.mark.gba_rom
@pytest.mark.gba_sram
def test_loads_subsystem_content(
    session: SessionFactory, nds_rom: Path, gba_rom: Path, gba_sram: Path
) -> None:
    """The ``gba`` subsystem loads an NDS ROM alongside a GBA ROM and its save data."""
    content = SubsystemContent("gba", (nds_rom, gba_rom, gba_sram))

    with session(content) as emulator:
        subsystems = emulator.subsystems
        assert subsystems is not None
        assert b"gba" in [s.ident for s in subsystems]

        proc_address_callback = emulator.proc_address_callback
        assert proc_address_callback is not None
        assert proc_address_callback.get_proc_address is not None

        gba_rom_length = emulator.get_proc_address(
            b"melondsds_gba_rom_length", TypedFunctionPointer[c_size_t, []]
        )
        assert gba_rom_length is not None
        assert gba_rom_length() > 0

        gba_rom_data = emulator.get_proc_address(
            b"melondsds_gba_rom", TypedFunctionPointer[TypedPointer[c_uint8], []]
        )
        assert gba_rom_data is not None
        assert gba_rom_data()

        gba_sram_length = emulator.get_proc_address(
            b"melondsds_gba_sram_length", TypedFunctionPointer[c_size_t, []]
        )
        assert gba_sram_length is not None
        assert gba_sram_length() > 0

        gba_sram_data = emulator.get_proc_address(
            b"melondsds_gba_sram", TypedFunctionPointer[TypedPointer[c_uint8], []]
        )
        assert gba_sram_data is not None
        assert gba_sram_data()
