"""
Reading the text that homebrew prints with libnds's demo console.

``consoleDemoInit()`` sets up a 32x24 text console on the sub engine's BG0,
with its tile map at map base 22 of the sub engine's background VRAM.
The low 10 bits of each map entry select a glyph from libnds's built-in font,
which starts at ASCII 32 (space).
See ``source/arm9/console.c`` in BlocksDS's libnds.
"""

from __future__ import annotations

from ctypes import c_uint16, c_uint32

from libretro import Session
from libretro.ctypes import CIntArg, TypedFunctionPointer, TypedPointer

#: ``uint32_t melondsds_read_sub_bg_vram(uint32_t offset, uint16_t* out, uint32_t count)``.
ReadSubBgVramProbe = TypedFunctionPointer[c_uint32, [CIntArg[c_uint32], TypedPointer[c_uint16], CIntArg[c_uint32]]]

#: The demo console's size, in characters.
CONSOLE_WIDTH = 32
CONSOLE_HEIGHT = 24

#: Where ``consoleDemoInit()`` puts the console's tile map:
#: map base 22, in units of 2 KiB.
CONSOLE_MAP_OFFSET = 22 * 0x800

#: The ASCII code of the font's first glyph.
FONT_FIRST_CHAR = 32

#: How many glyphs the font has.
FONT_LENGTH = 96

#: Selects the tile index from a map entry, leaving out the flip and palette bits.
TILE_INDEX_MASK = 0x3FF


def read_demo_console(emulator: Session) -> tuple[str, ...]:
    """
    Return the text on libnds's demo console, one row per string.

    Trailing spaces are removed from each row.
    A tile that isn't in the font comes back as ``?``.
    """
    probe = emulator.get_proc_address("melondsds_read_sub_bg_vram", ReadSubBgVramProbe)
    assert probe is not None, "the core doesn't export melondsds_read_sub_bg_vram"

    count = CONSOLE_WIDTH * CONSOLE_HEIGHT
    tiles = (c_uint16 * count)()
    assert probe(CONSOLE_MAP_OFFSET, tiles, count) == count

    def glyph(entry: int) -> str:
        tile = entry & TILE_INDEX_MASK
        return chr(FONT_FIRST_CHAR + tile) if tile < FONT_LENGTH else "?"

    return tuple(
        "".join(
            glyph(entry) for entry in tiles[row * CONSOLE_WIDTH : (row + 1) * CONSOLE_WIDTH]
        ).rstrip()
        for row in range(CONSOLE_HEIGHT)
    )


__all__ = ["read_demo_console"]
