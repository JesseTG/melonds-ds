"""
Learn with Pokémon: Typing Adventure and its Bluetooth keyboard.

melonDS emulates the keyboard and its link to the cart.
The core presses the keyboard's keys for the frontend's keyboard callback,
which it registers only for this game.
The game's own driver decodes the keyboard's reports into its key queue in main RAM,
which is what these tests read.

All of these use the European release.
The RAM addresses and frame counts below belong to that build.
"""

from __future__ import annotations

import struct
from collections.abc import Callable, Iterator
from ctypes import c_uint16, c_uint32
from pathlib import Path

import pytest
from libretro import RETRO_MEMORY_SAVE_RAM, RETRO_MEMORY_SYSTEM_RAM, Pointer, Session
from libretro.api.input import Key, KeyboardState, KeyModifier
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.options import DIRECT_BOOT_BUILTIN

MAIN_RAM = 0x02000000

#: The game's 16-entry key queue: 16 u32 events, then the read and write indices.
KEY_QUEUE = 0x020C88B8

#: The game's keyboard driver state; +7 is set while a keyboard is connected.
DRIVER_STATE = 0x020C87D0

#: A word the game fills from the D-pad every frame: 0x40 while Up is held.
DPAD_STATE = 0x02147204

#: Long enough for the game to have read its save.
BOOT_FRAMES = 1200

#: The game searches for the keyboard once, a little after this frame.
SEARCH_FRAME = 600

#: By now the keyboard has connected, if it's going to.
CONNECTED_FRAMES = 2400

#: The title screens need a few taps before the game looks for the keyboard.
TAP_FRAMES = 1100

NO_MODIFIERS = KeyModifier(0)

#: ``uint16_t melondsds_typing_flip_letter_case(uint32_t region, uint16_t character)``.
FlipLetterCaseProbe = TypedFunctionPointer[c_uint16, [c_uint32, c_uint16]]

#: ``melonDS::PokeTypeKeyboard::Region``, in order.
EUROPE, FRANCE, GERMANY, ITALY, SPAIN, JAPAN = range(6)


def run_frames(emulator: Session, frames: int) -> None:
    for _ in range(frames):
        emulator.run()


def ram(emulator: Session) -> memoryview:
    memory = emulator.core.get_memory(RETRO_MEMORY_SYSTEM_RAM)
    assert memory is not None
    return memory


def queue_write_index(emulator: Session) -> int:
    return struct.unpack_from("<I", ram(emulator), KEY_QUEUE - MAIN_RAM + 0x44)[0]


def last_event(emulator: Session) -> int:
    """The event behind the write index: ``(mods << 24) | (HID usage << 16) | character``."""
    slot = (queue_write_index(emulator) - 1) & 0xF
    return struct.unpack_from("<I", ram(emulator), KEY_QUEUE - MAIN_RAM + slot * 4)[0]


def events_since(emulator: Session, write_index: int) -> list[int]:
    """The events queued since the write index was ``write_index``."""
    events = []
    while write_index != queue_write_index(emulator):
        events.append(struct.unpack_from("<I", ram(emulator), KEY_QUEUE - MAIN_RAM + write_index * 4)[0])
        write_index = (write_index + 1) & 0xF
    return events


def keyboard_connected(emulator: Session) -> bool:
    return ram(emulator)[DRIVER_STATE - MAIN_RAM + 7] == 1


def dpad_state(emulator: Session) -> int:
    return struct.unpack_from("<H", ram(emulator), DPAD_STATE - MAIN_RAM)[0]


def press(emulator: Session, key: Key, character: int, modifiers: KeyModifier = NO_MODIFIERS) -> None:
    """Press and release a key a frame apart, as a frontend would report them."""
    emulator.input.keyboard_event(True, key, character, modifiers)
    emulator.run()
    emulator.input.keyboard_event(False, key, 0, modifiers)
    emulator.run()


def connect(emulator: Session) -> None:
    """Get past the title screens until the game has connected to the keyboard, which it needs to hear any key."""
    run_frames(emulator, CONNECTED_FRAMES)
    assert keyboard_connected(emulator)


def title_taps() -> Iterator[Pointer]:
    """Tap the bottom screen now and then to get past the title screens."""
    frame = 0
    while True:
        yield Pointer(0, 25600, frame < TAP_FRAMES and frame % 240 < 6)
        frame += 1


def taps_then_held(held: dict[str, bool]) -> Callable[[], Iterator[Pointer | KeyboardState]]:
    """Tap past the title screens, then report the keys in ``held`` (``KeyboardState`` fields) as held or not."""

    def generate() -> Iterator[Pointer | KeyboardState]:
        frame = 0
        while True:
            if frame < TAP_FRAMES:
                yield Pointer(0, 25600, frame % 240 < 6)
            else:
                yield KeyboardState(**held)
            frame += 1

    return generate


