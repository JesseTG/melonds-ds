"""
Video views: letting the frontend arrange the emulated screens itself.

A frontend that presents views takes each screen out of the frame
and lays it out, shades it and shows it to each eye on its own.
The core packs its frame for that and says where the screens are;
see ``melondsds.views`` for the calls involved.
"""

from __future__ import annotations

import itertools
from collections.abc import Callable, Mapping
from ctypes import c_int
from pathlib import Path
from typing import Any

import pytest
from libretro import Content, JoypadState, Pointer, Rotation, SubsystemContent, TempDirPathDriver
from libretro.ctypes import TypedFunctionPointer

from melondsds.video import TrackingVideoDriver
from melondsds.views import FLAT, HEADSET, STEREO, View, ViewsSession, ViewsStatus

#: A factory for un-entered sessions whose frontend may present video views.
type ViewsSessionFactory = Callable[..., ViewsSession]

#: The size of one emulated screen at the native resolution.
W, H = 256, 192

#: The options that shape the core's own layouts,
#: which do nothing while the frontend arranges the screens.
OWN_LAYOUT_OPTIONS = (
    "melonds_hybrid_ratio",
    "melonds_hybrid_small_screen",
    "melonds_opengl_filtering",
    "melonds_screen_gap",
    "melonds_secondary_screen_scale",
    "melonds_secondary_screen_filtering",
)


@pytest.fixture
def views_session(
    core_path: str,
    path_driver: TempDirPathDriver,
    staged_system_files: Mapping[str, Path],  # noqa: ARG001
) -> ViewsSessionFactory:
    """Return a factory like the ``session`` fixture's, for a frontend that knows video views."""

    def make(game: Content | SubsystemContent | None = None, /, **kwargs: Any) -> ViewsSession:
        kwargs.setdefault("path", path_driver)
        return ViewsSession(core_path, game, **kwargs)

    return make


def _top(x: int, y: int, screen: int, scale: int = 1) -> View:
    """Return the top screen's view at the given position, in native pixels."""
    return View(x * scale, y * scale, W * scale, H * scale, screen)


#: The bottom screen's view looks just like the top screen's; only the name differs.
_bottom = _top


def _layout(layout: str, **options: str) -> dict[str, str]:
    """Core options that select a single screen layout."""
    return {
        "melonds_number_of_screen_layouts": "1",
        "melonds_screen_layout1": layout,
        **{f"melonds_{key}": value for key, value in options.items()},
    }


def _frame_size(emulator: ViewsSession) -> tuple[int, int]:
    """Return the size of the frame the core last sent, checked against its geometry."""
    # (as the core sent it, before the frontend rotates it)
    frame = emulator.video.screenshot(False)
    geometry = emulator.video.geometry
    assert frame is not None
    assert geometry is not None
    assert (frame.width, frame.height) == (geometry.base_width, geometry.base_height)
    return frame.width, frame.height


