"""
Input the core accepts: buttons, the touch pointer, the analog cursor and the mic.

Migrated from the input cases in ``test/cmake/Basics.cmake``.

The button and pointer cases both work
by driving the DS system menu and comparing frames,
so they need bootable native firmware.
"""

from __future__ import annotations

import itertools
from collections.abc import Iterator
from ctypes import c_int
from math import cos, pi, sin
from pathlib import Path

import pytest
from libretro import AnalogState, GeneratorMicrophoneDriver, JoypadState, Point, Pointer
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory


@pytest.mark.nds_sysfiles
def test_button_input(session: SessionFactory) -> None:
    """Pressing A on the intro screen advances to the main menu."""

    def generate_input():
        yield from itertools.repeat(0, 240)
        yield JoypadState(a=True)
        yield from itertools.repeat(0)

    with session(input=generate_input) as emulator:
        for _ in range(240):
            emulator.run()

        frame1 = emulator.video.screenshot()
        assert frame1

        for _ in range(240):
            emulator.run()

        frame2 = emulator.video.screenshot()
        assert frame2

        # The logo screen (frame1) has a white pixel in the top left corner,
        # whereas the main menu screen doesn't.
        assert frame1.data[0:4] != frame2.data[0:4]


@pytest.mark.nds_sysfiles
def test_pointer_input(session: SessionFactory) -> None:
    """Circling and then tapping the pointer advances past the intro screen."""

    def generate_circle_points(points: int, radius: int = 1, offset: float = 0):
        for i in range(points):
            angle = (pi * 2 * i / points) + offset
            yield Point(x=int(cos(angle) * radius), y=int(sin(angle) * radius))

    def generate_input():
        circle = tuple(generate_circle_points(180, 0x6FFF, pi / 2))

        # Circle the pointer around the screen, starting from near the bottom
        yield from iter(circle)

        # Hold the pointer in place for a bit
        yield from itertools.repeat(circle[0], 120)

        # Touch the screen at the current pointer position
        yield Pointer(circle[0].x, circle[0].y, True)

        yield from itertools.repeat(circle[0])

    with session(input=generate_input, options={"melonds_show_cursor": "always"}) as emulator:
        for _ in range(180):
            emulator.run()

        frame1 = emulator.video.screenshot()
        assert frame1

        for _ in range(240):
            emulator.run()

        frame2 = emulator.video.screenshot()
        assert frame2

        # The logo screen (frame1) has a white pixel in the top left corner,
        # whereas the main menu screen doesn't.
        assert frame1.data[0:4] != frame2.data[0:4]


@pytest.mark.nds_sysfiles
def test_analog_input(session: SessionFactory) -> None:
    """Deflecting the right stick moves the analog touch cursor."""

    def generate_input():
        yield from itertools.repeat(0, 2)
        yield from itertools.repeat(AnalogState(rstick=(3000, 150)), 35)
        yield from itertools.repeat(0)

    with session(input=generate_input, options={"melonds_show_cursor": "enabled"}) as emulator:
        cursor_x = emulator.get_proc_address(b"melondsds_analog_cursor_x", TypedFunctionPointer[c_int, []])
        assert cursor_x is not None
        cursor_y = emulator.get_proc_address(b"melondsds_analog_cursor_y", TypedFunctionPointer[c_int, []])
        assert cursor_y is not None

        initial_cursor_pos = cursor_x(), cursor_y()

        for _ in range(360):
            emulator.run()

        current_cursor_pos = cursor_x(), cursor_y()

        assert initial_cursor_pos != current_cursor_pos


@pytest.mark.micrecord_nds
@pytest.mark.parametrize(
    "mic_input",
    [
        pytest.param(None, id="default"),
        pytest.param("blow", id="blow"),
    ],
)
def test_microphone_input(
    session: SessionFactory, micrecord_nds: Path, mic_input: str | None
) -> None:
    """
    A sine wave fed through the mic driver reaches the emulated microphone.

    ``micrecord.nds`` records from the microphone and plays the result back,
    so non-silent audio output means the samples made it through.
    """

    def generate_input():
        yield from itertools.repeat(None, 6)
        yield JoypadState(a=True)
        yield from itertools.repeat(None, 90)
        yield JoypadState(a=True)
        yield from itertools.repeat(None)

    def generate_sine_wave() -> Iterator[int]:
        for i in itertools.count():
            yield int(sin(i * 440) * 30000)

    options = {"melonds_boot_mode": "direct", "melonds_mic_input_active": "always"}
    if mic_input is not None:
        options["melonds_mic_input"] = mic_input

    with session(
        micrecord_nds,
        input=generate_input,
        mic=GeneratorMicrophoneDriver(generate_sine_wave),
        options=options,
    ) as emulator:
        for _ in range(300):
            emulator.run()

        # Assert that we're not just being given silence
        audio = emulator.audio
        assert audio.buffer is not None
        assert len(audio.buffer) > 0
        assert any(b != 0 for b in audio.buffer)
