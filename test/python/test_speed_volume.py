"""Per-speed audio volume: the volume applied for each frontend throttle mode."""

from __future__ import annotations

from array import array
from pathlib import Path

import pytest
from libretro import Session, ThrottleMode
from libretro.api.timing import retro_throttle_state
from libretro.drivers import DefaultTimingDriver

from melondsds import SessionFactory

#: Frames to run before sampling audio; matches test_av.test_generates_audio.
FRAMES = 300

#: Upper bound on frames to search for sustained audio before triggering a mid-session
#: transition, in _run_until_sustained_audio below. Generous rather than tuned to any
#: one ROM: test/README.md promises "the tests don't assume any particular ROM," so a
#: ROM with a long silent intro, or one that waits at a title screen, must be searched
#: past rather than assumed away with a fixed frame count.
RAMP_WARMUP_FRAME_CAP = 3000

#: Minimum ratio between the transition buffer's opening-tenth peak and its
#: closing-tenth peak, in test_transition_ramps_instead_of_snapping below. See that
#: test for why this threshold was chosen.
RAMP_SHAPE_MIN_FALL_RATIO = 5


def _timing(mode: ThrottleMode) -> DefaultTimingDriver:
    """A timing driver that reports ``mode`` to the core."""
    return DefaultTimingDriver(retro_throttle_state(mode, 0.0), 60.0)


def _run_until_sustained_audio(emulator: Session, buffer: array) -> None:
    """
    Run frames until two consecutive buffers are non-silent, up to
    ``RAMP_WARMUP_FRAME_CAP`` frames total.

    Two in a row, rather than one, makes it likely audio is genuinely sustained (e.g.
    background music) rather than a single transient blip: the caller needs the
    *next* buffer after this to carry audio too, to capture a real transition.

    Skips the test, rather than failing it, if the cap is reached without finding
    sustained audio: the ROM may have a long silent intro or sit at a title screen,
    and the suite promises not to assume otherwise (see test/README.md).
    """
    consecutive_audible_buffers = 0
    for _ in range(RAMP_WARMUP_FRAME_CAP):
        before = len(buffer)
        emulator.run()
        if any(sample != 0 for sample in buffer[before:]):
            consecutive_audible_buffers += 1
            if consecutive_audible_buffers >= 2:
                return
        else:
            consecutive_audible_buffers = 0

    pytest.skip(
        f"the test ROM produced no sustained audio within {RAMP_WARMUP_FRAME_CAP} frames"
    )


def _peak(session: SessionFactory, rom: Path, mode: ThrottleMode, **options: str) -> int:
    """Peak absolute sample value after running ``FRAMES`` frames under ``mode``."""
    variables = {key.encode(): value.encode() for key, value in options.items()}
    with session(rom, options=variables, timing=_timing(mode)) as emulator:
        for _ in range(FRAMES):
            emulator.run()

        buffer = emulator.audio.buffer
        assert buffer is not None
        assert len(buffer) > 0
        return max(abs(sample) for sample in buffer)


@pytest.mark.nds_rom
def test_full_volume_is_unchanged(session: SessionFactory, nds_rom: Path) -> None:
    """At 100% the speed-up volume leaves audio alone."""
    baseline = _peak(session, nds_rom, ThrottleMode.NONE)
    speeding = _peak(
        session, nds_rom, ThrottleMode.FAST_FORWARD, melonds_audio_speedup_volume="100"
    )
    assert speeding == baseline


@pytest.mark.nds_rom
def test_zero_volume_is_silent(session: SessionFactory, nds_rom: Path) -> None:
    """At 0% the speed-up volume silences audio entirely."""
    # Negative control: without the override, this ROM produces audible output,
    # so a silent result below is the option's doing, not a silent ROM.
    assert _peak(session, nds_rom, ThrottleMode.NONE) > 0

    assert (
        _peak(session, nds_rom, ThrottleMode.FAST_FORWARD, melonds_audio_speedup_volume="0")
        == 0
    )


