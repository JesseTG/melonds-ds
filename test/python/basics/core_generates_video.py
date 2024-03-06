from array import array
from sys import argv
from typing import cast

from libretro import default_session
from libretro.callback.video import SoftwareVideoState

with default_session(argv[1]) as session:
    video = cast(SoftwareVideoState, session.video)

    assert isinstance(video, SoftwareVideoState)

    for i in range(60):
        session.core.run()

    assert video.frame is not None

    frame1 = array(video.frame.typecode, video.frame)

    for i in range(60):
        session.core.run()

    frame2 = array(video.frame.typecode, video.frame)

    assert frame1 != frame2
