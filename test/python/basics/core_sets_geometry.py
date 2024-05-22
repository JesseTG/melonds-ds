import itertools
from typing import cast

from libretro import JoypadState

import prelude

options = {
    b"melonds_number_of_screen_layouts": b"2",
    b"melonds_screen_layout1": b"top-bottom",
    b"melonds_screen_layout2": b"left-right",
}


def generate_input():
    # Wait a little while...
    yield from itertools.repeat(None, 10)

    # Cycle to the next screen layout
    yield JoypadState(r2=True)

    yield from itertools.repeat(None)


with prelude.builder().with_input(generate_input).build() as session:
    for i in range(10):
        session.run()

    frame1 = session.video.screenshot()
    geometry1 = session.video.geometry

    assert frame1 is not None
    assert geometry1 is not None

    assert frame1.width == geometry1.base_width, \
        f"Frame width ({frame1.width}) should match geometry base width ({geometry1.base_width})"

    assert frame1.height == geometry1.base_height, \
        f"Frame height ({frame1.height}) should match geometry base height ({geometry1.height})"

    for i in range(20):
        session.run()

    # Now that we've changed the screen layout, let's make sure we changed the geometry

    frame2 = session.video.screenshot()
    geometry2 = session.video.geometry

    assert frame2 is not None
    assert geometry2 is not None

    assert geometry1 != geometry2, f"Geometry should have changed from {geometry1} after switching screen layout"
