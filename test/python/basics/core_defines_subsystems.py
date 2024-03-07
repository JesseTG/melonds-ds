from libretro import default_session

import prelude

with default_session(prelude.core_path) as session:
    subsystems = session.subsystems

    assert subsystems is not None
    assert len(subsystems) > 0
    assert all(s.desc for s in subsystems)
