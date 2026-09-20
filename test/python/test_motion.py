"""
The Slot-2 motion sensors: the homebrew DS Motion Pak and the retail DS Motion Pack.

Every case here loads ``test/nds/periph_motion_card.nds``,
BlocksDS's motion sensor example.
It detects whichever motion pak is in Slot-2
and prints what it reads from it to the bottom screen every frame,
which :func:`melondsds.libnds.read_demo_console` reads back.
"""

from __future__ import annotations

import re
from collections.abc import Callable, Iterator
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pytest
from libretro import (
    Content,
    IterableSensorDriver,
    Port,
    PortInput,
    Session,
    SubsystemContent,
    Vector3,
)

from melondsds import SessionFactory
from melondsds.libnds import read_demo_console

pytestmark = pytest.mark.periph_motion_card_nds

HOMEBREW = "motion-pak-homebrew"
RETAIL = "motion-pak-retail"

#: Slot-2 device option value -> the name the ROM gives the pak it finds.
PAK_NAMES = {HOMEBREW: "DS Motion Pak", RETAIL: "DS Motion Pack"}

#: How far the ROM's acceleration readout may stray from the host's reading, in mG.
#:
#: Both paks span 5 g,
#: but the homebrew pak's accelerometer has 12 bits (about 1.2 mG per step)
#: where the retail pack's has only 8 (about 20 mG per step).
ACCELERATION_TOLERANCE = {HOMEBREW: 5, RETAIL: 20}

#: A host device lying flat and still.
LYING_FLAT = PortInput(accelerometer=Vector3(0.0, 0.0, 1.0))

#: The longest a test waits for the ROM to show what it's looking for.
#: The ROM prints its first readout within a few frames of booting.
MAX_FRAMES = 60


@pytest.fixture
def session(session: SessionFactory) -> SessionFactory:
    """
    Force DS mode for every session in this module.

    Slot-2 is DS-only hardware,
    and ``periph_motion_card.nds`` is built with BlocksDS,
    which stamps its ROMs with a DSiWare title ID
    that sends it to a DSi in "auto" console mode.
    See ``test_slot2.py`` for the details.
    """

    def make(game: Content | SubsystemContent | None = None, /, **kwargs: Any) -> Session:
        kwargs["options"] = {"melonds_console_mode": "ds", **kwargs.get("options", {})}
        return session(game, **kwargs)

    return make


class HostMotion:
    """The host's motion sensor readings, which a test can change between frames."""

    def __init__(self, reading: PortInput = LYING_FLAT) -> None:
        """Start out reporting ``reading``."""
        self.reading = reading

    def __iter__(self) -> Iterator[PortInput]:
        """Report the current reading every time the sensors are polled."""
        while True:
            yield self.reading


@dataclass(frozen=True)
class Readout:
    """What ``periph_motion_card.nds`` shows on the bottom screen."""

    #: The name of the motion pak the ROM found, or ``"None"`` if it found nothing.
    device: str

    #: The calibrated acceleration along each axis, in mG.
    acceleration: tuple[int, int, int]

    #: The calibrated rotation around the Z axis, in degrees per second.
    rotation: int

    @classmethod
    def parse(cls, screen: tuple[str, ...]) -> Readout | None:
        """Parse the bottom screen, or return ``None`` if the ROM hasn't finished printing it."""
        text = "\n".join(screen)
        device = re.search(r"^Device: (.+)$", text, re.MULTILINE)
        axes = [
            re.search(rf"^{axis} acceleration: (-?\d+) mG$", text, re.MULTILINE) for axis in "XYZ"
        ]
        rotation = re.search(r"^\s*Z rotation: (-?\d+) deg$", text, re.MULTILINE)

        if not (device and rotation and all(axes)):
            return None

        x, y, z = (int(axis[1]) for axis in axes if axis)
        return cls(device[1], (x, y, z), int(rotation[1]))


def run_until(emulator: Session, predicate: Callable[[Readout], bool]) -> Readout:
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

    pytest.fail(
        "The ROM never showed the expected readout. Its screen says:\n" + "\n".join(screen)
    )