@pytest.mark.typing_adventure_rom
def test_registers_a_keyboard_callback(session: SessionFactory, typing_adventure_rom: Path) -> None:
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN) as emulator:
        assert emulator.input.keyboard_callback is not None


@pytest.mark.nds_rom
def test_no_keyboard_callback_for_other_games(session: SessionFactory, nds_rom: Path) -> None:
    """RetroArch's "Detect" Game Focus mode turns Game Focus on for any core with a keyboard callback."""
    with session(nds_rom, options=DIRECT_BOOT_BUILTIN) as emulator:
        assert emulator.input.keyboard_callback is None


@pytest.mark.typing_adventure_rom
def test_a_bare_letter_types_its_capital(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """The game's keyboard types 'A' for a bare A and 'a' for Shift+A, the other way up from a host."""
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        connect(emulator)
        before = queue_write_index(emulator)

        press(emulator, Key.A, ord("a"))

        assert queue_write_index(emulator) == (before + 1) & 0xF
        event = last_event(emulator)
        assert event & 0xFFFF == ord("A")
        assert (event >> 16) & 0xFF == 0x04  # HID A
        assert event >> 24 == 0


@pytest.mark.typing_adventure_rom
@pytest.mark.parametrize(
    ("key", "typed", "usage"),
    [
        pytest.param(Key.A, "A", 0x04, id="shifted-letter"),
        # The game's keyboard has these where a US one does
        pytest.param(Key.Two, "@", 0x1F, id="us-at-sign"),
        # A UK keyboard has '"' on Shift+2, the game's has it on Shift+'
        pytest.param(Key.Two, '"', 0x34, id="uk-double-quote"),
        # A German keyboard has ';' on Shift+, the game's has it on its own key, unshifted
        pytest.param(Key.COMMA, ";", 0x33, id="german-semicolon"),
    ],
)
def test_a_typed_character_follows_the_hosts_layout(
    session: SessionFactory, typing_adventure_rom: Path, key: Key, typed: str, usage: int
) -> None:
    """
    Shift held on the host, the core presses whichever of the game's keys types the character,
    holding the game's Shift or not as that key needs.
    """
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        connect(emulator)
        emulator.input.keyboard_event(True, Key.LSHIFT, 0, KeyModifier.SHIFT)
        run_frames(emulator, 2)
        before = queue_write_index(emulator)

        press(emulator, key, ord(typed), KeyModifier.SHIFT)

        # Shift going up and down again for the keystroke is queued too
        typed_events = [event for event in events_since(emulator, before) if (event >> 16) & 0xFF == usage]
        # The game's own letters are the other way up
        assert [chr(event & 0xFFFF) for event in typed_events] == [typed.swapcase()]


@pytest.mark.typing_adventure_rom
@pytest.mark.parametrize(
    ("region", "typed", "sent"),
    [
        pytest.param(EUROPE, "a", "A", id="europe-letter"),
        pytest.param(EUROPE, "A", "a", id="europe-shifted-letter"),
        pytest.param(EUROPE, "1", "1", id="europe-digit"),
        pytest.param(GERMANY, "ä", "Ä", id="germany-umlaut"),
        pytest.param(GERMANY, "Ü", "ü", id="germany-shifted-umlaut"),
        pytest.param(SPAIN, "ñ", "Ñ", id="spain-n-tilde"),
        pytest.param(SPAIN, "ç", "Ç", id="spain-c-cedilla"),
        pytest.param(FRANCE, "é", "é", id="france-e-acute"),
        pytest.param(ITALY, "è", "è", id="italy-e-grave"),
        pytest.param(JAPAN, "a", "A", id="japan-letter"),
    ],
)
def test_letter_case_follows_each_builds_layout(
    session: SessionFactory, typing_adventure_rom: Path, region: int, typed: str, sent: str
) -> None:
    """
    Other builds' keyboards have letters that ASCII doesn't, like German's Ä and Spanish's Ñ,
    and type those the other way up too.
    French é and Italian è aren't letter keys on theirs, so they come through as typed.
    """
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN) as emulator:
        probe = emulator.get_proc_address("melondsds_typing_flip_letter_case", FlipLetterCaseProbe)
        assert probe is not None, "the core doesn't export melondsds_typing_flip_letter_case"
        assert chr(probe(region, ord(typed))) == sent


