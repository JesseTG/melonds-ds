from sys import argv
from libretro import default_session
from libretro.defs import PixelFormat

import prelude

with default_session(prelude.core_path) as session:
    assert session.video.pixel_format == PixelFormat.XRGB8888