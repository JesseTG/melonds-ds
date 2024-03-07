from ctypes import *
from libretro import default_session, retro_get_proc_address_t

import prelude

with default_session(prelude.core_path, save_dir=prelude.save_directory) as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None

    get_proc_address: retro_get_proc_address_t = proc_address_callback.get_proc_address
    assert get_proc_address is not None

    get_save_directory = get_proc_address(b"libretropy_get_save_directory")
    assert get_save_directory is not None

    get_save_directory_callable = cast(get_save_directory, CFUNCTYPE(c_char_p))

    assert get_save_directory_callable is not None

    save_directory_pointer = get_save_directory_callable()
    assert isinstance(save_directory_pointer, c_char_p)

    assert string_at(save_directory_pointer) == session.save_dir
