from ctypes import *
from libretro import default_session, retro_get_proc_address_t

import prelude

with default_session(prelude.core_path) as session:
    descriptors = session.input_descriptors

    assert descriptors is not None
    assert len(descriptors) > 0
    assert all(d.description for d in descriptors)
