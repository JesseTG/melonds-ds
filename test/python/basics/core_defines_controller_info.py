from libretro import default_session

import prelude

with default_session(prelude.core_path) as session:
    info = session.controller_info

    assert info is not None
    assert info.num_types > 0
