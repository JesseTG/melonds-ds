from typing import cast

from libretro import Session, PillowVideoDriver

import prelude

session: Session
with prelude.builder().with_video(PillowVideoDriver).build() as session:
    video = cast(PillowVideoDriver, session.video)

    assert isinstance(video, PillowVideoDriver), f"Expected PillowVideoDriver, got {type(video).__name__}"

    for i in range(70):
        session.core.run()

    frame1 = video.frame
    assert frame1 is not None

    for i in range(70):
        session.core.run()

    frame2 = video.frame
    assert frame2 is not None

    assert frame1.size == frame2.size
    assert frame1 != frame2
