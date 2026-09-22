"""
The memory map: where the frontend can reach the console's RAM by address.

Every case here loads ``test/nds/tests_sys_mem_regions.nds``,
BlocksDS's memory region test.
It puts one function in ITCM and two variables in DTCM,
then prints their addresses and values to the bottom screen,
which :func:`melondsds.libnds.read_demo_console` reads back.
That gives each test an oracle:
the ROM says where it put something,
and the map has to lead to the same bytes.

DTCM is the interesting one.
It has no fixed place in the DS's address space --
the ARM9 puts it wherever CP15 says, which is usually inside main RAM's window --
so the map exposes it at :data:`DTCM_ADDRESS`,
a pseudo-address that RetroAchievements picked for this purpose.
Reads at DTCM's *real* address reach the main RAM underneath it instead.
"""

from __future__ import annotations

import re
from collections.abc import Callable
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pytest
from libretro import (
    RETRO_MEMORY_SYSTEM_RAM,
    Content,
    MemoryDescriptorFlag,
    Session,
    SubsystemContent,
    retro_memory_descriptor,
)

from melondsds import SessionFactory
from melondsds.libnds import read_demo_console
from melondsds.options import system_option_path

pytestmark = pytest.mark.mem_regions_nds

#: Where main RAM starts, and how much of it each console has.
MAIN_RAM_ADDRESS = 0x02000000
DS_MAIN_RAM_SIZE = 0x400000
DSI_MAIN_RAM_SIZE = 0x1000000

#: Where the 16 MiB window that main RAM mirrors through ends.
MAIN_RAM_WINDOW_END = 0x03000000

#: Where the ARM9's ITCM starts, and how much of it there is.
#: CP15 fixes the base at 0; the firmware gives it a 32 MiB virtual size,
#: so the 32 KiB repeats through 0x01FFFFFF.
ITCM_ADDRESS = 0x00000000
ITCM_SIZE = 0x8000

#: Where the ARM9's DTCM is exposed, and how much of it there is.
#: Nothing lives here on real hardware;
#: RetroAchievements picked the address so that cores have somewhere to put it.
DTCM_ADDRESS = 0x0E000000
DTCM_SIZE = 0x4000

#: Where BlocksDS links DTCM, in DS and DSi mode alike.
#: From ``sys/crts/ds_arm9.mem`` in BlocksDS.
BLOCKSDS_DTCM_ADDRESS = 0x02FF4000

#: ``test_function`` as GCC compiles it at ``-O2``:
#: Thumb ``adds r0, r0, r1`` then ``bx lr``.
TEST_FUNCTION_CODE = b"\x40\x18\x70\x47"

#: What the ROM puts in its DTCM ``.data`` variable, little-endian.
DTCM_DATA_VALUE = (12345).to_bytes(4, "little")

#: The longest a test waits for the ROM to print its addresses.
MAX_FRAMES = 60

#: The system files a DSi boot needs, as :func:`system_option_path` variables.
DSI_PATHS = {
    "melonds_firmware_dsi_path": "DSI_FIRMWARE",
    "melonds_dsi_nand_path": "DSI_NAND",
}

DSI_OPTIONS = {"melonds_console_mode": "dsi", "melonds_boot_mode": "direct"}


@pytest.fixture
def session(session: SessionFactory) -> SessionFactory:
    """
    Boot straight into the ROM on a DS unless a test says otherwise.

    ``tests_sys_mem_regions.nds`` is built with BlocksDS,
    which stamps its ROMs with a DSiWare title ID
    that sends them to a DSi in "auto" console mode.
    See ``test_console_mode.py`` for the details.
    """

    def make(game: Content | SubsystemContent | None = None, /, **kwargs: Any) -> Session:
        kwargs["options"] = {
            "melonds_console_mode": "ds",
            "melonds_boot_mode": "direct",
            "melonds_sysfile_mode": "builtin",
            **kwargs.get("options", {}),
        }
        return session(game, **kwargs)

    return make


# --------------------------------------------------------------------------- #
# Reaching the core's memory the way a frontend does
# --------------------------------------------------------------------------- #


