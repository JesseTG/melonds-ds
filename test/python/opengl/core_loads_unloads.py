from typing import cast
import time
from libretro import ModernGlVideoDriver

from PIL import Image
import prelude

def dd():
    return ModernGlVideoDriver(window="pyglet")

with prelude.builder().with_video(ModernGlVideoDriver).build() as session:
    video = cast(ModernGlVideoDriver, session.video)

    assert isinstance(video, ModernGlVideoDriver), f"Expected ModernGlVideoDriver, got {type(video).__name__}"

    for i in range(70):
        session.run()

    frame1 = video.screenshot
    assert frame1 is not None

    for i in range(360 * 2):
        session.run()

    frame2 = video.screenshot
    assert frame2 is not None

    geometry = video.geometry
    size = (geometry.base_width, geometry.base_height)
    image = Image.frombuffer("RGB", size, frame2)
    #image.show()
    assert len(frame1) == len(frame2)
    assert frame1 != frame2
