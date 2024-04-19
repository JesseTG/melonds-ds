from ctypes import *

import prelude

with prelude.builder().with_options(1).build() as session:
    get_options_version = session.get_proc_address(b"libretropy_get_options_version", CFUNCTYPE(c_uint))

    assert get_options_version is not None
    assert get_options_version() == 1
