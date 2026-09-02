"""Audio and video output, screen geometry and rotation."""

from __future__ import annotations

import itertools
import time
from ctypes import c_int
from pathlib import Path

import pytest
from libretro import JoypadState, PixelFormat, Rotation, Screenshot, ThrottleMode
from libretro.api.timing import retro_throttle_state
from libretro.ctypes import TypedFunctionPointer
from libretro.drivers import DefaultTimingDriver

from melondsds import SessionFactory
from melondsds.options import DIRECT_BOOT_BUILTIN

#: The DS's SPU output rate, in Hz; the rate the core reports to the frontend.
SAMPLE_RATE = 33513982.0 / 1024.0


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
def test_fast_forward_audio_is_paced(session: SessionFactory, nds_rom: Path) -> None:
    """
    Fast-forward audio is stretched down to real time, not pushed through raw.

    The SPU still produces one frame of audio per ``retro_run``, but those calls no
    longer arrive at real time, so smoothing paces the core's output against the wall
    clock instead. For the same number of emulated frames that means far fewer samples
    than smoothing off, which hands the frontend everything the SPU made.
    """

    calls = 300

    def emitted(*, smoothing: str) -> tuple[int, float]:
        timing = DefaultTimingDriver(
            retro_throttle_state(ThrottleMode.FAST_FORWARD, 4 * 60.0), 60.0
        )
        options = {**DIRECT_BOOT_BUILTIN, "melonds_audio_time_stretch": smoothing}
        with session(nds_rom, timing=timing, options=options) as emulator:
            start = time.monotonic()
            for _ in range(calls):
                emulator.run()
            wall = time.monotonic() - start

            # The driver stores interleaved samples, so halve for stereo frames.
            return len(emulator.audio.buffer) // 2, wall

    raw, raw_wall = emitted(smoothing="disabled")
    smoothed, _ = emitted(smoothing="enabled")

    assert raw > 0, "expected audio with smoothing off"
    assert smoothed > 0, "smoothing silenced the core entirely"

    # Stretching paces output against the wall clock, so it emits roughly what real
    # time owes however fast the emulator ran. The two streams therefore only differ
    # while the harness runs well ahead of real time; a slow build can take longer
    # than a frame per retro_run, at which point the paced stream converges on the
    # raw one and there is nothing here to measure. Skip rather than assert something
    # the environment cannot demonstrate. The threshold is a third rather than the
    # half below so the two conditions can't both miss.
    owed = raw_wall * SAMPLE_RATE
    if owed > raw / 3:
        pytest.skip(
            f"harness ran near real time ({owed:.0f} frames owed against {raw} "
            "produced); paced and raw output are indistinguishable here"
        )

    # A loose "smoothed < raw" passes even when the option is ignored entirely,
    # so require the paced stream to be dramatically smaller.
    assert smoothed < raw / 2, (
        f"smoothing emitted {smoothed} frames against {raw} raw "
        f"({owed:.0f} owed by real time); expected the paced stream to be far smaller"
    )


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


@pytest.mark.dsiware_rom
@pytest.mark.dsi_firmware
@pytest.mark.no_skip_error_screen
@pytest.mark.parametrize(
    ("options", "probe"),
    [
        # The bottom screen is drawn at half size, centered under the top screen.
        pytest.param(
            {"melonds_screen_layout1": "top-bottom", "melonds_secondary_screen_scale": "50"},
            (128, 240),
            id="secondary-scale",
        ),
        # The top screen is drawn at triple size on the left.
        pytest.param(
            {"melonds_screen_layout1": "hybrid-top", "melonds_hybrid_ratio": "3"},
            (384, 288),
            id="hybrid",
        ),
    ],
)
def test_error_screen_honors_screen_layout(
    session: SessionFactory, dsiware_rom: Path, options: dict[str, str], probe: tuple[int, int]
) -> None:
    """
    The error screen is laid out the same way the emulated screens would be,
    including any screen that's scaled through a staging buffer.

    See https://github.com/JesseTG/melonds-ds/issues/316
    """
    with session(dsiware_rom, options=options) as emulator:
        for _ in range(10):
            emulator.run()

        frame = emulator.video.screenshot()
        geometry = emulator.video.geometry

        assert frame is not None
        assert geometry is not None
        assert frame.width == geometry.base_width
        assert frame.height == geometry.base_height

        # The probe lands in the middle of the scaled screen,
        # which is left blank if the staging buffer was never set up.
        x, y = probe
        offset = (y * frame.width + x) * 4
        pixel = bytes(frame.data[offset : offset + 3])  # XRGB8888, so B, G, R come first
        assert pixel != b"\x00\x00\x00"


@pytest.mark.dsiware_rom
@pytest.mark.dsi_firmware
@pytest.mark.no_skip_error_screen
def test_error_screen_cycles_layout_with_hotkey(session: SessionFactory, dsiware_rom: Path) -> None:
    """
    The screen layout hotkey still cycles layouts while the error screen is shown.

    See https://github.com/JesseTG/melonds-ds/issues/316
    """
    options = {
        "melonds_number_of_screen_layouts": "2",
        "melonds_screen_layout1": "top-bottom",
        "melonds_screen_layout2": "left-right",
    }

    with session(dsiware_rom, input=_press_r3_after(10), options=options) as emulator:
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
        assert frame2.width == geometry2.base_width
        assert frame2.height == geometry2.base_height


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
