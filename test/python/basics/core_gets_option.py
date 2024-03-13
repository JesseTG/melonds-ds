from ctypes import *

from libretro import Session
import prelude

session: Session
with prelude.session() as session:
    get_option = session.get_proc_address("libretropy_get_option", CFUNCTYPE(c_char_p, c_char_p))

    assert get_option is not None
    assert get_option(b"melonds_touch_mode") == b'auto'

    session.options.variables[b"melonds_touch_mode"] = b'touch'
    assert get_option(b"melonds_touch_mode") == b'touch'
