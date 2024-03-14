import math
import time
from array import array
import itertools
from typing import cast
from math import sin, cos, tau, pi

import wand.display
from libretro import Session
from libretro.api.input import Point
from libretro.api.input.pointer import Pointer
from libretro.api.video import SoftwareVideoState

from wand.image import Image

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
with prelude.session(input_state=generate_input) as session:
    video = cast(SoftwareVideoState, session.video)
    for i in range(180):
        session.core.run()

    frame1 = array(video.frame.typecode, video.frame)

    for i in range(240):
        session.core.run()

    frame2 = array(video.frame.typecode, video.frame)

    screen1 = Image(blob=frame1.tobytes(), format="BGRA", width=256, height=192*2, depth=8)
    screen2 = Image(blob=frame2.tobytes(), format="BGRA", width=256, height=192*2, depth=8)

    assert screen1.get_image_distortion(screen2, 'absolute') > 50000