# --------------------------------------------------------------------------- #
# Whether the frame is packed for views, and how
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("status", "options", "views", "frame_size"),
    [
        # In the frontend's 2D mode, views only stand in for a layout they look the same as.
        pytest.param(
            FLAT,
            _layout("top-bottom"),
            (_top(0, 0, 0), _bottom(0, H, 1)),
            (W, H * 2),
            id="2d-top-bottom",
        ),
        pytest.param(
            FLAT,
            _layout("bottom-top"),
            (_bottom(0, 0, 0), _top(0, H, 1)),
            (W, H * 2),
            id="2d-bottom-top",
        ),
        pytest.param(FLAT, _layout("top"), (_top(0, 0, 0),), (W, H), id="2d-top-only"),
        pytest.param(FLAT, _layout("bottom"), (_bottom(0, 0, 0),), (W, H), id="2d-bottom-only"),
        pytest.param(FLAT, _layout("left-right"), (), (W * 2, H), id="2d-left-right"),
        pytest.param(FLAT, _layout("rotate-left"), (), (W, H * 2), id="2d-rotated"),
        pytest.param(
            FLAT, _layout("hybrid-top", hybrid_ratio="2"), (), (W * 3, H * 2), id="2d-hybrid"
        ),
        pytest.param(
            FLAT, _layout("top-bottom", screen_gap="64"), (), (W, H * 2 + 64), id="2d-screen-gap"
        ),
        pytest.param(
            FLAT,
            _layout("top-bottom", secondary_screen_scale="50"),
            (),
            (W, H + H // 2),
            id="2d-secondary-scale",
        ),
        # In its stereo modes the frontend arranges the screens whatever the layout is.
        pytest.param(
            STEREO,
            _layout("top-bottom", screen_gap="64", secondary_screen_scale="50"),
            (_top(0, 0, 0), _bottom(0, H, 1)),
            (W, H * 2),
            id="stereo-top-bottom",
        ),
        pytest.param(
            STEREO,
            _layout("left-right"),
            (_top(0, 0, 0), _bottom(W, 0, 1)),
            (W * 2, H),
            id="stereo-left-right",
        ),
        pytest.param(
            STEREO,
            _layout("right-left"),
            (_bottom(0, 0, 0), _top(W, 0, 1)),
            (W * 2, H),
            id="stereo-right-left",
        ),
        pytest.param(STEREO, _layout("top"), (_top(0, 0, 0),), (W, H), id="stereo-top-only"),
        pytest.param(
            STEREO, _layout("bottom"), (_bottom(0, 0, 0),), (W, H), id="stereo-bottom-only"
        ),
        # A layout that enlarges a screen is packed as a vertical one,
        # with the screen it focuses on first.
        pytest.param(
            STEREO,
            _layout("hybrid-top", hybrid_ratio="3"),
            (_top(0, 0, 0), _bottom(0, H, 1)),
            (W, H * 2),
            id="stereo-hybrid-top",
        ),
        pytest.param(
            STEREO,
            _layout("flipped-hybrid-bottom"),
            (_bottom(0, 0, 0), _top(0, H, 1)),
            (W, H * 2),
            id="stereo-flipped-hybrid-bottom",
        ),
        pytest.param(
            STEREO,
            _layout("largescreen-top"),
            (_top(0, 0, 0), _bottom(0, H, 1)),
            (W, H * 2),
            id="stereo-largescreen-top",
        ),
        pytest.param(
            STEREO,
            _layout("flipped-largescreen-bottom"),
            (_bottom(0, 0, 0), _top(0, H, 1)),
            (W, H * 2),
            id="stereo-flipped-largescreen-bottom",
        ),
        pytest.param(
            STEREO,
            _layout("rotate-left", screen_gap="64"),
            (_top(0, 0, 0), _bottom(0, H, 1)),
            (W, H * 2),
            id="stereo-rotated",
        ),
        pytest.param(
            HEADSET,
            _layout("left-right"),
            (_top(0, 0, 0), _bottom(W, 0, 1)),
            (W * 2, H),
            id="headset",
        ),
        # Nothing is packed for a frontend that wouldn't use it.
        pytest.param(ViewsStatus.NONE, _layout("top-bottom"), (), (W, H * 2), id="not-presenting"),
        pytest.param(
            ViewsStatus.STEREO, _layout("left-right"), (), (W * 2, H), id="stereo-without-presents"
        ),
        pytest.param(None, _layout("top-bottom"), (), (W, H * 2), id="unknown-to-frontend"),
        pytest.param(
            STEREO,
            _layout("left-right", frontend_layout="disabled"),
            (),
            (W * 2, H),
            id="option-off",
        ),
    ],
)
def test_view_map(
    views_session: ViewsSessionFactory,
    nds_rom: Path,
    status: ViewsStatus | None,
    options: dict[str, str],
    views: tuple[View, ...],
    frame_size: tuple[int, int],
) -> None:
    """The core sends the frontend a view map that matches the frame it draws."""
    with views_session(nds_rom, views_status=status, options=options) as emulator:
        for _ in range(5):
            emulator.run()

        assert emulator.views == views
        assert emulator.rejected_maps == 0
        assert _frame_size(emulator) == frame_size

        # Every view lies inside the frame it describes
        for view in emulator.views:
            assert view.x + view.width <= frame_size[0]
            assert view.y + view.height <= frame_size[1]


@pytest.mark.opengl
@pytest.mark.nds_rom
def test_view_map_follows_internal_resolution(
    views_session: ViewsSessionFactory, nds_rom: Path
) -> None:
    """The views are measured in the pixels of the frame, which the OpenGL renderer scales up."""
    options = {
        **_layout("left-right"),
        "melonds_render_mode": "opengl",
        "melonds_opengl_resolution": "2",
    }
    video = TrackingVideoDriver()

    with views_session(nds_rom, views_status=STEREO, video=video, options=options) as emulator:
        for _ in range(5):
            emulator.run()

        assert video.last_frame == "hardware"
        assert video.last_frame_size == (W * 4, H * 2)
        assert emulator.views == (_top(0, 0, 0, scale=2), _bottom(W, 0, 1, scale=2))


@pytest.mark.nds_rom
def test_view_map_is_sent_with_every_frame(
    views_session: ViewsSessionFactory, nds_rom: Path
) -> None:
    """The map goes out again with each frame, which covers a frontend that dropped it."""
    with views_session(nds_rom, views_status=FLAT, options=_layout("top-bottom")) as emulator:
        emulator.run()
        sent = len(emulator.accepted_maps)
        assert sent >= 1

        for _ in range(10):
            emulator.run()

        assert len(emulator.accepted_maps) == sent + 10
        assert set(emulator.accepted_maps) == {(_top(0, 0, 0), _bottom(0, H, 1))}


