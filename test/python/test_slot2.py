"""
Slot-2 accessories: the Memory Expansion Pak, Rumble Pak and Solar Sensor.

Every case here loads ``test/nds/periph_slot2.nds``,
a homebrew ROM that reports what's in Slot-2.
"""

from __future__ import annotations

from collections.abc import Iterator, Sequence
from ctypes import POINTER, c_int32, c_uint16, c_uint32
from itertools import chain, repeat
from pathlib import Path
from typing import Any

import pytest
from libretro.ctypes import TypedFunctionPointer, TypedPointer
from melondsds import SessionFactory

from libretro import (
    Content,
    DictRumbleDriver,
    InputDevice,
    InputStateGenerator,
    IterableSensorDriver,
    JoypadState,
    MouseState,
    Port,
    RecordingRumbleDriver,
    RumbleEffect,
    Session,
    SubsystemContent,
)

#: Cart type IDs reported by ``melondsds_get_gba_cart_type``.
NO_CART = 0
GBA_CART = 0x101
SOLAR_SENSOR = 0x102
EXPANSION_PAK = 0x201
RUMBLE_PAK = 0x202

#: Slot-2 device option value -> the cart type the core should report for it.
DEVICES = {
    "expansion-pak": EXPANSION_PAK,
    "rumble-pak": RUMBLE_PAK,
    "solar1": SOLAR_SENSOR,
}

#: The strongest rumble a libretro frontend can be asked for.
MAX_STRENGTH = 0xFFFF

#: ``int32_t melondsds_get_rumble_level()`` and ``int32_t melondsds_get_rumble_edges()``.
RumbleIntProbe = TypedFunctionPointer[c_int32, []]

#: ``uint32_t melondsds_rumble_envelope_window()``.
RumbleWindowProbe = TypedFunctionPointer[c_uint32, []]

#: ``uint32_t melondsds_rumble_filter(const uint32_t*, uint32_t, uint16_t*)``.
RumbleFilterProbe = TypedFunctionPointer[
    c_uint32, [TypedPointer[c_uint32], c_uint32, TypedPointer[c_uint16]]
]

pytestmark = pytest.mark.periph_slot2_nds


@pytest.fixture
def session(session: SessionFactory) -> SessionFactory:
    """
    Force DS mode for every session in this module.

    Slot-2 is DS-only hardware, so a DSi has nothing for these tests to look at.
    ``periph_slot2.nds`` is built with BlocksDS,
    which stamps its ROMs with a DSiWare title ID by default,
    and "auto" console mode sends anything DSiWare-flagged to a DSi.
    See https://github.com/JesseTG/melonds-ds/issues/319.
    """

    def make(game: Content | SubsystemContent | None = None, /, **kwargs: Any) -> Session:
        kwargs["options"] = {"melonds_console_mode": "ds", **kwargs.get("options", {})}
        return session(game, **kwargs)

    return make


# --------------------------------------------------------------------------- #
# Detecting an accessory that's configured up front
# --------------------------------------------------------------------------- #


@pytest.mark.parametrize("device", list(DEVICES), ids=list(DEVICES))
def test_accessory_detected(session: SessionFactory, periph_slot2_nds: Path, device: str) -> None:
    """A Slot-2 accessory chosen before load is reported by the ROM."""
    expected = DEVICES[device]

    with session(periph_slot2_nds, options={"melonds_slot2_device": device}) as emulator:
        emulator.run()
        cart_type_probe = emulator.get_proc_address(
            "melondsds_get_gba_cart_type", TypedFunctionPointer[c_uint32, []]
        )
        assert cart_type_probe is not None
        assert cart_type_probe() == expected


@pytest.mark.gba_rom
@pytest.mark.parametrize("device", ["expansion-pak", "rumble-pak"])
def test_no_accessory_with_gba_rom(
    session: SessionFactory, periph_slot2_nds: Path, gba_rom: Path, device: str
) -> None:
    """An explicitly loaded GBA ROM takes priority over a configured accessory."""
    content = SubsystemContent("gbanosav", (periph_slot2_nds, gba_rom))

    with session(content, options={"melonds_slot2_device": device}) as emulator:
        emulator.run()
        cart_type_probe = emulator.get_proc_address(
            "melondsds_get_gba_cart_type", TypedFunctionPointer[c_uint32, []]
        )
        assert cart_type_probe is not None
        assert cart_type_probe() == GBA_CART


