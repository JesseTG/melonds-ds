from collections.abc import Sequence
from pprint import pprint

from libretro import Session
from libretro.api.content import retro_subsystem_info

import prelude

subsystems: Sequence[retro_subsystem_info]
session: Session
with prelude.noload_session() as session:
    subsystems = session.subsystems

    assert subsystems is not None
    assert len(subsystems) > 0
    assert all(s.desc for s in subsystems)

# Testing this _after_ unloading the core to ensure the data is still valid
pprint(subsystems)