def near(actual: tuple[int, ...], expected: tuple[int, ...], tolerance: int) -> bool:
    """Return whether every element of ``actual`` is within ``tolerance`` of ``expected``."""
    return all(abs(a - e) <= tolerance for a, e in zip(actual, expected, strict=True))


# --------------------------------------------------------------------------- #
# Detection
# --------------------------------------------------------------------------- #


@pytest.mark.parametrize(
    ("device", "name"),
    [
        pytest.param(HOMEBREW, PAK_NAMES[HOMEBREW], id="homebrew"),
        pytest.param(RETAIL, PAK_NAMES[RETAIL], id="retail"),
        pytest.param("auto", "None", id="none"),
    ],
)
def test_motion_pak_detected(
    session: SessionFactory, periph_motion_card_nds: Path, device: str, name: str
) -> None:
    """The ROM finds the motion pak that the Slot-2 option inserted, and only that one."""
    with session(periph_motion_card_nds, options={"melonds_slot2_device": device}) as emulator:
        readout = run_until(emulator, lambda _: True)

    assert readout.device == name


@pytest.mark.parametrize(
    ("device", "accelerometer", "gyroscope"),
    [
        pytest.param(HOMEBREW, True, True, id="homebrew"),
        pytest.param(RETAIL, True, False, id="retail"),
        pytest.param("auto", False, False, id="none"),
    ],
)
def test_motion_pak_enables_host_sensors(
    session: SessionFactory,
    periph_motion_card_nds: Path,
    device: str,
    accelerometer: bool,
    gyroscope: bool,
) -> None:
    """
    A motion pak switches on the host sensors it needs, and no others.

    Only the homebrew pak has a gyroscope.
    """
    sensor = IterableSensorDriver(HostMotion())
    options = {"melonds_slot2_device": device}

    with session(periph_motion_card_nds, options=options, sensor=sensor) as emulator:
        emulator.run()

        state = sensor.sensor_state[Port(0)]
        assert state.accelerometer.enabled == accelerometer
        assert state.gyroscope.enabled == gyroscope
        assert not state.illuminance.enabled


# --------------------------------------------------------------------------- #
# Readings
# --------------------------------------------------------------------------- #


@pytest.mark.parametrize("device", [HOMEBREW, RETAIL], ids=["homebrew", "retail"])
def test_motion_pak_reads_host_accelerometer(
    session: SessionFactory, periph_motion_card_nds: Path, device: str
) -> None:
    """
    The ROM sees the host's acceleration on the same axes, with the same signs.

    Each axis gets a different reading,
    so that a swapped or flipped axis can't go unnoticed.
    """
    host = HostMotion(PortInput(accelerometer=Vector3(0.5, -0.25, 0.75)))
    options = {"melonds_slot2_device": device}
    tolerance = ACCELERATION_TOLERANCE[device]

    with session(
        periph_motion_card_nds, options=options, sensor=IterableSensorDriver(host)
    ) as emulator:
        run_until(emulator, lambda r: near(r.acceleration, (500, -250, 750), tolerance))

        # The pak follows the host from one frame to the next,
        # not just once when the game starts.
        host.reading = PortInput(accelerometer=Vector3(-0.75, 0.5, -0.25))
        run_until(emulator, lambda r: near(r.acceleration, (-750, 500, -250), tolerance))


def test_motion_pak_reads_host_gyroscope(
    session: SessionFactory, periph_motion_card_nds: Path
) -> None:
    """
    The homebrew pak's gyroscope sees the host's rotation around the Z axis.

    The ROM reports degrees per second, and 1 rad/s is about 57 of them.
    Only the magnitude is checked;
    which way is positive is up to melonDS, which describes its choice as a guess.
    """
    host = HostMotion(PortInput(accelerometer=Vector3(0.0, 0.0, 1.0), gyroscope=Vector3(z=1.0)))
    options = {"melonds_slot2_device": HOMEBREW}

    with session(
        periph_motion_card_nds, options=options, sensor=IterableSensorDriver(host)
    ) as emulator:
        run_until(emulator, lambda r: abs(abs(r.rotation) - 57) <= 2)