# --------------------------------------------------------------------------- #
# Inserting and removing accessories at runtime
# --------------------------------------------------------------------------- #


@pytest.mark.parametrize("device", list(DEVICES), ids=list(DEVICES))
def test_accessory_inserted_after_reset(
    session: SessionFactory, periph_slot2_nds: Path, device: str
) -> None:
    """Choosing an accessory mid-run only takes effect after a reset."""
    expected = DEVICES[device]
    drivers = {"rumble": DictRumbleDriver()} if device == "rumble-pak" else {}

    with session(
        periph_slot2_nds, options={"melonds_slot2_device": "auto"}, **drivers
    ) as emulator:
        emulator.run()

        cart_type_probe = emulator.get_proc_address(
            "melondsds_get_gba_cart_type", TypedFunctionPointer[c_uint32, []]
        )
        assert cart_type_probe is not None
        assert cart_type_probe() == NO_CART

        emulator.options.variables["melonds_slot2_device"] = device.encode()
        emulator.run()

        # Inserting an accessory only takes effect after a reset.
        assert cart_type_probe() == NO_CART

        emulator.reset()

        assert cart_type_probe() == expected


@pytest.mark.parametrize("device", list(DEVICES), ids=list(DEVICES))
def test_accessory_removed_after_reset(
    session: SessionFactory, periph_slot2_nds: Path, device: str
) -> None:
    """Deselecting an accessory mid-run only takes effect after a reset."""
    expected = DEVICES[device]
    drivers = {"rumble": DictRumbleDriver()} if device == "rumble-pak" else {}

    with session(
        periph_slot2_nds, options={"melonds_slot2_device": device}, **drivers
    ) as emulator:
        emulator.run()

        cart_type_probe = emulator.get_proc_address(
            "melondsds_get_gba_cart_type", TypedFunctionPointer[c_uint32, []]
        )
        assert cart_type_probe is not None
        assert cart_type_probe() == expected

        emulator.options.variables["melonds_slot2_device"] = b"auto"
        emulator.run()

        # Removing an accessory only takes effect after a reset.
        assert cart_type_probe() == expected

        emulator.reset()

        assert cart_type_probe() == NO_CART


# --------------------------------------------------------------------------- #
# Rumble Pak
# --------------------------------------------------------------------------- #


#: Frames between taps of A in :func:`_tap_a`.
TAP_INTERVAL = 30

#: How many times :func:`_tap_a` taps A.
TAP_COUNT = 6


def _tap_a() -> Iterator[JoypadState | None]:
    """
    Tap A every ``TAP_INTERVAL`` frames.

    ``periph_slot2.nds`` buzzes the Rumble Pak once per press,
    as a burst of register toggles within a single frame.
    Holding A does nothing after the first frame,
    so the pak has to be re-triggered to be observed over time.
    """
    yield from chain.from_iterable(
        repeat([JoypadState(a=True), *repeat(None, TAP_INTERVAL - 1)], TAP_COUNT)
    )
    yield from repeat(None)


class RumbleFilter:
    """Drives the core's rumble envelope over a synthetic series of per-frame edge counts."""

    def __init__(self, probe: RumbleFilterProbe, window: int):
        """Wrap the core's ``melondsds_rumble_filter`` probe."""
        self.window = window
        self._probe = probe

    def __call__(self, edges: Sequence[int]) -> list[int]:
        """Return the strength the envelope would emit for each frame of ``edges``."""
        count = len(edges)
        buffer_in = (c_uint32 * count)(*edges)
        buffer_out = (c_uint16 * count)()
        assert self._probe(buffer_in, count, buffer_out) == count
        return list(buffer_out)

    def steady(self, edges: int) -> int:
        """Return the strength a constant ``edges``-per-frame rate settles on."""
        return self(list(repeat(edges, self.window * 2)))[-1]


@pytest.fixture
def rumble_filter(session: SessionFactory, periph_slot2_nds: Path) -> Iterator[RumbleFilter]:
    """Expose the core's rumble envelope as a pure function."""
    with session(periph_slot2_nds) as emulator:
        probe = emulator.get_proc_address("melondsds_rumble_filter", RumbleFilterProbe)
        window_probe = emulator.get_proc_address(
            "melondsds_rumble_envelope_window", RumbleWindowProbe
        )
        assert probe is not None, "the core doesn't export melondsds_rumble_filter"
        assert window_probe is not None, "the core doesn't export melondsds_rumble_envelope_window"

        window = window_probe()
        assert window > 1
        yield RumbleFilter(probe, window)


