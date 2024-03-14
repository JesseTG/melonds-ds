from array import array
import itertools
from typing import cast

from libretro import Session
from libretro.api.input import JoypadState
from libretro.api.video import SoftwareVideoState

from wand.image import Image

import prelude


def generate_input():
    yield from itertools.repeat(0, 240)

    yield JoypadState(a=True)

    yield from itertools.repeat(0)


session: Session
with prelude.session(input_state=generate_input) as session:
    video = cast(SoftwareVideoState, session.video)
    for i in range(240):
        session.core.run()

    frame1 = array(video.frame.typecode, video.frame)
    for i in range(240):
        session.core.run()

    frame2 = array(video.frame.typecode, video.frame)

    screen1 = Image(blob=frame1.tobytes(), format="BGRA", width=256, height=192*2, depth=8)
    screen2 = Image(blob=frame2.tobytes(), format="BGRA", width=256, height=192*2, depth=8)

    assert screen1.get_image_distortion(screen2, 'absolute') > 50000
    # Assert that the two screenshots differ by more than 50000 pixels
