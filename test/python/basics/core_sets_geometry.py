import itertools
from typing import cast

from libretro import JoypadState, PillowVideoDriver

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


with prelude.builder().with_input(generate_input).with_video(PillowVideoDriver).build() as session:
    video = cast(PillowVideoDriver, session.video)
    for i in range(10):
        session.core.run()

    frame1 = video.frame
    framebuffer1 = video.frame_max
    geometry1 = video.geometry

    assert frame1 is not None
    assert framebuffer1 is not None
    assert geometry1 is not None

    assert frame1.size != framebuffer1.size, \
        f"Frame size should not be the same as framebuffer size ({frame1.size})"

    assert frame1.width < framebuffer1.width, \
        f"Frame width ({frame1.width}) shouldn't exceed framebuffer width ({framebuffer1.width})"

    assert frame1.height < framebuffer1.height, \
        f"Frame height ({frame1.height}) shouldn't exceed framebuffer height ({framebuffer1.height})"

    assert frame1.width == geometry1.base_width, \
        f"Frame width ({frame1.width}) should match geometry base width ({geometry1.base_width})"

    assert frame1.height == geometry1.base_height, \
        f"Frame height ({frame1.height}) should match geometry base height ({geometry1.height})"

    assert framebuffer1.width == geometry1.max_width, \
        f"Framebuffer width ({framebuffer1.width}) should match geometry max width ({geometry1.max_width})"

    assert framebuffer1.height == geometry1.max_height, \
        f"Framebuffer height ({framebuffer1.height}) should match geometry max height ({geometry1.max_height})"

    for i in range(20):
        session.core.run()

    # Now that we've changed the screen layout, let's make sure we changed the geometry

    frame2 = video.frame
    framebuffer2 = video.frame_max
    geometry2 = video.geometry

    assert frame2 is not None
    assert framebuffer2 is not None
    assert geometry2 is not None

    assert geometry1 != geometry2, f"Geometry should have changed from {geometry1} after switching screen layout"
    assert framebuffer1.size == framebuffer2.size, \
        f"Framebuffer size changed from {framebuffer1.size} to {framebuffer2.size}, but it shouldn't have"
