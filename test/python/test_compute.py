"""
melonDS's compute-shader renderer: rendering, fallbacks, and settings changes.

The compute renderer needs an OpenGL 4.3 core context,
so every test here needs a core built with ``ENABLE_COMPUTE_RENDERER``
and most need a host that can provide that context.
The fallback tests instead check what the core does when it *can't* get one.
"""

from __future__ import annotations

import time
from collections.abc import Mapping
from ctypes import c_bool
from pathlib import Path

import pytest
from libretro import (
    ArrayVideoDriver,
    LoggerMessageDriver,
    MessageType,
    ModernGlVideoDriver,
    Session,
)
from libretro.api import retro_message_ext
from libretro.ctypes import TypedFunctionPointer

from melondsds import SessionFactory

pytestmark = [pytest.mark.opengl, pytest.mark.compute]

COMPUTE = {"melonds_render_mode": "compute"}

#: The OpenGL version the compute renderer needs, as moderngl spells it.
GL_43 = 430


def _probes(emulator: Session) -> Mapping[str, TypedFunctionPointer[c_bool, []]]:
    """Fetch the core's test hooks that report which renderer is active."""
    probes: dict[str, TypedFunctionPointer[c_bool, []]] = {}
    for mode in ("software", "opengl", "compute"):
        probe = emulator.get_proc_address(
            f"melondsds_is_{mode}_renderer".encode(), TypedFunctionPointer[c_bool, []]
        )
        assert probe is not None, f"The core doesn't export melondsds_is_{mode}_renderer"
        probes[mode] = probe

    return probes


def _active_mode(probes: Mapping[str, TypedFunctionPointer[c_bool, []]]) -> str:
    """Return the one render mode the core says is active."""
    active = [mode for mode, probe in probes.items() if probe()]
    assert len(active) == 1, f"Exactly one renderer should be active, got {active}"
    return active[0]


@pytest.mark.gl43
@pytest.mark.nds_rom
def test_renders(session: SessionFactory, nds_rom: Path) -> None:
    """The compute renderer produces differing frames through the OpenGL video driver."""
    with session(nds_rom, video=ModernGlVideoDriver, options=COMPUTE) as emulator:
        video = emulator.video
        assert isinstance(video, ModernGlVideoDriver)
        probes = _probes(emulator)

        for _ in range(70):
            emulator.run()

        assert _active_mode(probes) == "compute"
        frame1 = video.screenshot()
        assert frame1 is not None

        for _ in range(60):
            emulator.run()

        frame2 = video.screenshot()
        assert frame2 is not None

        assert len(frame1.data) == len(frame2.data)
        assert frame1 != frame2
        assert _active_mode(probes) == "compute"


@pytest.mark.nds_rom
def test_falls_back_without_hw_render(session: SessionFactory, nds_rom: Path) -> None:
    """Requesting the compute renderer from a software-only frontend falls back cleanly."""
    with session(nds_rom, video=ArrayVideoDriver, options=COMPUTE) as emulator:
        assert isinstance(emulator.video, ArrayVideoDriver)
        probes = _probes(emulator)

        assert _active_mode(probes) == "software"

        for _ in range(10):
            emulator.run()

        assert _active_mode(probes) == "software"


@pytest.mark.nds_rom
def test_falls_back_when_frontend_refuses_4_3(session: SessionFactory, nds_rom: Path) -> None:
    """
    A frontend that refuses the 4.3 context request gets software rendering.

    The video driver's cap makes ``SET_HW_RENDER`` return false,
    the way a frontend on hardware that stops at 4.1 would.
    """
    video = ModernGlVideoDriver(max_gl_version=(4, 1))
    with session(nds_rom, video=video, options=COMPUTE) as emulator:
        probes = _probes(emulator)
        assert _active_mode(probes) == "software"

        for _ in range(10):
            emulator.run()

        assert _active_mode(probes) == "software"


@pytest.mark.nds_rom
def test_legacy_opengl_unaffected_by_cap(session: SessionFactory, nds_rom: Path) -> None:
    """The same 4.1 cap still lets the legacy OpenGL renderer through, since it only asks for 3.2."""
    video = ModernGlVideoDriver(max_gl_version=(4, 1))
    with session(nds_rom, video=video, options={"melonds_render_mode": "opengl"}) as emulator:
        probes = _probes(emulator)

        for _ in range(10):
            emulator.run()

        assert _active_mode(probes) == "opengl"


