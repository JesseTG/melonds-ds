from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    get_message_version = session.get_proc_address(b"libretropy_get_message_version", CFUNCTYPE(c_uint))

    assert get_message_version is not None
    assert get_message_version() == 1