@pytest.mark.nds_rom
@pytest.mark.parametrize("mode", [ThrottleMode.FAST_FORWARD, ThrottleMode.UNBLOCKED])
def test_speedup_volume_scales(
    session: SessionFactory, nds_rom: Path, mode: ThrottleMode
) -> None:
    """Both speed-up modes halve the peak at 50%."""
    baseline = _peak(session, nds_rom, ThrottleMode.NONE)
    halved = _peak(session, nds_rom, mode, melonds_audio_speedup_volume="50")

    # Integer scaling and rounding make this approximate, not exact.
    assert halved == pytest.approx(baseline // 2, rel=0.02, abs=2)


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("mode", "option"),
    [
        (ThrottleMode.SLOW_MOTION, "melonds_audio_slowmo_volume"),
        (ThrottleMode.REWINDING, "melonds_audio_rewind_volume"),
    ],
)
def test_each_mode_uses_its_own_option(
    session: SessionFactory, nds_rom: Path, mode: ThrottleMode, option: str
) -> None:
    """Slow motion and rewind read their own option, not the speed-up one."""
    # Silencing the *other* two options must not affect this mode.
    others = {
        key: "0"
        for key in (
            "melonds_audio_speedup_volume",
            "melonds_audio_slowmo_volume",
            "melonds_audio_rewind_volume",
        )
        if key != option
    }
    assert _peak(session, nds_rom, mode, **others) > 0

    # Silencing this mode's own option must silence it.
    assert _peak(session, nds_rom, mode, **{option: "0"}) == 0


@pytest.mark.nds_rom
@pytest.mark.parametrize("mode", [ThrottleMode.NONE, ThrottleMode.VSYNC, ThrottleMode.FRAME_STEPPING])
def test_normal_speed_ignores_every_option(
    session: SessionFactory, nds_rom: Path, mode: ThrottleMode
) -> None:
    """At normal speed no override applies, whatever the options say."""
    baseline = _peak(session, nds_rom, ThrottleMode.NONE)
    muted_everywhere = _peak(
        session,
        nds_rom,
        mode,
        melonds_audio_speedup_volume="0",
        melonds_audio_slowmo_volume="0",
        melonds_audio_rewind_volume="0",
    )
    assert muted_everywhere == baseline


@pytest.mark.nds_rom
def test_falls_back_when_throttle_state_unsupported(
    session: SessionFactory, nds_rom: Path
) -> None:
    """With neither GET_THROTTLE_STATE nor GET_FASTFORWARDING reported, no override applies."""
    # libretro.py 0.8.3 gates GET_THROTTLE_STATE and GET_FASTFORWARDING on the same
    # DefaultTimingDriver.throttle_state (drivers/environment/composite.py:1669,1992),
    # so a driver that refuses one refuses both. That means this only exercises the
    # third fallback tier in ThrottleVolume ("neither call available"); the
    # is_fastforwarding()-returns-True tier is out of reach with this harness and is
    # not covered anywhere in this suite.
    no_throttle = DefaultTimingDriver(None, 60.0)
    variables = {b"melonds_audio_speedup_volume": b"0"}
    with session(nds_rom, options=variables, timing=no_throttle) as emulator:
        for _ in range(FRAMES):
            emulator.run()

        buffer = emulator.audio.buffer
        assert buffer is not None
        assert len(buffer) > 0
        # Neither environment call is answered, so ThrottleVolume falls all the way
        # through to its default of 100: the override must NOT apply and audio must
        # survive despite melonds_audio_speedup_volume="0".
        assert any(sample != 0 for sample in buffer)


@pytest.mark.nds_rom
def test_out_of_range_option_falls_back_to_full_volume(
    session: SessionFactory, nds_rom: Path
) -> None:
    """An out-of-range option value is rejected by ParseIntegerInRange and defaults to 100."""
    baseline = _peak(session, nds_rom, ThrottleMode.NONE)
    out_of_range = _peak(
        session, nds_rom, ThrottleMode.FAST_FORWARD, melonds_audio_speedup_volume="150"
    )
    assert out_of_range == baseline