def locate(emulator: Session, address: int) -> tuple[retro_memory_descriptor, int]:
    """Return the descriptor covering ``address`` and the offset of ``address`` within it."""
    maps = emulator.memory_maps
    assert maps is not None, "the core never registered a memory map"

    found = maps.find(address)
    if found is None:
        pytest.fail(f"{address:#010x} isn't in the core's memory map")

    return found


def window(emulator: Session, address: int, size: int) -> memoryview:
    """Return a :class:`memoryview` of ``size`` bytes of the console's memory at ``address``."""
    desc, offset = locate(emulator, address)
    view = desc.view
    assert view is not None, f"{address:#010x} maps to a descriptor with no data"

    end = offset + size
    assert end <= len(view), f"{size} bytes from {address:#010x} run past the end of its region"
    return view[offset:end]


def peek(emulator: Session, address: int, size: int) -> bytes:
    """Read ``size`` bytes of the console's memory, starting at ``address``."""
    return bytes(window(emulator, address, size))


def poke(emulator: Session, address: int, data: bytes) -> None:
    """Write ``data`` into the console's memory at ``address``."""
    window(emulator, address, len(data))[:] = data


# --------------------------------------------------------------------------- #
# What the ROM says about itself
# --------------------------------------------------------------------------- #


@dataclass(frozen=True)
class Readout:
    """The addresses that ``tests_sys_mem_regions.nds`` prints on the bottom screen."""

    #: The address of ``test_function``, which is linked into ITCM.
    #: It's built as Thumb code, so the ROM prints it with bit 0 set.
    function: int

    #: The address of the ROM's DTCM ``.data`` variable, which holds 12345.
    data_var: int

    #: The address of the ROM's DTCM ``.bss`` variable, which holds 0.
    bss_var: int

    @classmethod
    def parse(cls, screen: tuple[str, ...]) -> Readout | None:
        """Parse the bottom screen, or return ``None`` if the ROM hasn't finished printing it."""
        text = "\n".join(screen)
        function = re.search(r"^Func address: 0x([0-9A-F]{8})$", text, re.MULTILINE)
        variables = re.findall(r"^Address: 0x([0-9A-F]{8})$", text, re.MULTILINE)

        if not (function and len(variables) == 2):
            return None

        return cls(int(function[1], 16), int(variables[0], 16), int(variables[1], 16))

    @property
    def function_entry(self) -> int:
        """The address of ``test_function``'s first instruction, without the Thumb bit."""
        return self.function & ~1


def run_until(emulator: Session, predicate: Callable[[Readout], bool] = lambda _: True) -> Readout:
    """
    Run frames until the ROM's readout satisfies ``predicate``, then return that readout.

    Fails the test with the ROM's screen if that doesn't happen within :data:`MAX_FRAMES`.
    """
    screen: tuple[str, ...] = ()
    for _ in range(MAX_FRAMES):
        emulator.run()
        screen = read_demo_console(emulator)
        readout = Readout.parse(screen)
        if readout is not None and predicate(readout):
            return readout

    pytest.fail("The ROM never printed its addresses. Its screen says:\n" + "\n".join(screen))


# --------------------------------------------------------------------------- #
# The shape of the map
# --------------------------------------------------------------------------- #


