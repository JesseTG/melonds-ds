from typing import cast
from libretro import ModernGlVideoDriver

import prelude


class FixedGlVideoDriver(ModernGlVideoDriver):
    """Provides a screenshot method that reads directly from the hw_render_fbo
    with the correct viewport, working around a libretro.py bug where
    copy_framebuffer + shader re-render loses content when
    max_geometry >> base_geometry (the rendered content occupies a tiny
    fraction of the oversized FBO texture)."""

    def read_hw_frame(self):
        geo = self._system_av_info.geometry
        return bytes(self._hw_render_fbo.read(
            viewport=(0, 0, geo.base_width, geo.base_height),
            components=4,
        ))


with prelude.builder().with_video(FixedGlVideoDriver).build() as session:
    video = cast(FixedGlVideoDriver, session.video)

    assert isinstance(video, ModernGlVideoDriver), f"Expected ModernGlVideoDriver, got {type(video).__name__}"

    for i in range(70):
        session.run()

    frame1 = video.read_hw_frame()
    assert frame1 is not None
    assert len(frame1) > 0

    # Use an odd number of frames so we don't land on the same phase
    # of the DS's 2-frame rendering cycle
    for i in range(361):
        session.run()

    frame2 = video.read_hw_frame()
    assert frame2 is not None

    assert len(frame1) == len(frame2)
    assert frame1 != frame2
