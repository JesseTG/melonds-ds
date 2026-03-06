from typing import cast
from libretro import ModernGlVideoDriver

import prelude

with prelude.builder().with_video(ModernGlVideoDriver).build() as session:
    video = cast(ModernGlVideoDriver, session.video)

    assert isinstance(video, ModernGlVideoDriver), f"Expected ModernGlVideoDriver, got {type(video).__name__}"

    for i in range(70):
        session.run()

    frame1 = video.screenshot()
    assert frame1 is not None

    for i in range(360):
        session.run()

    frame2 = video.screenshot()
    assert frame2 is not None

    geometry = video.geometry
    size = (geometry.base_width, geometry.base_height)

    assert len(frame1.data) == len(frame2.data)
    assert frame1 != frame2
