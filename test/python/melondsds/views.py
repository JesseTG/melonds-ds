"""
A frontend that answers RetroArch's video views calls.

A core tells a frontend where each emulated screen is in its frame
with ``RETRO_ENVIRONMENT_SET_VIDEO_VIEWS``,
and asks whether the frontend would use that
with ``RETRO_ENVIRONMENT_GET_VIDEO_VIEWS_STATUS``.
libretro.py doesn't know either call yet,
so this session answers them itself.
"""

from __future__ import annotations

from ctypes import POINTER, Structure, c_float, c_int, c_uint, c_void_p, cast
from dataclasses import dataclass
from enum import IntFlag
from typing import Any, override

from libretro import Session

_EXPERIMENTAL = 0x10000

#: ``RETRO_ENVIRONMENT_SET_VIDEO_VIEWS``
SET_VIDEO_VIEWS = 95 | _EXPERIMENTAL

#: ``RETRO_ENVIRONMENT_GET_VIDEO_VIEWS_STATUS``
GET_VIDEO_VIEWS_STATUS = 96 | _EXPERIMENTAL

#: ``RETRO_VIDEO_VIEWS_MAX``
VIEWS_MAX = 8

#: ``RETRO_VIDEO_VIEW_EYE_NONE``
EYE_NONE = 0


class ViewsStatus(IntFlag):
    """The ``RETRO_VIDEO_VIEWS_STATUS_`` flags."""

    NONE = 0

    #: The frontend would lay out a map sent now.
    PRESENTS = 1 << 0

    #: Both eyes are shown. Only set together with :attr:`PRESENTS`.
    STEREO = 1 << 1

    #: A headset is tracking. Only set together with :attr:`PRESENTS` and :attr:`STEREO`.
    HMD = 1 << 2


#: What a frontend in its 2D mode answers.
FLAT = ViewsStatus.PRESENTS

#: What a frontend in one of its stereo 3D modes answers.
STEREO = ViewsStatus.PRESENTS | ViewsStatus.STEREO

#: What a frontend showing a headset answers.
HEADSET = ViewsStatus.PRESENTS | ViewsStatus.STEREO | ViewsStatus.HMD


class retro_video_view(Structure):
    """Matches ``struct retro_video_view``."""

    _fields_ = (
        ("x", c_uint),
        ("y", c_uint),
        ("width", c_uint),
        ("height", c_uint),
        ("screen", c_uint),
        ("eye", c_uint),
        ("aspect_ratio", c_float),
    )


class retro_video_views(Structure):
    """Matches ``struct retro_video_views``."""

    _fields_ = (
        ("views", POINTER(retro_video_view)),
        ("num_views", c_uint),
        ("flags", c_uint),
        ("reference_space", c_int),
        ("recommended_view_width", c_uint),
        ("recommended_view_height", c_uint),
    )


@dataclass(frozen=True, slots=True)
class View:
    """One screen's rectangle in the frame, in pixels from the top left."""

    x: int
    y: int
    width: int
    height: int

    #: Screens are numbered from 0, and screen 0 is the primary.
    screen: int

    eye: int = EYE_NONE

    #: At or below zero means ``width / height``.
    aspect_ratio: float = 0.0


def _valid(views: tuple[View, ...]) -> bool:
    """Apply the rules RetroArch's ``video_views_validate`` does."""
    if not views:
        # An empty map clears the one the frontend holds
        return True

    if len(views) > VIEWS_MAX:
        return False

    eyes: dict[int, list[int]] = {}
    for view in views:
        if view.width == 0 or view.height == 0:
            return False

        if view.screen >= VIEWS_MAX or view.eye not in (0, 1, 2):
            return False

        if view.aspect_ratio > 0 and not 0.01 <= view.aspect_ratio <= 100:
            return False

        eyes.setdefault(view.screen, []).append(view.eye)

    # Screens number from 0 with no gaps,
    # and each is either one view or a left/right pair
    return all(sorted(eyes.get(screen, ())) in ([0], [1, 2]) for screen in range(max(eyes) + 1))


class ViewsSession(Session):
    """A session whose frontend may present video views."""

    def __init__(
        self, *args: Any, views_status: ViewsStatus | None = ViewsStatus.NONE, **kwargs: Any
    ) -> None:
        """
        Accept the same arguments as :class:`~libretro.Session`.

        :param views_status: What the frontend answers the status call with,
            or :obj:`None` for a frontend that doesn't know either call.
        """
        super().__init__(*args, **kwargs)

        #: What the frontend answers the status call with.
        #: Tests may change it between frames, as a player changing the stereo mode would.
        self.views_status = views_status

        #: The map the frontend holds; empty if the core never sent one or took it back.
        self.views: tuple[View, ...] = ()

        #: Every map the frontend accepted, in order.
        self.accepted_maps: list[tuple[View, ...]] = []

        #: How many maps the frontend turned down.
        self.rejected_maps = 0

        #: How many times the core asked for the status.
        self.status_queries = 0

    @override
    def environment(self, cmd: int, data: c_void_p) -> bool:
        if cmd == GET_VIDEO_VIEWS_STATUS:
            return self._get_video_views_status(data)

        if cmd == SET_VIDEO_VIEWS:
            return self._set_video_views(data)

        return super().environment(cmd, data)

    def _get_video_views_status(self, data: c_void_p) -> bool:
        self.status_queries += 1
        if self.views_status is None or not data:
            return False

        cast(data, POINTER(c_uint))[0] = int(self.views_status)
        return True

    def _set_video_views(self, data: c_void_p) -> bool:
        if self.views_status is None or not data:
            return False

        arg = cast(data, POINTER(retro_video_views))[0]
        if arg.num_views and not arg.views:
            self.rejected_maps += 1
            return False

        views = tuple(
            View(
                view.x,
                view.y,
                view.width,
                view.height,
                view.screen,
                view.eye,
                view.aspect_ratio,
            )
            for view in (arg.views[i] for i in range(min(arg.num_views, VIEWS_MAX + 1)))
        )

        if arg.num_views > VIEWS_MAX or not _valid(views):
            self.rejected_maps += 1
            return False

        self.views = views
        self.accepted_maps.append(views)
        return True


__all__ = [
    "EYE_NONE",
    "FLAT",
    "HEADSET",
    "STEREO",
    "View",
    "ViewsSession",
    "ViewsStatus",
]
