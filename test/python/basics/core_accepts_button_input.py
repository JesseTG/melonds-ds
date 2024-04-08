from itertools import repeat
from typing import cast

from libretro import JoypadState, PillowVideoDriver

import prelude


def generate_input():
    yield from repeat(0, 240)
    yield JoypadState(a=True)
    yield from repeat(0)


with prelude.builder().with_video(PillowVideoDriver).with_input(generate_input).build() as session:
    video = cast(PillowVideoDriver, session.video)
    for i in range(240):
        session.core.run()

    frame1 = video.frame
    for i in range(240):
        session.core.run()

    frame2 = video.frame

    # The logo screen (frame1) has a white pixel in the top left corner,
    # whereas the main menu screen doesn't
    assert frame1.getpixel((0, 0)) != frame2.getpixel((0, 0))
