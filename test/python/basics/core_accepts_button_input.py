from itertools import repeat

from libretro import JoypadState
import prelude


def generate_input():
    yield from repeat(0, 240)
    yield JoypadState(a=True)
    yield from repeat(0)


with prelude.builder().with_input(generate_input).build() as session:
    for i in range(240):
        session.run()

    frame1 = session.video.screenshot()
    for i in range(240):
        session.run()

    frame2 = session.video.screenshot()

    # The logo screen (frame1) has a white pixel in the top left corner,
    # whereas the main menu screen doesn't
    assert frame1.data[0:4] != frame2.data[0:4]
