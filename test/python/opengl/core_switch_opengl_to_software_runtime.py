from ctypes import CFUNCTYPE, c_bool

import prelude

options = {
    b"melonds_render_mode": b"opengl",
}

with prelude.builder().with_options(options).build() as session:
    is_opengl_renderer = session.get_proc_address(b"melondsds_is_opengl_renderer", CFUNCTYPE(c_bool))
    assert is_opengl_renderer is not None, "melondsds_is_opengl_renderer not defined in the core"

    is_software_renderer = session.get_proc_address(b"melondsds_is_software_renderer", CFUNCTYPE(c_bool))
    assert is_software_renderer is not None, "melondsds_is_software_renderer not defined in the core"

    assert is_opengl_renderer()

    for i in range(3):
        session.run()

    session.options.variables["melonds_render_mode"] = b"software"

    for i in range(3):
        session.run()

    assert is_software_renderer()