def test_motion_pack_ignores_host_gyroscope(
    session: SessionFactory, periph_motion_card_nds: Path
) -> None:
    """The retail pack has no gyroscope, so the host's rotation doesn't reach the ROM."""
    host = HostMotion(PortInput(accelerometer=Vector3(0.5, 0.0, 1.0), gyroscope=Vector3(z=1.0)))
    options = {"melonds_slot2_device": RETAIL}
    tolerance = ACCELERATION_TOLERANCE[RETAIL]

    with session(
        periph_motion_card_nds, options=options, sensor=IterableSensorDriver(host)
    ) as emulator:
        # Wait for the host's reading to show up, so we know the sensors are live
        readout = run_until(emulator, lambda r: near(r.acceleration, (500, 0, 1000), tolerance))

    assert readout.rotation == 0


@pytest.mark.parametrize("device", [HOMEBREW, RETAIL], ids=["homebrew", "retail"])
def test_motion_pak_lies_flat_without_host_sensors(
    session: SessionFactory, periph_motion_card_nds: Path, device: str
) -> None:
    """
    Without host sensors, the pak reads as if it were lying flat and still.

    That's 1 g straight up and no rotation,
    the same as standalone melonDS does without a motion-capable controller.
    The player is told why the pak isn't responding.
    """
    options = {"melonds_slot2_device": device}
    tolerance = ACCELERATION_TOLERANCE[device]

    with session(periph_motion_card_nds, options=options, sensor=None) as emulator:
        readout = run_until(emulator, lambda r: r.device == PAK_NAMES[device])
        assert near(readout.acceleration, (0, 0, 1000), tolerance)
        assert readout.rotation == 0

        assert any(b"motion sensors" in m.msg for m in emulator.message.message_exts if m.msg)


# --------------------------------------------------------------------------- #
# Releasing the host sensors
# --------------------------------------------------------------------------- #


@pytest.mark.parametrize(
    ("before", "after", "accelerometer", "gyroscope"),
    [
        pytest.param(HOMEBREW, "auto", False, False, id="removed"),
        pytest.param(HOMEBREW, HOMEBREW, True, True, id="homebrew-to-homebrew"),
        pytest.param(HOMEBREW, RETAIL, True, False, id="homebrew-to-retail"),
        pytest.param(RETAIL, HOMEBREW, True, True, id="retail-to-homebrew"),
    ],
)
def test_host_sensors_follow_the_pak_across_resets(
    session: SessionFactory,
    periph_motion_card_nds: Path,
    before: str,
    after: str,
    accelerometer: bool,
    gyroscope: bool,
) -> None:
    """
    Swapping motion paks leaves on exactly the host sensors that the new pak needs.

    A pak that stays in Slot-2 across a reset
    must not switch off the sensors it's still using.
    """
    sensor = IterableSensorDriver(HostMotion())

    with session(
        periph_motion_card_nds, options={"melonds_slot2_device": before}, sensor=sensor
    ) as emulator:
        emulator.run()

        emulator.options.variables["melonds_slot2_device"] = after.encode()
        emulator.reset()
        emulator.run()

        state = sensor.sensor_state[Port(0)]
        assert state.accelerometer.enabled == accelerometer
        assert state.gyroscope.enabled == gyroscope


def test_host_sensors_released_when_the_game_unloads(
    session: SessionFactory, periph_motion_card_nds: Path
) -> None:
    """Unloading the game switches off the host sensors that the motion pak was using."""
    sensor = IterableSensorDriver(HostMotion())
    options = {"melonds_slot2_device": HOMEBREW}

    with session(periph_motion_card_nds, options=options, sensor=sensor) as emulator:
        emulator.run()

        state = sensor.sensor_state[Port(0)]
        assert state.accelerometer.enabled
        assert state.gyroscope.enabled

    assert not state.accelerometer.enabled
    assert not state.gyroscope.enabled
