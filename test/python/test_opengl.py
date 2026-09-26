"""Hardware rendering: OpenGL setup, fallback and runtime renderer switching."""

from __future__ import annotations

import platform
import sys
from ctypes import c_bool
from pathlib import Path

import pytest
from libretro import ArrayVideoDriver, ModernGlVideoDriver
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory
from melondsds.video import RetroArchVideoDriver, TrackingVideoDriver

pytestmark = pytest.mark.opengl

#: On macOS x86_64 this times out.
#: The core's OpenGL rendering is correct,
#: but libretro.py's ``ModernGlVideoDriver.refresh()``
#: sizes its FBOs and textures to ``max_geometry``
#: rather than to the current ``base_geometry``.
#: The rendered content is correct
#: but occupies a tiny fraction of the oversized FBO,
#: so ``screenshot()`` reads back mostly empty pixels and the comparison fails.
#: This is a libretro.py bug, not a core bug;
#: the OpenGL renderer works fine in RetroArch on both arm64 and x86_64 macOS.
macos_fbo_bug = pytest.mark.skipif(
    sys.platform == "darwin" and platform.machine() == "x86_64",
    reason="libretro.py sizes its FBOs to max_geometry on macOS x86_64",
)

@pytest.mark.nds_rom
@pytest.mark.parametrize(
    "options",
    [
        pytest.param({"melonds_render_mode": "opengl"}, id="opengl", marks=macos_fbo_bug),
        pytest.param({}, id="software-default"),
    ],
)
def test_loads_unloads(session: SessionFactory, nds_rom: Path, options: dict[str, str]) -> None:
    """The core renders differing frames through the OpenGL video driver."""
    with session(nds_rom, video=ModernGlVideoDriver, options=options) as emulator:
        # TODO: Update SessionFactory to accept generic parameters so the returned Session type is correct
        video = emulator.video
        assert isinstance(video, ModernGlVideoDriver)

        for _ in range(70):
            emulator.run()

        frame1 = video.screenshot()
        assert frame1 is not None

        for _ in range(60):
            emulator.run()

        frame2 = video.screenshot()
        assert frame2 is not None

        assert len(frame1.data) == len(frame2.data)
        assert frame1 != frame2


@pytest.mark.nds_rom
def test_falls_back_to_software(session: SessionFactory, nds_rom: Path) -> None:
    """Requesting OpenGL with a software-only frontend falls back cleanly."""
    options = {"melonds_render_mode": "opengl"}

    with session(nds_rom, video=ArrayVideoDriver, options=options) as emulator:
        video = emulator.video
        assert isinstance(video, ArrayVideoDriver)

        is_opengl = emulator.get_proc_address(b"melondsds_is_opengl_renderer", TypedFunctionPointer[c_bool, []])
        assert is_opengl is not None

        is_software = emulator.get_proc_address(b"melondsds_is_software_renderer", TypedFunctionPointer[c_bool, []])
        assert is_software is not None

        assert is_software()
        assert not is_opengl()

        for _ in range(10):
            emulator.run()

        assert is_software()
        assert not is_opengl()


#: Switching to or from the compute renderer needs a core that offers it and a host that can run it.
compute = [pytest.mark.compute, pytest.mark.gl43]

#: The size of a 3:1 hybrid layout at 1x,
#: which is four screens wide and three screens tall;
#: no layout is wider or taller.
LARGEST_LAYOUT = (1024, 576)


@pytest.mark.nds_rom
@pytest.mark.parametrize("scale", [1, 4])
def test_max_geometry_follows_internal_resolution(
    session: SessionFactory, nds_rom: Path, scale: int
) -> None:
    """
    The maximum geometry is as large as the internal resolution needs, and no larger.

    The frontend sizes its video output for the maximum geometry,
    so asking for 8x when the player wants 1x would waste video memory.
    """
    options = {"melonds_render_mode": "opengl", "melonds_opengl_resolution": str(scale)}
    with session(nds_rom, video=ModernGlVideoDriver, options=options) as emulator:
        emulator.run()

        geometry = emulator.video.geometry
        assert geometry is not None
        assert (geometry.max_width, geometry.max_height) == (
            LARGEST_LAYOUT[0] * scale,
            LARGEST_LAYOUT[1] * scale,
        )


