from ctypes import CFUNCTYPE, c_bool
from typing import cast
from libretro import ArrayVideoDriver

import prelude

options = {
    b"melonds_render_mode": b"opengl",
}

with prelude.builder().with_video(ArrayVideoDriver).with_options(options).build() as session:
    video = cast(ArrayVideoDriver, session.video)
    assert isinstance(video, ArrayVideoDriver), f"Expected ArrayVideoDriver, got {type(video).__name__}"

    is_opengl_renderer = session.get_proc_address(b"melondsds_is_opengl_renderer", CFUNCTYPE(c_bool))
    assert is_opengl_renderer is not None, "melondsds_is_opengl_renderer not defined in the core"

    is_software_renderer = session.get_proc_address(b"melondsds_is_software_renderer", CFUNCTYPE(c_bool))
    assert is_software_renderer is not None, "melondsds_is_software_renderer not defined in the core"

    assert is_software_renderer()
    assert not is_opengl_renderer()

    for i in range(10):
        session.run()

    assert is_software_renderer()
    assert not is_opengl_renderer()
