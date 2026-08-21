"""When the core opens and activates the host microphone."""

from __future__ import annotations

from collections.abc import Iterator
from itertools import repeat
from pathlib import Path
from random import randint

import pytest
from libretro import DeviceIdJoypad, GeneratorMicrophoneDriver

from melondsds import SessionFactory


def generate_noise() -> Iterator[int]:
    """Endless white noise, so the mic driver always has something to hand over."""
    while True:
        yield randint(-30000, 30000)


@pytest.mark.nds_rom
def test_mic_opened_at_start(session: SessionFactory, nds_rom: Path) -> None:
    """The core opens exactly one host microphone when configured to use one."""
    with session(nds_rom, options={"melonds_mic_input": "microphone"}) as emulator:
        microphones = emulator.mic.microphones

        assert len(microphones) == 1


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("mic_input_active", "hold_button", "expected_before", "frames", "expected_after"),
    [
        pytest.param("hold", True, 0, 1, 1, id="hold-held"),
        pytest.param("hold", False, 0, 2, 0, id="hold-not-held"),
        pytest.param("toggle", False, 0, 1, 0, id="toggle-not-toggled"),
        # The "always" case never checked the state before running a frame.
        pytest.param("always", False, None, 1, 1, id="always"),
    ],
)
def test_mic_active_at_start(
    session: SessionFactory,
    nds_rom: Path,
    mic_input_active: str,
    hold_button: bool,
    expected_before: int | None,
    frames: int,
    expected_after: int,
) -> None:
    """
    The microphone is only active when the configured activation condition holds.

    :param mic_input_active: Value for the ``melonds_mic_input_active`` option.
    :param hold_button: Whether to hold the microphone button (L3) down.
    :param expected_before: Active microphones before running, or :obj:`None` to skip the check.
    :param frames: How many frames to run before counting again.
    :param expected_after: Active microphones after running ``frames`` frames.
    """
    options = {"melonds_mic_input": "microphone", "melonds_mic_input_active": mic_input_active}
    drivers = {}

    # Only the button-driven cases fed the microphone; leaving the others on the
    # default driver keeps them identical to the scripts they replace.
    if mic_input_active in ("hold", "toggle"):
        drivers["mic"] = GeneratorMicrophoneDriver(generate_noise)

    if hold_button:
        drivers["input"] = lambda: repeat(DeviceIdJoypad.L3)

    with session(nds_rom, options=options, **drivers) as emulator:
        microphones = emulator.mic.microphones

        if expected_before is not None:
            assert len(tuple(m for m in microphones if m.state)) == expected_before

        for _ in range(frames):
            emulator.run()

        assert len(tuple(m for m in microphones if m.state)) == expected_after
