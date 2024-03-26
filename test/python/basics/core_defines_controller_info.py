from collections.abc import Sequence
from pprint import pprint
from libretro import Session
from libretro.api.input.info import retro_controller_info

import prelude

info: Sequence[retro_controller_info]
session: Session
with prelude.session() as session:
    info = session.controller_info

    assert info is not None
    assert len(info) > 0
    assert all(len(i) for i in info)


# Testing this _after_ unloading the core to ensure the data is still valid
pprint(info)
pprint(info[0])