def test_rumble_filter_silence(rumble_filter: RumbleFilter) -> None:
    """No register toggles means no rumble."""
    frames = rumble_filter.window * 4
    assert rumble_filter([0] * frames) == [0] * frames


def test_rumble_filter_attacks_immediately(rumble_filter: RumbleFilter) -> None:
    """The very first frame of toggling already produces rumble."""
    levels = rumble_filter([1, *repeat(0, rumble_filter.window)])
    assert levels[0] > 0


def test_rumble_filter_saturates(rumble_filter: RumbleFilter) -> None:
    """A toggle rate well past the top of the scale pins the motors at full strength."""
    levels = rumble_filter([64] * (rumble_filter.window * 2))
    assert levels == sorted(levels)
    assert levels[-1] == MAX_STRENGTH


def test_rumble_filter_decays_to_zero(rumble_filter: RumbleFilter) -> None:
    """After the toggling stops, the rumble fades out instead of cutting off."""
    window = rumble_filter.window
    levels = rumble_filter([64] * window + [0] * (window * 2))
    tail = levels[window:]

    assert tail[0] > 0
    assert tail[window - 1] == 0

    # Strictly decreasing, not held at one level and then dropped;
    # a buzz that stops dead is exactly what this envelope exists to avoid.
    fading = tail[: window - 1]
    assert all(a > b for a, b in zip(fading, fading[1:], strict=False)), tail


def test_rumble_filter_scales_with_toggle_rate(rumble_filter: RumbleFilter) -> None:
    """
    A faster toggle rate means a stronger rumble.

    This is the whole point of the Rumble Pak's design:
    the actuator has no on/off setting,
    it moves each time the register is flipped,
    so how *often* the game flips it is what the player feels.
    """
    levels = [rumble_filter.steady(edges) for edges in range(9)]

    assert levels[0] == 0
    assert levels == sorted(levels)
    assert len(set(levels)) >= 3, f"toggle rate barely affects strength: {levels}"


@pytest.mark.parametrize(
    ("device", "should_rumble"),
    [
        pytest.param("rumble-pak", True, id="inserted"),
        pytest.param(None, False, id="not-inserted"),
    ],
)
def test_rumble(
    session: SessionFactory, periph_slot2_nds: Path, device: str | None, should_rumble: bool
) -> None:
    """The Rumble Pak drives the frontend's rumble motors, and only when inserted."""
    rumble = DictRumbleDriver()
    options = {} if device is None else {"melonds_slot2_device": device}
    strongest = 0
    weakest = 0

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)

        for _ in range(TAP_INTERVAL * TAP_COUNT):
            emulator.run()
            state = rumble[Port(0)]
            strongest = max(strongest, state.strong)
            weakest = max(weakest, state.weak)

    if should_rumble:
        assert strongest > 0
        assert strongest == weakest, "both motors should be driven by default"
    else:
        assert strongest == 0
        assert weakest == 0


def test_rumble_updates_motors_at_most_once_per_frame(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """
    The core sets the motors once per frame, not once per emulated register toggle.

    The Rumble Pak is toggled many times within a single frame,
    but libretro's rumble interface is level-based,
    so the core has to summarize a frame's worth of toggles into one strength.
    """
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak"}

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)

        per_frame: list[int] = []
        for _ in range(TAP_INTERVAL * TAP_COUNT):
            before = len(rumble.calls)
            emulator.run()
            per_frame.append(len(rumble.calls) - before)

    # One call per motor, at most.
    assert max(per_frame) <= 2, f"up to {max(per_frame)} rumble calls in a single frame"


