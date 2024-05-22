from typing import cast

from libretro import Session, Screenshot

import prelude

session: Session
with prelude.builder().build() as session:
    for i in range(70):
        session.run()

    frame1 = session.video.screenshot()
    assert isinstance(frame1, Screenshot)

    for i in range(70):
        session.run()

    frame2 = session.video.screenshot()
    assert isinstance(frame2, Screenshot)

    assert (frame1.width, frame1.height) == (frame2.width, frame2.height)
    assert frame1.data != frame2.data