@pytest.mark.typing_adventure_rom
@pytest.mark.parametrize(
    ("key", "shift", "character", "usage"),
    [
        pytest.param(Key.A, False, ord("A"), 0x04, id="letter"),
        pytest.param(Key.A, True, ord("a"), 0x04, id="shifted-letter"),
        pytest.param(Key.One, False, ord("1"), 0x1E, id="digit"),
        pytest.param(Key.One, True, ord("!"), 0x1E, id="shifted-digit"),
    ],
)
def test_a_key_sent_without_its_character_types_the_games_own(
    session: SessionFactory,
    typing_adventure_rom: Path,
    key: Key,
    shift: bool,
    character: int,
    usage: int,
) -> None:
    """RetroArch on Windows, among others, sends a key's code but never its character."""
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        connect(emulator)
        modifiers = KeyModifier.SHIFT if shift else NO_MODIFIERS
        if shift:
            emulator.input.keyboard_event(True, Key.LSHIFT, 0, modifiers)
            run_frames(emulator, 2)
        before = queue_write_index(emulator)

        press(emulator, key, 0, modifiers)

        assert queue_write_index(emulator) == (before + 1) & 0xF
        event = last_event(emulator)
        assert event & 0xFFFF == character
        assert (event >> 16) & 0xFF == usage


@pytest.mark.typing_adventure_rom
def test_enter_types_the_games_enter_key(session: SessionFactory, typing_adventure_rom: Path) -> None:
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        connect(emulator)
        before = queue_write_index(emulator)

        press(emulator, Key.RETURN, 0x0D)

        assert queue_write_index(emulator) == (before + 1) & 0xF
        assert last_event(emulator) == (0x28 << 16) | 0x000D  # HID Enter, carriage return