@pytest.mark.gl43
@pytest.mark.nds_rom
def test_falls_back_when_context_is_too_old(session: SessionFactory, nds_rom: Path) -> None:
    """
    A frontend that agrees to 4.3 but delivers an older context gets software rendering.

    RetroArch answers ``SET_HW_RENDER`` without checking the version
    and may hand back whatever its context driver managed to create,
    so the core has to check the context it actually got.
    Whether the host honors an exact 4.1 request is up to its OpenGL implementation;
    this skips rather than fails when it doesn't.
    """
    video = ModernGlVideoDriver(gl_version=(4, 1))
    with session(nds_rom, video=video, options=COMPUTE) as emulator:
        assert video.context is not None
        if video.context.version_code >= GL_43:
            pytest.skip(
                f"This OpenGL implementation returned {video.context.version_code} "
                "for a 4.1 request, so the context isn't too old after all"
            )

        probes = _probes(emulator)

        for _ in range(10):
            emulator.run()

        assert _active_mode(probes) == "software"
        assert video.screenshot() is not None


@pytest.mark.gl43
@pytest.mark.nds_rom
def test_settings_changes_recompile_shaders(session: SessionFactory, nds_rom: Path) -> None:
    """
    Changing the resolution or hi-res coordinates mid-run rebuilds the compute renderer's shaders.

    Both settings make melonDS throw away every shader program and queue them all for recompiling,
    which the core has to finish before the next frame or melonDS asserts.
    """
    with session(nds_rom, video=ModernGlVideoDriver, options=COMPUTE) as emulator:
        video = emulator.video
        assert isinstance(video, ModernGlVideoDriver)
        probes = _probes(emulator)

        for _ in range(10):
            emulator.run()

        frame1 = video.screenshot()
        assert frame1 is not None

        emulator.options.variables["melonds_opengl_resolution"] = b"2"
        for _ in range(10):
            emulator.run()

        frame2 = video.screenshot()
        assert frame2 is not None
        assert frame2.width * frame2.height > frame1.width * frame1.height

        emulator.options.variables["melonds_compute_hires_coordinates"] = b"disabled"
        for _ in range(10):
            emulator.run()

        assert video.screenshot() is not None
        assert _active_mode(probes) == "compute"


def _progress_messages(driver: LoggerMessageDriver) -> list[retro_message_ext]:
    """Every progress-type message the core has sent."""
    return [m for m in driver.message_exts if m.type == MessageType.PROGRESS]


@pytest.mark.gl43
@pytest.mark.nds_rom
def test_reports_shader_compilation_progress(session: SessionFactory, nds_rom: Path) -> None:
    """
    The core reports how far along it is while compiling the compute renderer's shaders.

    It sends one report per frame it spends compiling,
    so how many arrive depends on how fast the host builds shaders.
    A machine whose OpenGL driver caches compiled programs
    can get through all of them in a single frame,
    so this checks the shape of the reports rather than how many there are.
    """
    driver = LoggerMessageDriver(1)
    options = {"melonds_render_mode": "compute", "melonds_opengl_resolution": "4"}

    with session(nds_rom, video=ModernGlVideoDriver, message=driver, options=options) as emulator:
        for _ in range(120):
            emulator.run()

        assert _active_mode(_probes(emulator)) == "compute"

    progress = _progress_messages(driver)
    assert progress, "The core never reported shader compilation progress"

    percentages = [m.progress for m in progress]
    assert all(0 <= p <= 100 for p in percentages)
    assert percentages == sorted(percentages), "Progress went backwards"
    assert percentages[-1] == 100, "The core never reported that it had finished"
    assert all(m.msg is not None and b"Compiling shaders" in m.msg for m in progress)


@pytest.mark.gl43
@pytest.mark.nds_rom
def test_shader_compilation_does_not_stall_a_single_frame(
    session: SessionFactory, nds_rom: Path
) -> None:
    """
    No single frame takes anywhere near the whole shader compilation.

    This is the regression test for the freeze that compiling everything at once caused;
    the budget is a sixth of a second per frame,
    with an overrun of however long the slowest single shader takes.
    """
    options = {"melonds_render_mode": "compute", "melonds_opengl_resolution": "4"}

    with session(
        nds_rom, video=ModernGlVideoDriver, message=LoggerMessageDriver(1), options=options
    ) as emulator:
        durations: list[float] = []
        for _ in range(120):
            start = time.perf_counter()
            emulator.run()
            durations.append(time.perf_counter() - start)

    assert max(durations) < 1.5, (
        f"A single frame took {max(durations):.2f}s; "
        "shader compilation isn't being spread across frames"
    )
