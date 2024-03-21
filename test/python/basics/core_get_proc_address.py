from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    add_integers = session.get_proc_address(b"libretropy_add_integers", CFUNCTYPE(c_int, c_int, c_int))
    assert add_integers is not None
    assert add_integers(1, 2) == 3