@pytest.mark.nds_rom
def test_transition_ramps_instead_of_snapping(
    session: SessionFactory, nds_rom: Path
) -> None:
    """
    The gain change on entering (or leaving) an off-speed mode is ramped across
    one buffer instead of snapping instantly, to avoid an audible click.

    ``DefaultTimingDriver.throttle_state`` is mutable, so this drives a real
    mode change mid-session (the same thing the frontend does when the user
    presses the fast-forward hotkey) rather than starting a fresh session
    already in fast-forward.
    """
    driver = DefaultTimingDriver(retro_throttle_state(ThrottleMode.NONE, 0.0), 60.0)
    variables = {b"melonds_audio_speedup_volume": b"0"}
    with session(nds_rom, options=variables, timing=driver) as emulator:
        buffer = emulator.audio.buffer
        assert buffer is not None
        _run_until_sustained_audio(emulator, buffer)

        # Negative control: audio is actually flowing before the transition, at
        # ThrottleMode.NONE the speed-up option has no effect (ThrottleVolume
        # returns 100 the entire time), so a fade below is the ramp's doing,
        # not a ROM that happened to go quiet.
        assert any(sample != 0 for sample in buffer)

        # Enter fast-forward with the speed-up volume at 0%. The volume RenderAudio
        # has stored is still 100 from the run above, so this is a genuine 100 -> 0
        # transition, not a session that started in fast-forward already at 0.
        before_transition = len(buffer)
        driver.throttle_state = retro_throttle_state(ThrottleMode.FAST_FORWARD, 0.0)
        emulator.run()  # Exactly one retro_run() call == exactly one RenderAudio buffer.

        transition_buffer = buffer[before_transition:]
        assert len(transition_buffer) > 0
        # A flat cut to 0% would make this buffer uniformly silent. A ramp fades it
        # instead: the gain starts at 100 (continuous with the previous buffer) and
        # falls from there, so this buffer must NOT be uniformly silent.
        assert any(sample != 0 for sample in transition_buffer)

        # Shape check: a ramp's gain falls *linearly* across the buffer, so its
        # opening frames must be much louder than its closing ones. This is what
        # actually distinguishes a ramp from a one-buffer-late snap: a broken
        # implementation that applies the *previous* (100%) volume flat across this
        # whole buffer, and only stores the new volume afterward, would also pass the
        # assertion above (non-silent throughout) without ramping anything. At
        # ~547 frames per buffer, a correct 100 -> 0 ramp's average gain across the
        # opening tenth is ~95%, versus ~5% across the closing tenth (its very last
        # frame lands around 1%) -- about a 19x gap by average gain, though peaks
        # (rather than averages) move with the signal too: this test's ROM measured
        # ~9.9x (peaks of 1599 vs 161). RAMP_SHAPE_MIN_FALL_RATIO of 5 stays below
        # that observed value for headroom against normal signal variation, while
        # staying far above the ~1.1x a flat, unramped buffer produced when tried;
        # verified experimentally against both broken implementations (see
        # review-fixes-report.md).
        tenth = max(1, len(transition_buffer) // 10)
        opening_peak = max(abs(sample) for sample in transition_buffer[:tenth])
        closing_peak = max(abs(sample) for sample in transition_buffer[-tenth:])
        assert opening_peak >= RAMP_SHAPE_MIN_FALL_RATIO * closing_peak, (
            f"opening-tenth peak {opening_peak} was not at least "
            f"{RAMP_SHAPE_MIN_FALL_RATIO}x the closing-tenth peak {closing_peak}; "
            "the gain doesn't look like it ramps down across the buffer"
        )

        after_transition = len(buffer)
        for _ in range(30):
            emulator.run()

        settled_buffer = buffer[after_transition:]
        assert len(settled_buffer) > 0
        # Once the one-buffer ramp has completed, the volume is flat at 0% again,
        # same as it was before the ramp existed: fully silent.
        assert all(sample == 0 for sample in settled_buffer)
