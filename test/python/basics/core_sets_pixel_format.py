from libretro import Session, PixelFormat, PillowVideoDriver

import prelude

video = PillowVideoDriver()

assert video.pixel_format != PixelFormat.XRGB8888

session: Session
with prelude.session() as session:
    assert session.video.pixel_format == PixelFormat.XRGB8888