@pytest.mark.nds_rom
def test_frontend_without_views_is_asked_once(
    views_session: ViewsSessionFactory, nds_rom: Path
) -> None:
    """A frontend that doesn't know the status call isn't asked again with every frame."""
    with views_session(nds_rom, views_status=None) as emulator:
        for _ in range(5):
            emulator.run()

        queries = emulator.status_queries
        assert queries >= 1

        for _ in range(20):
            emulator.run()

        assert emulator.status_queries == queries
        assert emulator.accepted_maps == []


# --------------------------------------------------------------------------- #
# Changes while the game runs
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_view_map_follows_views_status(views_session: ViewsSessionFactory, nds_rom: Path) -> None:
    """The core takes its map back when the frontend stops presenting views, and sends it again after."""
    options = _layout("hybrid-top", hybrid_ratio="2")
    video = TrackingVideoDriver()

    with views_session(nds_rom, views_status=STEREO, video=video, options=options) as emulator:
        for _ in range(5):
            emulator.run()

        packed = (_top(0, 0, 0), _bottom(0, H, 1))
        assert emulator.views == packed
        assert video.last_frame_size == (W, H * 2)

        geometry = video.geometry
        assert geometry is not None
        max_size = (geometry.max_width, geometry.max_height)
        rebuilds = video.rebuilds

        # The player goes back to the frontend's 2D mode, where the hybrid layout is the core's own
        emulator.views_status = FLAT
        emulator.run()

        assert emulator.views == ()
        assert emulator.accepted_maps[-1] == ()
        assert video.last_frame_size == (W * 3, H * 2)

        # The empty map is sent once, not with every frame
        maps = len(emulator.accepted_maps)
        for _ in range(5):
            emulator.run()

        assert len(emulator.accepted_maps) == maps

        emulator.views_status = HEADSET
        emulator.run()

        assert emulator.views == packed
        assert video.last_frame_size == (W, H * 2)

        geometry = video.geometry
        assert geometry is not None
        assert (geometry.base_width, geometry.base_height) == (W, H * 2)

        # None of that is a reason for the frontend to start its video output over
        assert (geometry.max_width, geometry.max_height) == max_size
        assert video.rebuilds == rebuilds


@pytest.mark.nds_rom
def test_view_map_follows_layout_hotkey(views_session: ViewsSessionFactory, nds_rom: Path) -> None:
    """Cycling the screen layout repacks the frame and updates the map."""

    def generate_input():
        yield from itertools.repeat(None, 10)
        yield JoypadState(r3=True)
        yield from itertools.repeat(None, 9)
        yield JoypadState(r3=True)
        yield from itertools.repeat(None)

    options = {
        "melonds_number_of_screen_layouts": "3",
        "melonds_screen_layout1": "top-bottom",
        "melonds_screen_layout2": "largescreen-bottom",
        "melonds_screen_layout3": "bottom",
    }

    with views_session(
        nds_rom, views_status=FLAT, input=generate_input, options=options
    ) as emulator:
        for _ in range(10):
            emulator.run()

        assert emulator.views == (_top(0, 0, 0), _bottom(0, H, 1))
        assert _frame_size(emulator) == (W, H * 2)

        for _ in range(10):
            emulator.run()

        # A 2D frontend would stack the screens, which isn't what this layout looks like
        assert emulator.views == ()
        assert _frame_size(emulator) == (W * 3, H * 2)

        for _ in range(10):
            emulator.run()

        assert emulator.views == (_bottom(0, 0, 0),)
        assert _frame_size(emulator) == (W, H)


@pytest.mark.nds_rom
def test_view_map_follows_options(views_session: ViewsSessionFactory, nds_rom: Path) -> None:
    """Turning the option off hands the layout back to the core."""
    with views_session(nds_rom, views_status=STEREO, options=_layout("left-right")) as emulator:
        for _ in range(5):
            emulator.run()

        assert emulator.views == (_top(0, 0, 0), _bottom(W, 0, 1))

        emulator.options.variables["melonds_frontend_layout"] = "disabled"
        emulator.run()

        assert emulator.views == ()
        assert _frame_size(emulator) == (W * 2, H)

        emulator.options.variables["melonds_frontend_layout"] = "auto"
        emulator.run()

        assert emulator.views == (_top(0, 0, 0), _bottom(W, 0, 1))


# --------------------------------------------------------------------------- #
# Rotation and the touch screen
# --------------------------------------------------------------------------- #


