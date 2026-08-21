"""Audio and video output, screen geometry and rotation."""

from __future__ import annotations

import itertools
from ctypes import c_int
from pathlib import Path

import pytest
from libretro import JoypadState, PixelFormat, Rotation, Screenshot
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.options import DIRECT_BOOT_BUILTIN


def _press_r3_after(frames: int):
    """
    Yield nothing for ``frames`` frames, press R3 once, then idle forever.

    R3 cycles to the next screen layout.
    """

    def generate_input():
        yield from itertools.repeat(None, frames)
        yield JoypadState(r3=True)
        yield from itertools.repeat(None)

    return generate_input


# --------------------------------------------------------------------------- #
# Audio and video output
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_generates_audio(session: SessionFactory, nds_rom: Path) -> None:
    """The core produces audio that isn't just silence."""
    with session(nds_rom) as emulator:
        for _ in range(300):
            emulator.run()

        audio = emulator.audio
        assert audio.buffer is not None
        assert len(audio.buffer) > 0
        assert any(b != 0 for b in audio.buffer)


@pytest.mark.nds_rom
def test_generates_video(session: SessionFactory, nds_rom: Path) -> None:
    """Two frames 70 frames apart have the same dimensions but different contents."""
    with session(nds_rom) as emulator:
        for _ in range(70):
            emulator.run()

        frame1 = emulator.video.screenshot()
        assert isinstance(frame1, Screenshot)

        for _ in range(70):
            emulator.run()

        frame2 = emulator.video.screenshot()
        assert isinstance(frame2, Screenshot)

        assert (frame1.width, frame1.height) == (frame2.width, frame2.height)
        assert frame1.data != frame2.data


@pytest.mark.nds_rom
def test_pixel_format(session: SessionFactory, nds_rom: Path) -> None:
    """The core asks the frontend for XRGB8888."""
    with session(nds_rom) as emulator:
        assert emulator.video.pixel_format == PixelFormat.XRGB8888


# --------------------------------------------------------------------------- #
# Screen geometry
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_sets_geometry(session: SessionFactory, nds_rom: Path) -> None:
    """Switching screen layout changes the geometry the core reports."""
    options = {
        "melonds_number_of_screen_layouts": "2",
        "melonds_screen_layout1": "top-bottom",
        "melonds_screen_layout2": "left-right",
    }

    with session(nds_rom, input=_press_r3_after(10), options=options) as emulator:
        for _ in range(10):
            emulator.run()

        frame1 = emulator.video.screenshot()
        geometry1 = emulator.video.geometry

        assert frame1 is not None
        assert geometry1 is not None

        assert frame1.width == geometry1.base_width
        assert frame1.height == geometry1.base_height

        for _ in range(20):
            emulator.run()

        frame2 = emulator.video.screenshot()
        geometry2 = emulator.video.geometry

        assert frame2 is not None
        assert geometry2 is not None
        assert geometry1 != geometry2


@pytest.mark.nds_sysfiles
def test_rotates_screen(session: SessionFactory) -> None:
    """Switching to a rotated layout rotates the frame and inverts the aspect ratio."""
    options = {
        "melonds_number_of_screen_layouts": "2",
        "melonds_screen_layout1": "top-bottom",
        "melonds_screen_layout2": "rotate-left",
        "melonds_screen_gap": "0",
    }

    with session(input=_press_r3_after(10), options=options) as emulator:
        screen_layout = emulator.get_proc_address(b"melondsds_screen_layout", TypedFunctionPointer[c_int, []])
        assert screen_layout is not None

        for _ in range(10):
            emulator.run()

        assert emulator.video.rotation == Rotation.NONE

        # Screen layout 0 is TopBottom.
        assert screen_layout() == 0

        frame1 = emulator.video.screenshot(False)
        geometry1 = emulator.video.geometry

        assert frame1 is not None
        assert geometry1 is not None

        assert frame1.width == geometry1.base_width
        assert frame1.height == geometry1.base_height

        for _ in range(200):
            emulator.run()

        assert emulator.video.rotation == Rotation.NINETY

        # Screen layout 10 is TurnLeft.
        assert screen_layout() == 10

        frame2 = emulator.video.screenshot(False)
        geometry2 = emulator.video.geometry

        assert frame2 is not None
        assert geometry2 is not None
        assert geometry1 != geometry2

        assert frame2.width == geometry2.base_width
        assert frame2.height == geometry2.base_height

        # Rotating by 90 degrees inverts the aspect ratio.
        assert geometry1.aspect_ratio == pytest.approx(1.0 / geometry2.aspect_ratio)
        assert frame1.data != frame2.data


@pytest.mark.nds_rom
def test_hybrid_screen_ratio(session: SessionFactory, nds_rom: Path) -> None:
    """The hybrid screen layout accepts a 3:1 ratio."""
    options = {
        "melonds_hybrid_ratio": "3",
        "melonds_screen_layout1": "hybrid-top",
    }

    with session(nds_rom, options=options) as emulator:
        emulator.run()


# --------------------------------------------------------------------------- #
# Software rendering
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_unloads_with_threaded_software_renderer(session: SessionFactory, nds_rom: Path) -> None:
    """
    The threaded software renderer shuts down cleanly.

    See https://github.com/JesseTG/melonds-ds/issues/70
    """
    options = {**DIRECT_BOOT_BUILTIN, "melonds_threaded_renderer": "enabled"}

    with session(nds_rom, options=options) as emulator:
        for _ in range(300):
            emulator.run()