def test_rumble_leaves_the_frontend_alone_while_idle(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """
    A Rumble Pak that isn't buzzing doesn't touch the frontend at all.

    Repeatedly setting a motor that's already stopped
    is wasted work in the frontend's haptic driver.
    """
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak"}

    with session(periph_slot2_nds, rumble=rumble, options=options) as emulator:
        # Let the ROM's startup buzz play out and decay.
        for _ in range(60):
            emulator.run()

        rumble.clear()
        for _ in range(60):
            emulator.run()

        # Checked before the session closes;
        # shutting down legitimately switches the motors off.
        assert not rumble.calls, f"{len(rumble.calls)} redundant rumble calls while idle"


def test_rumble_level_reported_only_with_pak(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """Without a Rumble Pak inserted, there's no rumble level to report."""
    rumble = RecordingRumbleDriver()

    with session(
        periph_slot2_nds, rumble=rumble, options={"melonds_slot2_device": "auto"}
    ) as emulator:
        level = emulator.get_proc_address("melondsds_get_rumble_level", RumbleIntProbe)
        assert level is not None, "the core doesn't export melondsds_get_rumble_level"

        for _ in range(60):
            emulator.run()
            assert level() == -1

    assert not rumble.calls
    state = rumble[Port(0)]
    assert state.strong == 0
    assert state.weak == 0


def test_rumble_stops_when_pak_is_removed(session: SessionFactory, periph_slot2_nds: Path) -> None:
    """Removing the Rumble Pak stops the motors instead of leaving them where they were."""
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak"}

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)
        level = emulator.get_proc_address("melondsds_get_rumble_level", RumbleIntProbe)
        assert level is not None, "the core doesn't export melondsds_get_rumble_level"

        for _ in range(TAP_INTERVAL * 2):
            emulator.run()

        emulator.options.variables["melonds_slot2_device"] = b"auto"
        emulator.reset()

        for _ in range(TAP_INTERVAL):
            emulator.run()

        assert level() == -1
        state = rumble[Port(0)]
        assert state.strong == 0
        assert state.weak == 0


def test_rumble_stops_when_the_game_unloads(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """Unloading the game leaves the frontend's motors switched off."""
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak"}

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)

        # Stop right after a tap, while the motors are still going.
        for _ in range(TAP_INTERVAL + 1):
            emulator.run()

    state = rumble[Port(0)]
    assert state.strong == 0
    assert state.weak == 0


def test_rumble_intensity_zero_is_silent(session: SessionFactory, periph_slot2_nds: Path) -> None:
    """Turning the rumble intensity all the way down stops the motors entirely."""
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak", "melonds_rumble_intensity": "0"}

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)
        level = emulator.get_proc_address("melondsds_get_rumble_level", RumbleIntProbe)
        assert level is not None, "the core doesn't export melondsds_get_rumble_level"

        for _ in range(TAP_INTERVAL * TAP_COUNT):
            emulator.run()
            assert level() == 0

    assert all(call.strength == 0 for call in rumble.calls)


@pytest.mark.parametrize(
    ("motor_type", "driven"),
    [
        pytest.param("both", (RumbleEffect.STRONG, RumbleEffect.WEAK), id="both"),
        pytest.param("strong", (RumbleEffect.STRONG,), id="strong"),
        pytest.param("weak", (RumbleEffect.WEAK,), id="weak"),
    ],
)
def test_rumble_motor_type(
    session: SessionFactory,
    periph_slot2_nds: Path,
    motor_type: str,
    driven: tuple[RumbleEffect, ...],
) -> None:
    """The rumble motor setting picks which of the controller's two motors buzz."""
    rumble = RecordingRumbleDriver()
    options = {"melonds_slot2_device": "rumble-pak", "melonds_rumble_type": motor_type}
    strongest = {RumbleEffect.STRONG: 0, RumbleEffect.WEAK: 0}

    with session(periph_slot2_nds, rumble=rumble, options=options, input=_tap_a) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)

        for _ in range(TAP_INTERVAL * TAP_COUNT):
            emulator.run()
            state = rumble[Port(0)]
            strongest[RumbleEffect.STRONG] = max(strongest[RumbleEffect.STRONG], state.strong)
            strongest[RumbleEffect.WEAK] = max(strongest[RumbleEffect.WEAK], state.weak)

    for effect in RumbleEffect:
        if effect in driven:
            assert strongest[effect] > 0, f"{effect.name} motor was never driven"
        else:
            assert strongest[effect] == 0, f"{effect.name} motor shouldn't have been driven"


# --------------------------------------------------------------------------- #
# Solar Sensor
# --------------------------------------------------------------------------- #


def _select_up(times: int):
    """Tap Select+Up ``times`` times, which raises the solar sensor level."""
    for _ in range(times):
        yield JoypadState(select=True, up=True)
        yield None


def _select_down(times: int):
    """Tap Select+Down ``times`` times, which lowers the solar sensor level."""
    for _ in range(times):
        yield JoypadState(select=True, down=True)
        yield None


def _button_input():
    yield 0
    yield from _select_up(10)
    yield from _select_down(5)
    yield from repeat(None)


def _mouse_wheel_input():
    yield 0
    yield from repeat(MouseState(wheel_up=True), 10)
    yield from repeat(MouseState(wheel_down=True), 5)
    yield from repeat(0)