@pytest.mark.typing_adventure_rom
def test_nothing_is_typed_until_the_keyboard_connects(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """Keys reach the game over the keyboard's link, so without one they go nowhere."""
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_auto_fn": "disabled"}
    with session(typing_adventure_rom, options=options, input=title_taps) as emulator:
        run_frames(emulator, CONNECTED_FRAMES)
        assert not keyboard_connected(emulator)
        before = queue_write_index(emulator)

        press(emulator, Key.A, ord("a"))
        press(emulator, Key.RETURN, 0x0D)

        assert queue_write_index(emulator) == before


@pytest.mark.typing_adventure_rom
def test_typing_follows_the_setting(session: SessionFactory, typing_adventure_rom: Path) -> None:
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_keyboard": "disabled"}
    with session(typing_adventure_rom, options=options, input=title_taps) as emulator:
        connect(emulator)
        before = queue_write_index(emulator)

        press(emulator, Key.A, ord("a"))
        press(emulator, Key.RETURN, 0x0D)
        assert queue_write_index(emulator) == before

        # The core applies changed settings at the start of its next frame
        emulator.options.variables["melonds_typing_keyboard"] = "enabled"
        run_frames(emulator, 1)

        press(emulator, Key.RETURN, 0x0D)
        assert queue_write_index(emulator) == (before + 1) & 0xF


@pytest.mark.typing_adventure_rom
def test_a_key_whose_release_is_lost_is_let_go(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """
    RetroArch sends no key-up when its window loses focus, but stops reporting the key as held.
    A key the game thought still held couldn't be pressed again.
    """
    held = {"a": False}
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=taps_then_held(held)) as emulator:
        connect(emulator)
        before = queue_write_index(emulator)

        held["a"] = True
        emulator.input.keyboard_event(True, Key.A, 0, NO_MODIFIERS)
        run_frames(emulator, 6)
        held["a"] = False
        run_frames(emulator, 6)

        held["a"] = True
        emulator.input.keyboard_event(True, Key.A, 0, NO_MODIFIERS)
        run_frames(emulator, 6)

        assert queue_write_index(emulator) == (before + 2) & 0xF


@pytest.mark.typing_adventure_rom
def test_a_loaded_state_keeps_the_keys_held_now(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """
    The keys held are the player's, not the state's.
    A state saved with Shift held, loaded after letting go of it, doesn't leave the game's Shift down.
    """
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        connect(emulator)
        emulator.input.keyboard_event(True, Key.LSHIFT, 0, KeyModifier.SHIFT)
        run_frames(emulator, 2)
        state = bytearray(emulator.core.serialize_size())
        assert emulator.core.serialize(state)

        emulator.input.keyboard_event(False, Key.LSHIFT, 0, NO_MODIFIERS)
        run_frames(emulator, 2)
        assert emulator.core.unserialize(state)
        run_frames(emulator, 2)
        before = queue_write_index(emulator)

        press(emulator, Key.A, 0)

        typed = [event for event in events_since(emulator, before) if (event >> 16) & 0xFF == 0x04]
        assert [chr(event & 0xFFFF) for event in typed] == ["A"]


@pytest.mark.typing_adventure_rom
def test_savestates_keep_one_size_once_the_keyboard_connects(
    session: SessionFactory, typing_adventure_rom: Path
) -> None:
    """RetroArch sizes a savestate once, before the game has connected to the keyboard."""
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        size = emulator.core.serialize_size()
        early = bytearray(size)
        emulator.run()
        assert emulator.core.serialize(early)

        run_frames(emulator, CONNECTED_FRAMES)
        assert keyboard_connected(emulator)

        late = bytearray(size)
        assert emulator.core.serialize(late)
        assert emulator.core.unserialize(early)


@pytest.mark.typing_adventure_rom
def test_the_keyboards_flash_is_the_save_file(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """The game saves to the keyboard controller's flash, which melonDS keeps in the cart's save memory."""
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN) as emulator:
        run_frames(emulator, BOOT_FRAMES)

        sram = emulator.core.get_memory(RETRO_MEMORY_SAVE_RAM)
        assert sram is not None
        assert len(sram) == 64 * 1024
        assert b"RAXT" in bytes(sram)


@pytest.mark.typing_adventure_rom
def test_the_keyboard_connects_by_itself_by_default(session: SessionFactory, typing_adventure_rom: Path) -> None:
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=title_taps) as emulator:
        run_frames(emulator, CONNECTED_FRAMES)

        assert keyboard_connected(emulator)


@pytest.mark.typing_adventure_rom
def test_without_auto_fn_the_keyboard_waits_for_fn(session: SessionFactory, typing_adventure_rom: Path) -> None:
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_auto_fn": "disabled"}
    with session(typing_adventure_rom, options=options, input=title_taps) as emulator:
        run_frames(emulator, CONNECTED_FRAMES)

        assert not keyboard_connected(emulator)


@pytest.mark.typing_adventure_rom
def test_fn_pairs_the_keyboard(session: SessionFactory, typing_adventure_rom: Path) -> None:
    """Before the game has connected, Fn makes the keyboard discoverable rather than typing."""
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_auto_fn": "disabled"}
    with session(typing_adventure_rom, options=options, input=title_taps) as emulator:
        run_frames(emulator, SEARCH_FRAME)

        press(emulator, Key.INSERT, 0)

        run_frames(emulator, CONNECTED_FRAMES - SEARCH_FRAME - 2)
        assert keyboard_connected(emulator)


@pytest.mark.typing_adventure_rom
@pytest.mark.parametrize(
    ("fn_key", "pressed", "connects"),
    [
        pytest.param("delete", Key.DELETE, True, id="delete-pairs"),
        pytest.param("delete", Key.INSERT, False, id="insert-doesnt"),
    ],
)
def test_the_fn_key_can_be_moved(
    session: SessionFactory, typing_adventure_rom: Path, fn_key: str, pressed: Key, connects: bool
) -> None:
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_auto_fn": "disabled", "melonds_typing_fn_key": fn_key}
    with session(typing_adventure_rom, options=options, input=title_taps) as emulator:
        run_frames(emulator, SEARCH_FRAME)

        press(emulator, pressed, 0)

        run_frames(emulator, CONNECTED_FRAMES - SEARCH_FRAME - 2)
        assert keyboard_connected(emulator) == connects


@pytest.mark.typing_adventure_rom
@pytest.mark.parametrize(
    "release_sent",
    [
        pytest.param(True, id="released"),
        # RetroArch sends no key-up when its window loses focus, but stops reporting the key as held
        pytest.param(False, id="release-lost"),
    ],
)
def test_arrow_keys_press_the_dpad_while_held(
    session: SessionFactory, typing_adventure_rom: Path, release_sent: bool
) -> None:
    held = {"up": False}
    options = {**DIRECT_BOOT_BUILTIN, "melonds_typing_arrows_dpad": "enabled"}
    with session(typing_adventure_rom, options=options, input=taps_then_held(held)) as emulator:
        run_frames(emulator, CONNECTED_FRAMES)

        held["up"] = True
        emulator.input.keyboard_event(True, Key.UP, 0, NO_MODIFIERS)
        run_frames(emulator, 6)
        assert dpad_state(emulator) & 0x40

        held["up"] = False
        if release_sent:
            emulator.input.keyboard_event(False, Key.UP, 0, NO_MODIFIERS)
        run_frames(emulator, 6)
        assert not dpad_state(emulator) & 0x40


@pytest.mark.typing_adventure_rom
def test_arrow_keys_leave_the_dpad_alone_by_default(session: SessionFactory, typing_adventure_rom: Path) -> None:
    held = {"up": False}
    with session(typing_adventure_rom, options=DIRECT_BOOT_BUILTIN, input=taps_then_held(held)) as emulator:
        run_frames(emulator, CONNECTED_FRAMES)

        held["up"] = True
        emulator.input.keyboard_event(True, Key.UP, 0, NO_MODIFIERS)
        run_frames(emulator, 6)
        assert not dpad_state(emulator) & 0x40
