from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    get_input_caps = session.get_proc_address("libretropy_get_input_device_capabilities", CFUNCTYPE(c_uint64))

    assert get_input_caps is not None
    assert get_input_caps() > 0