def _touch(x: float, y: float, frame_size: tuple[int, int]):
    """
    Idle for a few frames, then hold the pointer at a point in the frame.

    A frontend reports a touch on any screen it shows as a point in the packed frame,
    inside that screen's view.
    """
    pointer = Pointer(
        round((x / frame_size[0] * 2 - 1) * 0x7FFF),
        round((y / frame_size[1] * 2 - 1) * 0x7FFF),
        True,
    )

    def generate_input():
        yield from itertools.repeat(None, 5)
        yield from itertools.repeat(pointer)

    return generate_input


def _touch_position(emulator: ViewsSession) -> tuple[int, int]:
    """Where the emulated touch screen is being touched, in its own pixels."""
    touch_x = emulator.get_proc_address(b"melondsds_touch_x", TypedFunctionPointer[c_int, []])
    touch_y = emulator.get_proc_address(b"melondsds_touch_y", TypedFunctionPointer[c_int, []])
    assert touch_x is not None
    assert touch_y is not None
    return touch_x(), touch_y()


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("layout", "bottom_screen", "frame_size"),
    [
        pytest.param("top-bottom", (0, H), (W, H * 2), id="top-bottom"),
        pytest.param("left-right", (W, 0), (W * 2, H), id="left-right"),
        pytest.param("hybrid-bottom", (0, 0), (W, H * 2), id="hybrid-bottom"),
        pytest.param("bottom", (0, 0), (W, H), id="bottom-only"),
    ],
)
def test_touch_lands_in_the_touch_screens_view(
    views_session: ViewsSessionFactory,
    nds_rom: Path,
    layout: str,
    bottom_screen: tuple[int, int],
    frame_size: tuple[int, int],
) -> None:
    """A touch inside the bottom screen's view touches the same spot on the emulated screen."""
    # The middle of the pixel a quarter of the way across and down the screen
    point = (bottom_screen[0] + 64.5, bottom_screen[1] + 48.5)

    with views_session(
        nds_rom, views_status=STEREO, input=_touch(*point, frame_size), options=_layout(layout)
    ) as emulator:
        for _ in range(10):
            emulator.run()

        assert _frame_size(emulator) == frame_size
        assert _touch_position(emulator) == (64, 48)


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("layout", "rotation", "touch"),
    [
        # Each expected spot is where the pixel a quarter of the way across and down
        # the screen *as displayed* ends up once the screen is turned back upright.
        pytest.param("rotate-left", Rotation.NINETY, (191, 48), id="left"),
        pytest.param("rotate-180", Rotation.ONE_EIGHTY, (191, 143), id="upside-down"),
        pytest.param("rotate-right", Rotation.TWO_SEVENTY, (64, 143), id="right"),
    ],
)
def test_rotated_views_turn_the_touch_within_the_screen(
    views_session: ViewsSessionFactory,
    nds_rom: Path,
    layout: str,
    rotation: Rotation,
    touch: tuple[int, int],
) -> None:
    """
    A frontend turns each view on its own for a rotated layout,
    so a touch turns about the middle of the touch screen rather than the middle of the frame.
    """
    frame_size = (W, H * 2)

    # The frontend reports a touch as far across the view's rectangle
    # as it is across the turned screen that the player sees.
    point = (64.5, H + 48.5)

    with views_session(
        nds_rom, views_status=STEREO, input=_touch(*point, frame_size), options=_layout(layout)
    ) as emulator:
        for _ in range(10):
            emulator.run()

        assert emulator.video.rotation == rotation
        assert emulator.views == (_top(0, 0, 0), _bottom(0, H, 1))
        assert _frame_size(emulator) == frame_size
        assert _touch_position(emulator) == touch


# --------------------------------------------------------------------------- #
# Core options
# --------------------------------------------------------------------------- #


@pytest.mark.nds_rom
def test_stereo_views_hide_the_options_they_override(
    views_session: ViewsSessionFactory, nds_rom: Path
) -> None:
    """The options that only shape the core's own layouts are hidden while they do nothing."""
    options = {
        "melonds_number_of_screen_layouts": "2",
        "melonds_screen_layout1": "top-bottom",
        "melonds_screen_layout2": "hybrid-top",
    }

    with views_session(nds_rom, views_status=FLAT, options=options) as emulator:
        visibility = emulator.options.visibility

        # In the frontend's 2D mode these decide whether the frontend arranges the screens at all
        for _ in range(5):
            emulator.run()

        for key in OWN_LAYOUT_OPTIONS:
            assert visibility[key], key

        emulator.views_status = STEREO
        emulator.run()

        for key in OWN_LAYOUT_OPTIONS:
            assert not visibility[key], key

        # The layouts themselves still choose which screens are shown and in what order
        assert visibility["melonds_number_of_screen_layouts"]
        assert visibility["melonds_screen_layout1"]
        assert visibility["melonds_frontend_layout"]

        # The frontend asks again after an option changes, before the core reads it
        emulator.options.variables["melonds_frontend_layout"] = "disabled"

        for key in OWN_LAYOUT_OPTIONS:
            assert visibility[key], key
