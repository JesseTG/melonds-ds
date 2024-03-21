from array import array
from typing import cast

from libretro import Session
from libretro.api.video import SoftwareVideoState

import prelude

options = {
    b"melonds_show_cursor": b"disabled"
}

WHITE = (0xFF, 0xFF, 0xFF, 0xFF)

session: Session
with prelude.session(options=options) as session:
    session.core.run()

    video = cast(SoftwareVideoState, session.video)

    # Very first frame should be all white
    blank_frame = array(video.frame.typecode, video.frame)
    assert all(byte == 0xFF for byte in blank_frame), "Screen is not blank"

    for i in range(300):
        session.core.run()

    after_frame = array(video.frame.typecode, video.frame)
    assert blank_frame != after_frame, "Screen is still blank after 300 frames"

    session.core.reset()

    for i in range(300):
        session.core.run()

    after_reset_frame = array(video.frame.typecode, video.frame)
    assert blank_frame != after_reset_frame, "Screen is still blank after resetting and running for 300 frames"
