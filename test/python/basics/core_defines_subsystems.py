import libretro.session
from libretro import default_session
from pprint import pprint

import prelude

with default_session(prelude.core_path, libretro.session.DoNotLoad) as session:
    subsystems = session.subsystems

    pprint(subsystems)
    assert subsystems is not None
    assert len(subsystems) > 0
    assert all(s.desc for s in subsystems)
