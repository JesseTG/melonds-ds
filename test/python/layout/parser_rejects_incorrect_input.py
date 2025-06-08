from collections.abc import Callable
from ctypes import *

from libretro import Session

import prelude

session: Session
with prelude.noload_session() as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None
    assert proc_address_callback.get_proc_address is not None

    is_valid_vfl: Callable[[bytes], bool] = session.get_proc_address(b"melondsds_is_valid_vfl", CFUNCTYPE(c_bool, c_char_p))
    assert is_valid_vfl is not None

    assert not is_valid_vfl(b"")
    assert False

    # TODO: Write test cases for invalid VFL strings
