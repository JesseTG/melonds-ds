import itertools
from typing import cast

from libretro import Session
from libretro.api.input import JoypadState
from libretro.api.video import PillowVideoDriver

import prelude


def generate_input():
    yield from itertools.repeat(0, 240)

    yield JoypadState(a=True)

    yield from itertools.repeat(0)


session: Session
with prelude.session(input_state=generate_input) as session:
    video = cast(PillowVideoDriver, session.video)
    for i in range(240):
        session.core.run()

    frame1 = video.get_frame()
    for i in range(240):
        session.core.run()

    frame2 = video.get_frame()

    # The logo screen (frame1) has a white pixel in the top left corner,
    # whereas the main menu screen doesn't
    assert frame1.getpixel((0, 0)) != frame2.getpixel((0, 0))
