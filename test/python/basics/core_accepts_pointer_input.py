import itertools
from typing import cast
from math import sin, cos, pi

from libretro import Session, Point, Pointer, PillowVideoDriver

import prelude


def generate_circle_points(points: int, radius: int = 1, offset: float = 0):
    for i in range(points):
        angle = (pi * 2 * i / points) + offset
        yield Point(x=int(cos(angle) * radius), y=int(sin(angle) * radius))


def generate_input():
    circle = tuple(generate_circle_points(180, 0x6fff, pi/2))
    yield from iter(circle)
    # Circle the pointer around the screen, starting from near the bottom

    yield from itertools.repeat(circle[0], 120)
    # Hold the pointer in place for a bit

    yield Pointer(circle[0].x, circle[0].y, True)
    # Touch the screen at the current pointer position

    yield from itertools.repeat(circle[0])


session: Session
with prelude.builder().with_input(generate_input).with_video(PillowVideoDriver).build() as session:
    video = cast(PillowVideoDriver, session.video)
    for i in range(180):
        session.core.run()

    frame1 = video.frame

    for i in range(240):
        session.core.run()

    frame2 = video.frame

    # The logo screen (frame1) has a white pixel in the top left corner,
    # whereas the main menu screen doesn't
    assert frame1.getpixel((0, 0)) != frame2.getpixel((0, 0))