@pytest.mark.nds_rom
def test_changing_internal_resolution_rebuilds_the_video_driver(
    session: SessionFactory, nds_rom: Path
) -> None:
    """
    A new internal resolution gets a video driver sized for it, larger or smaller.

    Reporting a new maximum geometry is what makes the frontend rebuild its video driver.
    """
    video = TrackingVideoDriver()
    options = {"melonds_render_mode": "opengl", "melonds_opengl_resolution": "1"}

    with session(nds_rom, video=video, options=options) as emulator:
        for _ in range(3):
            emulator.run()

        for scale in (2, 1):
            rebuilds = video.rebuilds
            emulator.options.variables["melonds_opengl_resolution"] = str(scale).encode()

            for _ in range(3):
                emulator.run()

            assert video.rebuilds == rebuilds + 1

            geometry = video.geometry
            assert geometry is not None
            assert (geometry.max_width, geometry.max_height) == (
                LARGEST_LAYOUT[0] * scale,
                LARGEST_LAYOUT[1] * scale,
            )

            # The core kept rendering, at the new size
            assert video.last_frame == "hardware"
            assert video.last_frame_size == (geometry.base_width, geometry.base_height)


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    "frontend",
    [
        pytest.param(TrackingVideoDriver, id="libretro-py"),
        pytest.param(RetroArchVideoDriver, id="retroarch"),
    ],
)
@pytest.mark.parametrize(
    ("start", "sequence"),
    [
        pytest.param("software", ("opengl",), id="sw-to-gl"),
        pytest.param("opengl", ("software",), id="gl-to-sw"),
        pytest.param("software", ("opengl", "software"), id="sw-to-gl-to-sw"),
        pytest.param("opengl", ("software", "opengl"), id="gl-to-sw-to-gl"),
        pytest.param("software", ("compute",), id="sw-to-compute", marks=compute),
        pytest.param("compute", ("software",), id="compute-to-sw", marks=compute),
        pytest.param("opengl", ("compute",), id="gl-to-compute", marks=compute),
        pytest.param("compute", ("opengl",), id="compute-to-gl", marks=compute),
        pytest.param("compute", ("opengl", "compute"), id="compute-to-gl-to-compute", marks=compute),
        pytest.param("software", ("compute", "software"), id="sw-to-compute-to-sw", marks=compute),
    ],
)
def test_render_mode_switch(
    session: SessionFactory,
    nds_rom: Path,
    start: str,
    sequence: tuple[str, ...],
    frontend: type[TrackingVideoDriver],
) -> None:
    """
    The renderer can be swapped at runtime, in any direction, repeatedly.

    The ``retroarch`` cases only rebuild the video driver
    when the maximum geometry changes,
    which it doesn't when switching renderers at 1x;
    the core has to change it anyway, or it never gets the context it asked for.
    """
    video = frontend()
    with session(nds_rom, video=video, options={"melonds_render_mode": start}) as emulator:
        is_opengl = emulator.get_proc_address(b"melondsds_is_opengl_renderer", TypedFunctionPointer[c_bool, []])
        assert is_opengl is not None

        is_software = emulator.get_proc_address(b"melondsds_is_software_renderer", TypedFunctionPointer[c_bool, []])
        assert is_software is not None

        is_compute = emulator.get_proc_address(b"melondsds_is_compute_renderer", TypedFunctionPointer[c_bool, []])
        assert is_compute is not None

        probes = {"opengl": is_opengl, "software": is_software, "compute": is_compute}

        assert probes[start]()

        for mode in sequence:
            for _ in range(3):
                emulator.run()

            emulator.options.variables["melonds_render_mode"] = mode.encode()
            frames = video.frames

            for _ in range(3):
                emulator.run()

            assert probes[mode]()

            # The core kept rendering, and with the renderer it switched to
            assert video.frames > frames, f"The core stopped rendering after switching to {mode}"
            assert video.last_frame == ("software" if mode == "software" else "hardware")


@pytest.mark.compute
@pytest.mark.nds_rom
def test_switching_to_compute_on_an_old_context_falls_back(
    session: SessionFactory, nds_rom: Path
) -> None:
    """
    Switching to the compute renderer on a context too old for it ends up in software mode.

    The core has to ask for a newer context,
    which RetroArch only creates by rebuilding its video driver,
    which it only does when the maximum geometry changes.
    The internal resolution is the same, so the core has to change the geometry itself.
    This frontend then provides another context that's too old,
    so the core should fall back to software rendering
    instead of waiting forever for one that can run the compute renderer.
    """
    video = RetroArchVideoDriver(gl_version=(3, 3))
    with session(nds_rom, video=video, options={"melonds_render_mode": "opengl"}) as emulator:
        assert video.context is not None
        if video.context.version_code >= 430:
            pytest.skip(
                f"This OpenGL implementation returned {video.context.version_code} "
                "for a 3.3 request, so the context isn't too old after all"
            )

        is_software = emulator.get_proc_address(
            b"melondsds_is_software_renderer", TypedFunctionPointer[c_bool, []]
        )
        assert is_software is not None

        for _ in range(3):
            emulator.run()

        emulator.options.variables["melonds_render_mode"] = b"compute"

        for _ in range(10):
            emulator.run()

        assert is_software()
        assert video.last_frame == "software"


@pytest.mark.no_skip_error_screen
@pytest.mark.parametrize(
    "mode",
    [
        pytest.param("opengl", id="opengl"),
        pytest.param("compute", id="compute", marks=compute),
    ],
)
def test_error_screen_does_not_crash(session: SessionFactory, mode: str) -> None:
    """
    The in-core error screen renders without crashing when an OpenGL renderer is configured.

    See https://github.com/JesseTG/melonds-ds/issues/155
    """
    with session(options={"melonds_render_mode": mode}) as emulator:
        for _ in range(300):
            emulator.run()