def test_map_is_registered_when_the_game_loads(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """The core sends its memory map before the frontend runs a single frame."""
    with session(mem_regions_nds) as emulator:
        maps = emulator.memory_maps
        assert maps is not None
        assert len(maps) == 3


def test_map_describes_main_ram_itcm_and_dtcm(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """Each descriptor names the region, size and address that the frontend expects."""
    with session(mem_regions_nds) as emulator:
        maps = emulator.memory_maps
        assert maps is not None

        main_ram, itcm, dtcm = maps[0], maps[1], maps[2]

        assert main_ram.start == MAIN_RAM_ADDRESS
        assert main_ram.len == DS_MAIN_RAM_SIZE

        assert itcm.start == ITCM_ADDRESS
        assert itcm.len == ITCM_SIZE

        assert dtcm.start == DTCM_ADDRESS
        assert dtcm.len == DTCM_SIZE

        for desc in (main_ram, itcm, dtcm):
            assert desc.flags & MemoryDescriptorFlag.SYSTEM_RAM
            assert not desc.flags & MemoryDescriptorFlag.CONST
            assert desc.ptr is not None
            assert desc.offset == 0


def test_main_ram_descriptor_is_the_system_ram_buffer(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """The main RAM descriptor points at the same bytes as ``retro_get_memory_data``."""
    with session(mem_regions_nds) as emulator:
        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        desc, offset = locate(emulator, MAIN_RAM_ADDRESS)
        assert offset == 0

        view = desc.view
        assert view is not None
        assert len(view) == len(memory)

        # Same length isn't the same buffer, so write through one and read the other
        view[0:4] = b"\xde\xad\xbe\xef"
        assert memory[0:4].tobytes() == b"\xde\xad\xbe\xef"


# --------------------------------------------------------------------------- #
# Main RAM
# --------------------------------------------------------------------------- #


def test_main_ram_mirrors_across_its_window(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """A DS's 4 MiB of main RAM answers four times over, as melonDS emulates it."""
    marker = b"\x01\x02\x03\x04"

    with session(mem_regions_nds) as emulator:
        run_until(emulator)
        poke(emulator, MAIN_RAM_ADDRESS, marker)

        mirrors = range(MAIN_RAM_ADDRESS, MAIN_RAM_WINDOW_END, DS_MAIN_RAM_SIZE)
        assert [peek(emulator, address, len(marker)) for address in mirrors] == [marker] * 4


def test_main_ram_window_ends_where_the_map_says(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """Addresses above main RAM's window belong to hardware the map doesn't describe."""
    with session(mem_regions_nds) as emulator:
        maps = emulator.memory_maps
        assert maps is not None

        assert maps.find(MAIN_RAM_WINDOW_END - 1) is not None
        assert maps.find(MAIN_RAM_WINDOW_END) is None


# --------------------------------------------------------------------------- #
# DTCM, the region that #301 is about
# --------------------------------------------------------------------------- #


def test_dtcm_data_variable_is_readable(session: SessionFactory, mem_regions_nds: Path) -> None:
    """The ROM's DTCM variable holds 12345, and the pseudo-address leads to it."""
    with session(mem_regions_nds) as emulator:
        readout = run_until(emulator)

        offset = readout.data_var - BLOCKSDS_DTCM_ADDRESS
        assert 0 <= offset < DTCM_SIZE, f"{readout.data_var:#010x} isn't in BlocksDS's DTCM"

        assert peek(emulator, DTCM_ADDRESS + offset, 4) == DTCM_DATA_VALUE


def test_dtcms_real_address_reaches_main_ram_instead(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """
    DTCM's real address falls through to the main RAM behind it.

    The ARM9 overlays DTCM onto whatever the CP15 base register points at,
    which the map has no way to follow,
    so main RAM keeps that part of the address space.
    This is why #301 needs the pseudo-address.
    """
    with session(mem_regions_nds) as emulator:
        readout = run_until(emulator)

        assert peek(emulator, readout.data_var, 4) != DTCM_DATA_VALUE

        desc, _ = locate(emulator, readout.data_var)
        main_ram, _ = locate(emulator, MAIN_RAM_ADDRESS)
        assert desc.ptr is not None and main_ram.ptr is not None
        assert desc.ptr.value == main_ram.ptr.value


def test_dtcm_bss_variable_is_writable(session: SessionFactory, mem_regions_nds: Path) -> None:
    """The ROM's zeroed DTCM variable reads back as 0, and a write to it sticks."""
    marker = b"\xca\xfe\xba\xbe"

    with session(mem_regions_nds) as emulator:
        readout = run_until(emulator)
        address = DTCM_ADDRESS + (readout.bss_var - BLOCKSDS_DTCM_ADDRESS)

        assert peek(emulator, address, 4) == bytes(4)

        poke(emulator, address, marker)
        assert peek(emulator, address, 4) == marker


def test_dtcm_is_exactly_16_kib(session: SessionFactory, mem_regions_nds: Path) -> None:
    """DTCM covers its declared length and not one byte more."""
    with session(mem_regions_nds) as emulator:
        maps = emulator.memory_maps
        assert maps is not None

        found = maps.find(DTCM_ADDRESS + DTCM_SIZE - 4)
        assert found is not None
        assert found[1] == DTCM_SIZE - 4

        assert maps.find(DTCM_ADDRESS + DTCM_SIZE) is None
        assert maps.find(DTCM_ADDRESS - 1) is None


# --------------------------------------------------------------------------- #
# ITCM
# --------------------------------------------------------------------------- #


def test_itcm_holds_the_roms_code(session: SessionFactory, mem_regions_nds: Path) -> None:
    """The function the ROM linked into ITCM is where the ROM says it is."""
    with session(mem_regions_nds) as emulator:
        readout = run_until(emulator)
        assert peek(emulator, readout.function_entry, 4) == TEST_FUNCTION_CODE


def test_itcm_mirrors_down_to_address_zero(
    session: SessionFactory, mem_regions_nds: Path
) -> None:
    """
    ITCM's 32 KiB repeats through the 32 MiB the firmware gives it.

    libnds links ITCM code at 0x01000000 and relies on that,
    so the same bytes have to answer at the physical base as well.
    """
    with session(mem_regions_nds) as emulator:
        readout = run_until(emulator)
        offset = readout.function_entry % ITCM_SIZE

        assert peek(emulator, ITCM_ADDRESS + offset, 4) == TEST_FUNCTION_CODE


# --------------------------------------------------------------------------- #
# The map follows the console
# --------------------------------------------------------------------------- #


def test_reset_sends_a_fresh_map(session: SessionFactory, mem_regions_nds: Path) -> None:
    """
    Resetting rebuilds the console, so the frontend gets the new buffers' addresses.

    melonDS DS throws out the whole ``NDS`` object on reset,
    which leaves every pointer in the old map dangling.
    """
    with session(mem_regions_nds) as emulator:
        run_until(emulator)

        emulator.reset()
        emulator.run()

        memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
        assert memory is not None

        desc, _ = locate(emulator, MAIN_RAM_ADDRESS)
        view = desc.view
        assert view is not None

        view[0:4] = b"\xde\xad\xbe\xef"
        assert memory[0:4].tobytes() == b"\xde\xad\xbe\xef"

        # The ROM runs again on the fresh console, and its regions still resolve
        readout = run_until(emulator)
        assert peek(emulator, readout.function_entry, 4) == TEST_FUNCTION_CODE


# --------------------------------------------------------------------------- #
# DSi mode
# --------------------------------------------------------------------------- #


@pytest.mark.dsi_sysfiles
def test_dsi_main_ram_fills_its_window(session: SessionFactory, mem_regions_nds: Path) -> None:
    """A DSi has 16 MiB of main RAM, which leaves no room for mirrors."""
    options = {**DSI_OPTIONS, "melonds_sysfile_mode": "native"}
    system_paths = {key: system_option_path(value) for key, value in DSI_PATHS.items()}

    with session(mem_regions_nds, options={**options, **system_paths}) as emulator:
        maps = emulator.memory_maps
        assert maps is not None

        main_ram = maps[0]
        assert main_ram.start == MAIN_RAM_ADDRESS
        assert main_ram.len == DSI_MAIN_RAM_SIZE

        poke(emulator, MAIN_RAM_ADDRESS, b"\x00\x00\x00\x00")
        poke(emulator, MAIN_RAM_ADDRESS + DS_MAIN_RAM_SIZE, b"\x01\x02\x03\x04")
        assert peek(emulator, MAIN_RAM_ADDRESS, 4) == bytes(4)


@pytest.mark.dsi_sysfiles
def test_dsi_still_exposes_the_tcms(session: SessionFactory, mem_regions_nds: Path) -> None:
    """The ARM9's TCMs are the same size on a DSi, and sit at the same addresses."""
    options = {**DSI_OPTIONS, "melonds_sysfile_mode": "native"}
    system_paths = {key: system_option_path(value) for key, value in DSI_PATHS.items()}

    with session(mem_regions_nds, options={**options, **system_paths}) as emulator:
        readout = run_until(emulator)

        assert peek(emulator, readout.function_entry, 4) == TEST_FUNCTION_CODE

        offset = readout.data_var - BLOCKSDS_DTCM_ADDRESS
        assert peek(emulator, DTCM_ADDRESS + offset, 4) == DTCM_DATA_VALUE
