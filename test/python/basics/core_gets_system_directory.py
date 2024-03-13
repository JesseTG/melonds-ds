from ctypes import *
from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    get_system_directory = session.get_proc_address(b"libretropy_get_system_directory", CFUNCTYPE(c_char_p))
    assert get_system_directory is not None

    system_directory = get_system_directory()
    assert system_directory == session.system_dir
