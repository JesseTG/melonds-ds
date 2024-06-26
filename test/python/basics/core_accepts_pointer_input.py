import itertools
from math import sin, cos, pi

from libretro import Session, Point, Pointer

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
with prelude.builder().with_input(generate_input).build() as session:
    for i in range(180):
        session.run()

    frame1 = session.video.screenshot()

    for i in range(240):
        session.run()

    frame2 = session.video.screenshot()

    # The logo screen (frame1) has a white pixel in the top left corner,
    # whereas the main menu screen doesn't
    assert frame1.data[0:4] != frame2.data[0:4]
