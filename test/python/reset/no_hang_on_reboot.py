import sys

if sys.version_info >= (3, 12):
    from itertools import batched
else:
    from more_itertools import batched

from libretro import Session
from PIL import Image

import prelude

options = {
    b"melonds_show_cursor": b"disabled"
}

session: Session
with prelude.builder().with_options(options).build() as session:
    session.run()

    # Very first frame should be all white
    blank_frame = session.video.screenshot()
    blank_framebuffer = blank_frame.data

    blank_colors = set(batched(blank_framebuffer, 4))
    assert blank_colors is not None and len(blank_colors) == 1, f"Expected an all-white frame, got {blank_colors}"

    for i in range(300):
        session.run()

    after_frame = session.video.screenshot()
    assert blank_frame != after_frame, "Screen is still blank after 300 frames"

    session.core.reset()

    for i in range(300):
        session.core.run()

    after_reset_frame = session.video.screenshot()
    assert blank_frame != after_reset_frame, "Screen is still blank after resetting and running for 300 frames"
