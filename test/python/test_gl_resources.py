"""
What the core does to OpenGL objects that belong to the frontend.

A libretro frontend draws its own menus and overlays
in the same OpenGL context that it lends the core,
so every object the core deletes is one the frontend might still be using.
The core has to be especially careful when it stops rendering with OpenGL:
saying so lets the frontend destroy the context and build a new one,
and object names in the new context start over from the beginning.
Anything melonDS deletes after that point
lands on whatever the frontend has since created.

These tests give the core a frontend that keeps objects of its own
and check that they're still intact once the core is done.
"""

from __future__ import annotations

from pathlib import Path
from typing import Any, override

import pytest
from libretro import ModernGlVideoDriver
from OpenGL import GL

from melondsds import SessionFactory

pytestmark = pytest.mark.opengl

#: Marks for the cases that need melonDS's compute renderer, which needs OpenGL 4.3.
COMPUTE = (pytest.mark.compute, pytest.mark.gl43)

#: Contents of the frontend's stand-in buffer.
BUFFER_CONTENTS = bytes(range(64))

#: Dimensions of the frontend's stand-in texture.
TEXTURE_SIZE = (4, 4)

#: Contents of the frontend's stand-in texture, as RGBA8.
TEXTURE_CONTENTS = bytes(range(TEXTURE_SIZE[0] * TEXTURE_SIZE[1] * 4))

#: Dimensions of the frame that :func:`assert_frontend_can_still_draw` puts through the driver.
PROBE_FRAME_SIZE = (64, 64)


class FrontendWithResources(ModernGlVideoDriver):
    """
    A video driver that keeps OpenGL objects of its own, the way a real frontend does.

    RetroArch fills the context it lends the core
    with the textures, buffers and programs that draw its menu and its overlays.
    This allocates a stand-in for those every time it builds a context,
    which is when a frontend's own resources first appear in a new one.
    """

    def __init__(self, **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self.frontend_buffer: Any = None
        self.frontend_texture: Any = None
        self.frontend_names: dict[str, int] = {}

    @override
    def reinit(self) -> None:
        # These belong to the context that's about to be released,
        # so let go of them while it's still there to release them into.
        self.frontend_buffer = None
        self.frontend_texture = None
        self.frontend_names = {}

        super().reinit()

        context = self.context
        assert context is not None, "reinit() always leaves a context behind"

        self.frontend_buffer = context.buffer(BUFFER_CONTENTS)
        self.frontend_texture = context.texture(TEXTURE_SIZE, 4, TEXTURE_CONTENTS)
        self.frontend_buffer.label = "Frontend Buffer"
        self.frontend_texture.label = "Frontend Texture"
        self.frontend_names = {
            "buffer": self.frontend_buffer.glo,
            "texture": self.frontend_texture.glo,
        }


def assert_resources_intact(video: FrontendWithResources) -> None:
    """Assert that the core neither deleted nor overwrote the frontend's own objects."""
    buffer = video.frontend_buffer
    texture = video.frontend_texture
    assert buffer is not None and texture is not None, "The driver never built its context"

    # A deleted name stops being a buffer or a texture,
    # even if something else claims it afterwards.
    assert GL.glIsBuffer(video.frontend_names["buffer"]), "The core deleted the frontend's buffer"
    assert GL.glIsTexture(video.frontend_names["texture"]), "The core deleted the frontend's texture"

    # ...and something else claiming it would show up here.
    assert buffer.read() == BUFFER_CONTENTS, "The core overwrote the frontend's buffer"
    assert texture.read() == TEXTURE_CONTENTS, "The core overwrote the frontend's texture"


def assert_frontend_can_still_draw(video: FrontendWithResources) -> None:
    """
    Assert that the video driver's own OpenGL objects still work.

    :class:`ModernGlVideoDriver` builds a texture, a shader program, a vertex array
    and a framebuffer at the start of every context it creates,
    which is exactly where a stale delete from the core would land.
    This draws a frame of our own through all of them and reads the result back,
    so the check doesn't depend on what the game happens to be showing.
    """
    width, height = PROBE_FRAME_SIZE
    pixel_size = video.pixel_format.bytes_per_pixel

    # One color per row, so a frame that arrives intact can't be mistaken for a blank one
    frame = bytearray()
    for row in range(height):
        frame += bytes([row, 255 - row, (row * 5) % 256, 255][:pixel_size]) * width

    video.refresh(memoryview(frame), width, height, width * pixel_size)

    shot = video.screenshot()
    assert shot is not None, "The frontend couldn't read back the frame it had just drawn"
    assert len(set(bytes(shot.data))) > 1, (
        "The frame the frontend drew came back as one flat color, "
        "so its own OpenGL objects no longer work"
    )


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("mode", "frames"),
    [
        # melonDS's legacy renderer creates its OpenGL objects when the context is reset
        # and has used all of them within its first few frames.
        # After that, every frame makes the same calls on the same objects
        # until the game draws textured 3D graphics (which fill melonDS's texture cache),
        # so rendering more frames only makes the test slower.
        # Each frame is expensive without a real GPU,
        # because melonDS composites both screens in a large fragment shader.
        pytest.param("opengl", 10, id="opengl"),
        # melonDS's compute renderer compiles its programs a few at a time,
        # within a per-frame time budget,
        # so it can take many more frames to create all of its objects.
        pytest.param("compute", 60, id="compute", marks=COMPUTE),
    ],
)
def test_rendering_leaves_the_frontends_objects_alone(
    session: SessionFactory, nds_rom: Path, mode: str, frames: int
) -> None:
    """Rendering frame after frame doesn't touch objects the frontend made for itself."""
    video = FrontendWithResources()
    with session(nds_rom, video=video, options={"melonds_render_mode": mode}) as emulator:
        for _ in range(frames):
            emulator.run()

        assert_resources_intact(video)
        assert_frontend_can_still_draw(video)


@pytest.mark.nds_rom
@pytest.mark.parametrize(
    ("start", "end"),
    [
        pytest.param("compute", "software", id="compute-to-software", marks=COMPUTE),
        pytest.param("opengl", "software", id="opengl-to-software"),
        pytest.param("compute", "opengl", id="compute-to-opengl", marks=COMPUTE),
        pytest.param("opengl", "compute", id="opengl-to-compute", marks=COMPUTE),
    ],
)
def test_switching_render_modes_leaves_the_frontends_objects_alone(
    session: SessionFactory, nds_rom: Path, start: str, end: str
) -> None:
    """
    Changing the render mode mid-game doesn't take the frontend's objects with it.

    Reporting the new mode's geometry is what makes the frontend rebuild its context,
    and the frontend does that from inside the call,
    so melonDS's OpenGL renderer has to have let go of its objects by the time it returns.
    Otherwise it deletes them out of a context that has since handed those same names
    to whatever the frontend built to replace its own.
    """
    video = FrontendWithResources()
    with session(nds_rom, video=video, options={"melonds_render_mode": start}) as emulator:
        for _ in range(30):
            emulator.run()

        assert_resources_intact(video)

        emulator.options.variables["melonds_render_mode"] = end.encode()

        for _ in range(30):
            emulator.run()

        assert_resources_intact(video)
        assert_frontend_can_still_draw(video)
