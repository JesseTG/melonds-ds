from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    get_input_bitmasks = session.get_proc_address("libretropy_get_input_bitmasks", CFUNCTYPE(c_bool))

    assert get_input_bitmasks is not None
    assert get_input_bitmasks()
