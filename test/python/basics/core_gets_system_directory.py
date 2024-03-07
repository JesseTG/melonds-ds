from ctypes import *
from libretro import default_session, retro_get_proc_address_t

import prelude

with default_session(prelude.core_path, system_dir=prelude.system_dir) as session:
    proc_address_callback = session.proc_address_callback
    assert proc_address_callback is not None

    get_proc_address: retro_get_proc_address_t = proc_address_callback.get_proc_address
    assert get_proc_address is not None

    get_system_directory = get_proc_address(b"libretropy_get_system_directory")
    assert get_system_directory is not None

    get_system_directory_callable = cast(get_system_directory, CFUNCTYPE(c_char_p))

    assert get_system_directory_callable is not None

    system_directory_pointer = get_system_directory_callable()
    assert isinstance(system_directory_pointer, c_char_p)

    assert string_at(system_directory_pointer) == session.system_dir
