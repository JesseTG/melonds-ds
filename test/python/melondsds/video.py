"""
Video drivers that keep track of what the core does with them.

A hardware-rendering core gets a new OpenGL context
whenever the frontend rebuilds its video driver,
and the core decides when that happens through the maximum geometry it reports.
These drivers make that visible to tests.
"""

from __future__ import annotations

from typing import Any, Literal, override

from libretro import FrameBufferSpecial, ModernGlVideoDriver, retro_system_av_info

#: What kind of frame the core sent through ``retro_video_refresh_t``.
type FrameKind = Literal["hardware", "software", "dupe"]


class TrackingVideoDriver(ModernGlVideoDriver):
    """A video driver that counts its rebuilds and the frames it receives."""

    def __init__(self, **kwargs: Any) -> None:
        """Accept the same arguments as :class:`~libretro.ModernGlVideoDriver`."""
        super().__init__(**kwargs)

        #: How many times this driver has been built, including the first.
        #: Each build gives a hardware-rendering core a new context.
        self.rebuilds = 0

        #: How many frames the core has sent, dupes included.
        self.frames = 0

        #: What kind of frame the core sent most recently.
        self.last_frame: FrameKind | None = None

        #: The size of the most recent frame that wasn't a dupe, in pixels.
        self.last_frame_size: tuple[int, int] | None = None

    @override
    def reinit(self) -> None:
        self.rebuilds += 1
        super().reinit()

    @override
    def refresh(
        self, data: memoryview[int] | FrameBufferSpecial, width: int, height: int, pitch: int
    ) -> None:
        super().refresh(data, width, height, pitch)

        self.frames += 1
        match data:
            case FrameBufferSpecial.HARDWARE:
                self.last_frame = "hardware"
            case FrameBufferSpecial.DUPE:
                self.last_frame = "dupe"
                return
            case _:
                self.last_frame = "software"

        self.last_frame_size = (width, height)


class RetroArchVideoDriver(TrackingVideoDriver):
    """
    A video driver that rebuilds itself only when the maximum geometry changes,
    the way RetroArch does.

    libretro.py's drivers rebuild themselves
    for every ``RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO``,
    which hides a core that asks for a new context
    without changing its maximum geometry.
    RetroArch would never create that context,
    and the core would wait for it forever.
    """

    @property
    @override
    def system_av_info(self) -> retro_system_av_info | None:
        return super().system_av_info

    @system_av_info.setter
    @override
    def system_av_info(self, av_info: retro_system_av_info) -> None:
        current = self.system_av_info
        if (
            current is not None
            and self.context is not None
            and current.geometry.max_width == av_info.geometry.max_width
            and current.geometry.max_height == av_info.geometry.max_height
        ):
            # RetroArch keeps its video driver as it is
            # and only adopts the new base geometry.
            self.geometry = av_info.geometry
            return

        setter = ModernGlVideoDriver.system_av_info.fset
        assert setter is not None
        setter(self, av_info)
