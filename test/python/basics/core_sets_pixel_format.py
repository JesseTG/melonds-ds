from libretro import Session, PixelFormat, ArrayVideoDriver

import prelude

video = ArrayVideoDriver()

assert video.pixel_format != PixelFormat.XRGB8888

session: Session
with prelude.session() as session:
    assert session.video.pixel_format == PixelFormat.XRGB8888
