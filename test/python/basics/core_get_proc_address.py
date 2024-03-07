from ctypes import *
from libretro import default_session, retro_get_proc_address_t

import prelude

with default_session(prelude.core_path) as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None

    get_proc_address: retro_get_proc_address_t = proc_address_callback.get_proc_address
    assert get_proc_address is not None

    add_integers = get_proc_address(b"libretropy_add_integers")
    assert add_integers is not None

    add_integers_callable = cast(add_integers, CFUNCTYPE(c_int, c_int, c_int))

    assert add_integers_callable is not None
    assert add_integers_callable(1, 2) == 3
