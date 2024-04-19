from typing import cast

from libretro import Session, PillowVideoDriver

import prelude

options = {
    b"melonds_show_cursor": b"disabled"
}

WHITE = (0xFF, 0xFF, 0xFF, 0xFF)

session: Session
with prelude.builder().with_options(options).with_video(PillowVideoDriver).build() as session:
    session.core.run()

    video = cast(PillowVideoDriver, session.video)

    # Very first frame should be all white
    blank_frame = video.frame
    blank_colors = blank_frame.getcolors()
    assert blank_colors is not None and len(blank_colors) == 1, f"Expected an all-white frame, got {blank_colors}"

    for i in range(300):
        session.core.run()

    after_frame = video.frame
    assert blank_frame != after_frame, "Screen is still blank after 300 frames"

    session.core.reset()

    for i in range(300):
        session.core.run()

    after_reset_frame = video.frame
    assert blank_frame != after_reset_frame, "Screen is still blank after resetting and running for 300 frames"
