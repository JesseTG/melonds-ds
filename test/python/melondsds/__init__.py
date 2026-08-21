"""Helpers shared by the melonDS DS test suite."""

from __future__ import annotations

from collections.abc import Callable

from libretro import Session

#: What the ``session`` fixture returns: a factory for un-entered sessions.
#:
#: The first positional argument is the content to load,
#: and every keyword argument is forwarded to :py:meth:`libretro.Session.__init__`.
type SessionFactory = Callable[..., Session]

__all__ = ["SessionFactory"]
