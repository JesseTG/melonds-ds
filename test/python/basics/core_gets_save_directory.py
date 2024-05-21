from ctypes import *
from libretro import Session

import prelude

session: Session
with prelude.session() as session:
    get_save_directory = session.get_proc_address(b"libretropy_get_save_directory", CFUNCTYPE(c_char_p))
    assert get_save_directory is not None

    save_directory = get_save_directory()
    assert save_directory == session.environment.path.save_dir
