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

pytestmark = pytest.mark.opengl

#: On macOS x86_64 this times out.
#: The core's OpenGL rendering is correct,
#: but libretro.py's ``ModernGlVideoDriver.refresh()``
#: sizes its FBOs and textures to ``max_geometry`` (8198x4608)
#: rather than to the current ``base_geometry`` (256x384).
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
        #pytest.param({"melonds_render_mode": "opengl"}, id="opengl", marks=macos_fbo_bug),
        pytest.param({"melonds_render_mode": "opengl"}, id="opengl"),
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


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("start", "sequence"),
    [
        pytest.param("software", ("opengl",), id="sw-to-gl"),
        pytest.param("opengl", ("software",), id="gl-to-sw"),
        pytest.param("software", ("opengl", "software"), id="sw-to-gl-to-sw"),
        pytest.param("opengl", ("software", "opengl"), id="gl-to-sw-to-gl"),
    ],
)
def test_render_mode_switch(
    session: SessionFactory, nds_rom: Path, start: str, sequence: tuple[str, ...]
) -> None:
    """The renderer can be swapped at runtime, in either direction, repeatedly."""
    with session(nds_rom, options={"melonds_render_mode": start}) as emulator:
        is_opengl = emulator.get_proc_address(b"melondsds_is_opengl_renderer", TypedFunctionPointer[c_bool, []])
        assert is_opengl is not None

        is_software = emulator.get_proc_address(b"melondsds_is_software_renderer", TypedFunctionPointer[c_bool, []])
        assert is_software is not None

        probes = {"opengl": is_opengl, "software": is_software}

        assert probes[start]()

        for mode in sequence:
            for _ in range(3):
                emulator.run()

            emulator.options.variables["melonds_render_mode"] = mode.encode()

            for _ in range(3):
                emulator.run()

            assert probes[mode]()


@pytest.mark.no_skip_error_screen
def test_error_screen_does_not_crash(session: SessionFactory) -> None:
    """
    The in-core error screen renders without crashing under OpenGL.

    See https://github.com/JesseTG/melonds-ds/issues/155
    """
    with session(options={"melonds_render_mode": "opengl"}) as emulator:
        for _ in range(300):
            emulator.run()
