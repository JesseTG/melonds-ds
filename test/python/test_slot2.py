"""
Slot-2 accessories: the Memory Expansion Pak, Rumble Pak and Solar Sensor.

Every case here loads ``test/nds/periph_slot2.nds``,
a homebrew ROM that reports what's in Slot-2.
"""

from __future__ import annotations

from ctypes import c_int32, c_uint32
from itertools import repeat
from pathlib import Path

import pytest
from libretro import (
    DictRumbleDriver,
    InputDevice,
    InputStateGenerator,
    IterableSensorDriver,
    JoypadState,
    MouseState,
    Port,
    SubsystemContent,
)
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory

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

pytestmark = pytest.mark.periph_slot2_nds


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

    with session(periph_slot2_nds, rumble=rumble, options=options) as emulator:
        emulator.run()

        driver = emulator.rumble
        assert isinstance(driver, DictRumbleDriver)

        state = driver[Port(0)]
        assert state is not None

        if should_rumble:
            assert state.strong > 0 or state.weak > 0
        else:
            assert state.strong == 0
            assert state.weak == 0


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
