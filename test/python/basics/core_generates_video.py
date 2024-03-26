from typing import cast

from libretro import Session
from libretro.api.video import PillowVideoDriver

import prelude

session: Session
with prelude.session() as session:
    video = cast(PillowVideoDriver, session.video)

    assert isinstance(video, PillowVideoDriver)

    for i in range(70):
        session.core.run()

    frame1 = video.get_frame()
    assert frame1 is not None

    for i in range(70):
        session.core.run()

    frame2 = video.get_frame()
    assert frame2 is not None

    assert frame1.size == frame2.size
    assert frame1 != frame2