@pytest.mark.parametrize(
    ("source", "generate_input", "up_frames", "down_frames"),
    [
        pytest.param("buttons", _button_input, 20, 10, id="buttons"),
        pytest.param("mouse-wheel", _mouse_wheel_input, 10, 5, id="mouse-wheel"),
    ],
)
def test_solar_sensor_input(
    session: SessionFactory,
    periph_slot2_nds: Path,
    source: str,
    generate_input: InputStateGenerator,
    up_frames: int,
    down_frames: int,
) -> None:
    """Raising and lowering the solar sensor level from host input."""
    options = {
        "melonds_slot2_device": "solar1",
        "melonds_solar_sensor_host_sensor": "disabled",
    }

    with session(periph_slot2_nds, options=options, input=generate_input) as emulator:
        if source == "buttons":
            emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)

        emulator.run()

        light_level_probe = emulator.get_proc_address(
            "melondsds_get_solar_sensor_level", TypedFunctionPointer[c_int32, []]
        )
        assert light_level_probe is not None

        assert light_level_probe() == 0

        for _ in range(up_frames):
            emulator.run()

        assert light_level_probe() == 10

        for _ in range(down_frames):
            emulator.run()

        assert light_level_probe() == 5


def test_solar_sensor_from_host_sensor(session: SessionFactory, periph_slot2_nds: Path) -> None:
    """Illuminance readings from the frontend's sensor drive the solar sensor level."""
    options = {
        "melonds_slot2_device": "solar1",
        "melonds_solar_sensor_host_sensor": "enabled",
    }

    def generate_sensor_readings():
        yield 0
        yield from repeat(32000, 10)  # Direct sunlight
        yield from repeat(400, 10)  # Sunset
        yield from repeat(0)

    with session(periph_slot2_nds, options=options, sensor=generate_sensor_readings) as emulator:
        sensor = emulator.sensor
        assert isinstance(sensor, IterableSensorDriver)
        assert sensor.sensor_state[Port(0)].illuminance.enabled

        emulator.run()

        light_level_probe = emulator.get_proc_address(
            "melondsds_get_solar_sensor_level", TypedFunctionPointer[c_int32, []]
        )
        assert light_level_probe is not None

        assert light_level_probe() == 0

        for _ in range(10):
            emulator.run()

        assert light_level_probe() > 0

        for _ in range(5):
            emulator.run()

        assert 0 < light_level_probe() <= 10


def test_solar_sensor_falls_back_to_buttons(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """With the host sensor enabled but unavailable, button input still works."""
    options = {
        "melonds_slot2_device": "solar1",
        "melonds_solar_sensor_host_sensor": "enabled",
    }

    def generate_input():
        yield 0
        yield from _select_up(10)
        yield from repeat(None)

    with session(periph_slot2_nds, options=options, input=generate_input, sensor=None) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)
        emulator.run()

        light_level_probe = emulator.get_proc_address(
            "melondsds_get_solar_sensor_level", TypedFunctionPointer[c_int32, []]
        )
        assert light_level_probe is not None

        assert light_level_probe() == 0

        for _ in range(20):
            emulator.run()

        assert light_level_probe() > 0


def test_solar_sensor_message_if_unavailable(
    session: SessionFactory, periph_slot2_nds: Path
) -> None:
    """An on-screen notice appears when the host light sensor isn't available."""
    options = {
        "melonds_slot2_device": "solar1",
        "melonds_solar_sensor_host_sensor": "enabled",
    }

    def generate_input():
        yield 0
        yield from _select_up(10)
        yield from repeat(None)

    with session(periph_slot2_nds, options=options, input=generate_input, sensor=None) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)
        emulator.run()

        assert any(b"luminance" in m.msg for m in emulator.message.message_exts if m.msg)


def test_host_sensor_stays_disabled(session: SessionFactory, periph_slot2_nds: Path) -> None:
    """Disabling the host light sensor keeps the frontend's illuminance sensor off."""
    options = {
        "melonds_slot2_device": "solar1",
        "melonds_solar_sensor_host_sensor": "disabled",
    }

    with session(periph_slot2_nds, options=options) as emulator:
        emulator.set_controller_port_device(Port(0), InputDevice.JOYPAD)
        emulator.run()

        sensor = emulator.sensor
        assert isinstance(sensor, IterableSensorDriver)
        assert not sensor.sensor_state[Port(0)].illuminance.enabled
