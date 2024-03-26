from ctypes import *

from libretro import Session
from libretro.api.options import DictOptionDriver

import prelude

session: Session
with prelude.session(options=DictOptionDriver(1)) as session:
    get_options_version = session.get_proc_address(b"libretropy_get_options_version", CFUNCTYPE(c_uint))

    assert get_options_version is not None
    assert get_options_version() == 1
