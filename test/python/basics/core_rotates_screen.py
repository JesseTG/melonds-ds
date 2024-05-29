import itertools
from ctypes import CFUNCTYPE, c_int

from libretro import JoypadState, Rotation
import prelude

options = {
    b"melonds_number_of_screen_layouts": b"2",
    b"melonds_screen_layout1": b"top-bottom",
    b"melonds_screen_layout2": b"rotate-left",
    b"melonds_screen_gap": b"0",
}


def generate_input():
    # Wait a little while...
    yield from itertools.repeat(None, 10)

    # Cycle to the next screen layout
    yield JoypadState(r2=True)

    yield from itertools.repeat(None)


with prelude.builder().with_input(generate_input).with_options(options).build() as session:
    screen_layout = session.get_proc_address(b"melondsds_screen_layout", CFUNCTYPE(c_int))
    assert screen_layout is not None, "melondsds_screen_layout not defined"

    for i in range(10):
        session.run()

    assert session.video.rotation == Rotation.NONE, "Core rotated earlier than expected"

    layout1 = screen_layout()
    assert layout1 == 0, f"Expected screen layout 0 (TopBottom), got {layout1}"

    frame1 = session.video.screenshot()
    geometry1 = session.video.geometry

    assert frame1 is not None
    assert geometry1 is not None

    assert frame1.width == geometry1.base_width, \
        f"Frame width ({frame1.width}) should match geometry base width ({geometry1.base_width})"

    assert frame1.height == geometry1.base_height, \
        f"Frame height ({frame1.height}) should match geometry base height ({geometry1.height})"

    for i in range(200):
        session.run()

    assert session.video.rotation == Rotation.NINETY, f"Expected core rotation of 90 degrees, got {session.video.rotation}"

    layout2 = screen_layout()
    assert layout2 == 8, f"Expected screen layout 8 (TurnLeft), got {layout2}"

    frame2 = session.video.screenshot()
    geometry2 = session.video.geometry

    assert frame2 is not None
    assert geometry2 is not None

    assert geometry1 != geometry2, f"Geometry should have changed from {geometry1}"

    assert frame2.width == geometry2.base_width, \
        f"Frame width ({frame2.width}) should match geometry base width ({geometry2.base_width})"

    assert frame2.height == geometry2.base_height, \
        f"Frame height ({frame2.height}) should match geometry base height ({geometry2.height})"

    assert abs(geometry1.aspect_ratio - (1.0 / geometry2.aspect_ratio)) < 0.0001, \
        f"Expected aspect ratio of {1.0 / geometry1.aspect_ratio}, got {geometry2.aspect_ratio}"

    assert frame1.data != frame2.data, "Frame data should have changed to reflect the rotation"

